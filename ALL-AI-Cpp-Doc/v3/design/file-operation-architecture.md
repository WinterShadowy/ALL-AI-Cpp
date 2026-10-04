# 文件操作架构（File Operation Architecture）

本页详细说明 ALL-AI-Cpp V3 文件子系统的设计：文件网关 `ai.Files`、文件类型识别、
策略模式（文件转对话）、多模态内容构建，以及适配任意站点的机制层三大入口。

阅读对象：

- **使用者**：确认文件应调用的接口及其返回内容；
- **维护者**：确认新需求落在哪个类、修改何处不破坏既有约定。

## 目录

1. [子系统全景图](#1-子系统全景图)
2. [URL 设计：显式传入，库不做推导](#2-url-设计显式传入库不做推导)
3. [文件类型识别：FileTypeDetector](#3-文件类型识别filetypedetector)
4. [文件上传：Files.Upload / UploadBatch](#4-文件上传filesupload--uploadbatch)
5. [文件管理：List / Info / Content / Delete](#5-文件管理list--info--content--delete)
6. [机制层：适配任意站点端点](#6-机制层适配任意站点端点)
7. [文件转对话：Files.ToMessages 与策略模式](#7-文件转对话filestomessages-与策略模式)
8. [多模态内容构建：ContentPartBuilder](#8-多模态内容构建contentpartbuilder)
9. [线程安全与错误处理约定](#9-线程安全与错误处理约定)
10. [维护者扩展清单](#10-维护者扩展清单)

---

## 1. 子系统全景图

```text
+-------------------------------------------------------------------------------------+
|                                 Application Layer                                   |
|          用户业务代码 / FileDemo-V3 / AudioDemo-V3 / MainTest-V3                |
+----------------------------+------------------------------+-------------------------+
                             |                              |
                             v                              v
+-------------------------------------------------------------------------------------+
|                              Facade Layer: ALL_AI::AI                               |
|                                                                                     |
|  文件网关 ai.Files（FileGateway，随 AI 构造注入）                                     |
|  +------------------+  +------------------+  +----------------------------------+   |
|  | Upload           |  | UploadBatch      |  | ToMessages                       |   |
|  | (单文件上传)      |  | (批量,自动类型)   |  | (文件 -> 对话messages，策略模式)  |   |
|  +------------------+  +------------------+  +----------------------------------+   |
|  +------------------+  +------------------+  +------------------+  +-------------+  |
|  | List             |  | Info             |  | Content          |  | Delete      |  |
|  +------------------+  +------------------+  +------------------+  +-------------+  |
|                                                                                     |
|  机制层（URL 一律调用参数显式传入）                                                    |
|  +------------------+  +------------------+  +----------------------------------+   |
|  | SendRequestRaw   |  | SendMultipart    |  | SetDataCallback                  |   |
|  | (原始GET/POST)    |  | Request (表单)  |  | (二进制流逐块回调)                 |   |
|  +------------------+  +------------------+  +----------------------------------+   |
+----------------------------+--------------------------------------------------------+
                             |
              +--------------+---------------+
              |                              |
              v                              v
+------------------------------+  +-------------------------------------------+
|      Strategy / Tool Layer   |  |            Transport Layer                |
|  FileOperator 命名空间:       |  |  IHttpTransport                           |
|   - FileTypeDetector         |  |   - SendRequest        (JSON/SSE)         |
|   - IFileProcessStrategy 家族 |  |   - SendMultipartRequest (multipart)     |
|   - FileStrategyFactory      |  |   - SendRequestRaw     (原始+回调)        |
|   - ContentPartBuilder       |  |  CurlHttpTransport (libcurl 默认实现)     |
+------------------------------+  +-------------------------------------------+
                                              |
                                              v
+-------------------------------------------------------------------------------------+
|                          External AI HTTP API                                       |
|   POST /v1/files    GET /v1/files[/{id}[/content]]    DELETE /v1/files/{id}         |
|   POST /v1/audio/transcriptions    POST /v1/audio/speech    POST /v1/images/...     |
+-------------------------------------------------------------------------------------+
```

**核心设计原则**：沉淀结构，不沉淀字段名。

- 文件网关只沉淀 **REST 资源路径结构**（`{url}` / `{url}/{id}` / `{url}/{id}/content`），
  URL 主机部分永远来自调用参数；
- 库不存储、不推导、不映射任何业务端点——端点表是用户自己的资产；
- 语音等"形状各异"的请求不收编为 AI 行为（STT=multipart、TTS=json进二进制出、
  音色上传=multipart多字段——无可沉淀结构），由机制层 + Demo 菜谱覆盖。

---

## 2. URL 设计：显式传入，库不做推导

**库内唯一的 URL 是聊天 URL**（`m_url`，构造 / `SetURL` 注入，`SendRequest` 系列的发送目标）。
其余一切 URL 都是调用参数：

```cpp
// 端点表是用户自己的资产，通常集中声明在配置区
const std::string FILES_URL = "https://api.moonshot.cn/v1/files";
const std::string STT_URL   = "https://api.siliconflow.cn/v1/audio/transcriptions";
const std::string TTS_URL   = "https://api.siliconflow.cn/v1/audio/speech";

ai.Files.Upload("Book.txt", "file-extract", FILES_URL);
ai.SendMultipartRequest(STT_URL, "voice.mp3", "file", {{"model", "..."}});
```

为什么不做端点推导 / 端点映射表？

1. 端点映射的内容（功能 → URL）全部来自**用户的站点文档**，库既不生产也无法验证，
   替用户维护一张表只会随着站点调整而过期；
2. URL 是一个长期可能变化的变量，把它交给 `ReloadAI` / 调用参数，
   比埋在库的映射表里更容易适配；
3. 上传行为本身（curl + multipart）在任何站点都几乎一致，变化的是 URL 与字段名——
   前者是参数，后者由 `form_fields` 显式传入，库一行适配代码都不用写。

---

## 3. 文件类型识别：FileTypeDetector

`FileOperator::FileTypeDetector` 是一个纯静态工具类，
根据**文件扩展名**（不读文件内容）完成三件事：识别类型、推导默认 purpose、查 MIME 类型。

```text
file_path
   |
   v
GetExtensionLower()   // 提取小写扩展名；无扩展名/点号属于目录名 -> 返回 ""
   |
   v
DetectFileType()      // 查四张扩展名表
   |
   +---> FileType::Image     jpg jpeg png gif webp bmp heic heif
   +---> FileType::Video     mp4 mpeg mov avi flv mpg webm wmv 3gpp
   +---> FileType::Audio     mp3 wav m4a flac ogg aac wma
   +---> FileType::Document  txt md pdf doc(x) xls(x) ppt(x) csv json xml html epub
   |                         c cpp h hpp java py rb sql js ts go css less sass scss jsonl...
   +---> FileType::Unknown   其余（默认按文档处理）
   |
   v
GetDefaultPurpose(type)          GetMimeType(path)
   |                                |
   |  Image  -> "image"             |  查 mime_map
   |  Video  -> "video"             |  未命中 -> "application/octet-stream"
   |  其他   -> "file-extract"      |
   v                                v
用于 UploadBatch 的 purpose 推导    用于 base64 data URL 的 MIME 头
```

| 公开静态方法 | 输入 | 输出 | 用途 |
| --- | --- | --- | --- |
| `DetectFileType` | 文件路径 | `FileType` 枚举 | 决定后续处理策略 |
| `GetDefaultPurpose` | `FileType` | `FilePurpose` 枚举 | 推导上传 purpose |
| `PurposeToString` | `FilePurpose` | 字符串 | 生成 API 的 purpose 字段 |
| `GetMimeType` | 文件路径 | MIME 字符串 | 组装 `data:...;base64,` URL |

> 注意：`Audio` 与 `Unknown` 的默认 purpose 是 `file-extract`（部分平台支持音频转写）。
> 若目标平台以其他方式处理音频，可参见第 7 节的自定义策略。

---

## 4. 文件上传：Files.Upload / UploadBatch

### 4.1 单文件上传链路

```text
AI::Files.Upload(file_path, purpose, url)
   |
   |  (1) 持 m_mutex_config 取 m_transport 快照
   |       +-- transport 为空 --> DoErrorThrow("HTTP transport is not set")，返回 {}
   |
   |  (2) form_fields["purpose"] = purpose
   |
   v
IHttpTransport::SendMultipartRequest(url, file_path, "file", form_fields)
   |
   v
服务器响应 json（含 id 字段） / 失败返回 {}
```

两个重载：

| 重载 | purpose 参数 | 说明 |
| --- | --- | --- |
| `Upload(path, const std::string& purpose, url)` | 字符串 | 通用，可传任意平台的 purpose |
| `Upload(path, FilePurpose purpose, url)` | 枚举 | 类型安全，内部 `PurposeToString` 后转调上一重载 |

返回约定：**成功**返回服务器 json（通常含 `id`、`bytes`、`filename`、`status` 等字段）；
**失败或响应无效**返回空 json 对象 `{}`。取文件 ID 用 `JsonGet`：

```cpp
nlohmann::json result = ai.Files.Upload("Book.txt", "file-extract", files_url);
std::string file_id = ALL_AI::JsonGet<std::string>(result, "id");
```

### 4.2 批量上传 UploadBatch

```text
AI::Files.UploadBatch(file_paths, url)
   |
   v
for each file_path:
   |
   |-- FileTypeDetector::DetectFileType(file_path)     // 自动识别类型
   |-- FileTypeDetector::GetDefaultPurpose(type)       // 自动推导 purpose
   |-- Upload(file_path, purpose, url)                 // 复用单文件链路
   |-- 提取 id 成功? -> success = true, file_id = ...
   |
   v
std::vector<FileUploadResult>   // 与输入路径一一对应
```

`FileUploadResult` 结构：

```cpp
struct FileUploadResult {
	std::string file_path;    // 本地文件路径
	std::string file_id;      // 上传成功时的文件ID
	FileType    file_type;    // 识别出的文件类型
	bool        success;      // 是否上传成功
	nlohmann::json raw_response;  // 服务器原始响应
};
```

**单个文件失败不影响其他文件**——失败的项 `success == false`、`file_id` 为空，
调用方逐项检查即可。

---

## 5. 文件管理：List / Info / Content / Delete

四个接口共享同一条简单链路（REST 资源路径是库唯一沉淀的结构）：

```text
Files.List(url)              -> GET    {url}
Files.Info(file_id, url)     -> GET    {url}/{file_id}
Files.Content(file_id, url)  -> GET    {url}/{file_id}/content
Files.Delete(file_id, url)   -> DELETE {url}/{file_id}
   |
   |  file_id 为空（后三个接口）--> DoErrorThrow(...)，返回空
   v
AI::SendRequestRaw(method, target_url)     // 机制层
   |
   v
FileGateway::ParseRawToJson(raw_string)    // 安全解析，失败返回 {}
   |
   v
nlohmann::json   （Content 例外：直接返回原始字符串）
```

| 接口 | HTTP | 返回 | 备注 |
| --- | --- | --- | --- |
| `List(url)` | GET | json | 已上传文件列表 |
| `Info(file_id, url)` | GET | json | 单文件元信息 |
| `Content(file_id, url)` | GET | **原始字符串** | KIMI 对 `file-extract` 文件返回抽取文本（通常是含 `content` 字段的 JSON 字符串）；此处不解析，由调用方决定 |
| `Delete(file_id, url)` | DELETE | json | 删除结果 |

> `Content` 故意返回原始字符串：不同站点响应格式不一（JSON 或纯文本），
> 策略层的 `ExtractTextContent` 会尝试解析 JSON 提取 `content` 字段，
> 失败则原样返回——这个"尽力解析"的逻辑只在需要文本的地方生效，
> 不污染原始接口。

---

## 6. 机制层：适配任意站点端点

语音、图片生成、视频生成等端点**形状各异**，不收编为 AI 行为，
统一由三个机制层入口覆盖（URL 与字段名全部显式传入）：

| 请求形状 | 入口 | 典型场景 |
| --- | --- | --- |
| json 进，json 出 | `SendRequestRaw(method, url, body)` | 图片/视频生成、任务轮询 |
| multipart 表单进，json 出 | `SendMultipartRequest(url, file, field, form_fields)` | STT 语音转写、音色上传 |
| json 进，二进制流出 | `SetDataCallback(...)` + `SendRequestRaw(...)` | TTS 文本转语音 |

```text
例：TTS（json -> mp3 二进制流）

ai.SetDataCallback([](const char* data, size_t size) -> size_t {
	// 逐块交给用户：写文件 / 喂播放器 / 自定义解析
	return size;   // 返回值 != size 时中止请求（libcurl 写回调语义）
});
ai.SendRequestRaw(HttpMethod::POST, tts_url, tts_body);   // 返回空字符串，数据归回调
ai.ClearDataCallback();   // 用完务必恢复默认的收集行为
```

设计要点：

- **数据回调是用户的资产**，库不检查交付的数据——返回音频就写文件，返回 SSE 就自行切分；
- 回调设置后作用于**之后的所有请求**，直到 `ClearDataCallback()` 恢复默认行为；
- 站点差异（字段名、返回结构）由调用方按站点文档适配，库不兜底——
  真实案例见 [站点适配指南](/v3/endpoint-abstraction.md)。

---

## 7. 文件转对话：Files.ToMessages 与策略模式

### 7.1 高层入口

```cpp
nlohmann::json Files.ToMessages(const std::vector<std::string>& file_paths,
	const std::string& url);
```

一次调用把任意混合的本地文件（文档、图片、视频、音频……）转换为
可直接用于 `messages` 参数的数组。**文本类内容变成 system 消息，媒体类内容合并为一条 user 消息。**

### 7.2 策略模式全景

```text
AI::Files.ToMessages(file_paths, url)
   |
   v
for each file_path:
   |
   |-- FileTypeDetector::DetectFileType(file_path)
   |
   |-- FileStrategyFactory::Create(file_type) --------+
   |       |                                          |
   |       |  (1) 查 m_custom_strategies（持锁）        |
   |       |      用户注册过该类型的自定义策略？          |
   |       |       +-- 是 --> 返回自定义策略            |
   |       |       |                                  |
   |       |  (2) 否 -> switch 默认策略：              |
   |       v                                          v
   |   +--------------------+   返回 shared_ptr<IFileProcessStrategy>
   |   | Image  -> ImageFileStrategy        |
   |   | Video  -> VideoFileStrategy        |
   |   | Audio  -> AudioFileStrategy        |
   |   | 其他   -> DocumentFileStrategy     |
   |   +--------------------+               |
   |                                        |
   |-- strategy->Process(ai, file_path, out_messages, out_parts)
   |       |
   |       +-- 文本类结果 --> push 到 out_messages（system 消息）
   |       +-- 媒体类结果 --> push 到 out_parts（content part）
   |
   |  Process 返回 false --> DoErrorThrow("failed to process file: ...")
   |
   v
if (!out_parts.empty())
	messages.push_back({"role":"user", "content": out_parts})   // 媒体合并为一条user消息
return messages
```

### 7.3 四个默认策略的行为对照

| 策略 | 对应类型 | 处理流程 | 产出 |
| --- | --- | --- | --- |
| `DocumentFileStrategy` | Document / Unknown | 上传(`file-extract`) → `Files.Content` 抽取文本 | system 消息 |
| `ImageFileStrategy` | Image | **默认**：base64 → `data:image/...;base64,...` 的 image_url part；可 `SetTransportMode(UploadReference)` 切换为上传(`image`)后 `ms://{file_id}` 引用 | image_url part |
| `VideoFileStrategy` | Video | 上传(`video`) → `ms://{file_id}` 的 video_url part | video_url part |
| `AudioFileStrategy` | Audio | 默认按 `file-extract` 上传并抽取文本（部分平台支持音频转写） | system 消息 |

策略共享的 protected 辅助函数（`IFileProcessStrategy`）：

- `ExtractFileId(upload_result)` —— 安全提取文件 ID；
- `ExtractTextContent(raw)` —— 尽力从 raw 中提取文本（JSON 取 `content`，否则原样）；
- `UploadAndGetId(ai, path, purpose, url)` —— 上传 + 提取 ID 的组合操作。

### 7.4 自定义策略（开闭原则的实际应用）

```cpp
// 例：音频改用 base64 audio_url part，而不是默认的 file-extract 上传
class MyAudioStrategy : public ALL_AI::FileOperator::IFileProcessStrategy {
public:
	virtual FilePurpose GetPurpose() const override
	{
		return FilePurpose::FileExtract;
	}

	virtual bool Process(ALL_AI::AI& ai, const std::string& file_path,
		nlohmann::json& out_messages, nlohmann::json& out_parts) override
	{
		std::string base64_data = ALL_AI::JsonOperatorTools::FileToBase64(file_path);
		if (base64_data.empty()) { return false; }
		// ... 组装目标平台要求的 part 结构，push 到 out_parts
		return true;
	}
};

// 注册（全局生效）；传 nullptr 恢复默认
FileStrategyFactory::RegisterStrategy(FileType::Audio,
	std::make_shared<MyAudioStrategy>());
```

`RegisterStrategy` / `Create` 内部均持锁，可在多线程环境下安全注册。

---

## 8. 多模态内容构建：ContentPartBuilder

`FileOperator::ContentPartBuilder` 是 Builder 模式的链式构建器，
用于手工组装多模态 user 消息（适用于不使用 `Files.ToMessages` 全自动策略的场景）：

```cpp
auto msg = ALL_AI::FileOperator::ContentPartBuilder()
	.AddText(u8"描述这张图片，并转写这段音频的内容")
	.AddImageBase64("girl.png")       // 本地图片 -> data:image/png;base64,...
	.AddAudioBase64("voice.mp3")      // 本地音频 -> data:audio/mpeg;base64,...
	.BuildUserMessage();              // {"role":"user","content":[...parts]}
```

| 方法 | 输入 | 生成的 part | 读文件失败时 |
| --- | --- | --- | --- |
| `AddText(text)` | 字符串 | `{"type":"text", ...}` | —— |
| `AddImageBase64(path)` | 本地图片 | `image_url`（data URL） | 不添加该 part |
| `AddImageFileId(file_id)` | 已上传 ID | `image_url`（`ms://` 引用） | —— |
| `AddVideoFileId(file_id)` | 已上传 ID | `video_url`（`ms://` 引用） | —— |
| `AddAudioBase64(path)` | 本地音频 | `audio_url`（data URL） | 不添加该 part |
| `BuildParts()` | —— | parts 数组 | —— |
| `BuildUserMessage()` | —— | 完整 user 消息 json | —— |

内部复用 `JsonOperatorTools::FileToBase64`（读文件 + Base64 编码）
与 `FileTypeDetector::GetMimeType`（MIME 识别）。
新增一种 base64 内嵌类型（如未来嵌入 PDF）时，参照 `AddImageBase64` / `AddAudioBase64`
的对称写法增加对应的 `AddXxxBase64` 方法即可。

> **站点差异实测**：硅基流动网关接受 OpenAI 约定的 `audio_url` 音频 part，
> 拒绝上游 DashScope 风格的 `input_audio` part（400, code 20029
> "Only text and image_url are supported"），且不要求 stream+modalities。
> 切换站点时按站点文档选择 part 格式，差异部分用
> `JsonOperatorTools::FileToBase64` 手工组装即可。

---

## 9. 线程安全与错误处理约定

| 约定 | 内容 |
| --- | --- |
| 配置锁 | `m_mutex_config` 保护 `m_url` / `m_api_key` / `m_transport` / `m_data_callback`。读配置先取快照再解锁使用，**不持锁发网络请求** |
| 初始化锁 | `m_mutex_ai_init` 保护 `InitAI` / `ReloadAI`，与 `m_mutex_config` 构成固定加锁顺序（先 config 后 init），避免死锁 |
| 策略注册锁 | `FileStrategyFactory` 的自定义策略表有独立互斥锁；C++17 用 `inline static`，C++14 用函数内 static 单例（宏 `__ALL_AI_CXX_VERSION` 控制） |
| 句柄状态清理 | curl easy 句柄复用，每个请求函数开头清理残留标志（`POST / POSTFIELDS / POSTFIELDSIZE / NOBODY / MIMEPOST`）；**新增请求路径必须延续此约定** |
| 错误处理 | 一律 `DoErrorThrow(...)`，禁止直接 `throw` / `std::cerr`；返回空值（`{}` / `""` / `false`）表示失败 |
| 失败不扩散 | `UploadBatch` 单项失败不影响其他项；`ContentPartBuilder` 单个 part 读文件失败不添加该 part |

---

## 10. 维护者扩展清单

### 10.1 新增一种文件类型（如压缩包）

1. `FileType` 枚举加值；
2. `FileTypeDetector::DetectFileType` 加扩展名集合与分支；
3. `GetDefaultPurpose` / `GetMimeType` 补映射；
4. 如需独立处理逻辑：新增 `IFileProcessStrategy` 子类，
   并在 `FileStrategyFactory::Create` 的 `switch` 中挂接。

### 10.2 覆盖默认行为而不改库代码

优先使用现成扩展点，而不是 fork 修改：

| 需求 | 扩展点 |
| --- | --- |
| 某类型文件换个处理方式 | `FileStrategyFactory::RegisterStrategy` |
| 站点端点 URL 不同 | 调用参数显式传入（建议集中声明在配置区） |
| 站点表单字段名不同 | `SendMultipartRequest` 的 `form_fields` 参数 |
| 站点返回结构不同 | `JsonGet` 按站点文档取值，或自定义 `DataCallback` |
| 换 HTTP 库 | 实现 `IHttpTransport`，`SetHttpTransport` 注入 |
| 换错误处理方式 | `SetErrorThrow` / `SetThrowErrorCallbackFunction` |

### 10.3 新增一个请求函数（给传输层）

1. 优先复用现有三个机制层入口——它们已覆盖"json 进出 / multipart / 二进制回调"全部形状；
2. 确需新增时，在 `IHttpTransport` 加**带默认实现的虚函数**（报"不支持"），
   保证已有自定义传输类无需修改即可继续编译；
3. 实现内必须延续状态清理约定（见第 9 节"句柄状态清理"）；
4. 三个头文件变体（`ALL-AI-V3.hpp` / `-CN` / `-EN`）同步，C++14 与 C++17 各做一次语法检查。
