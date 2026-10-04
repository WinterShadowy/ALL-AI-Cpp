# AI 类参考 (AI Class Reference)

`ALL_AI::AI` 是库的核心门面类，统一管理配置、HTTP 传输、请求构建、
文件网关（`Files` 成员）与数据回调。本文档按函数逐一给出签名、参数、
返回值、错误行为与调用示例。完整可运行的调用序列请参阅
[快速上手](/v3/getting-started.md) 与 [示例 Demo](/v3/demo-explained.md)。

- **命名空间**：`ALL_AI`
- **头文件**：`#include "ALL-AI-V3.hpp"`（或 `ALL-AI-V3-CN.hpp` / `ALL-AI-V3-EN.hpp`）

---

## 1. 类型定义

### HttpMethod

```cpp
enum class HttpMethod {
	POST,
	GET,
	DELETE
};
```

HTTP 请求方法枚举，用于 `SendRequest` 与 `SendRequestRaw`。

### DataCallback

```cpp
using DataCallback = std::function<size_t(const char* data, size_t size)>;
```

数据回调类型。响应数据到达时逐块调用，返回值语义与 libcurl 写回调一致：
返回已消费的字节数；返回值不等于传入的 `size` 时中止当前请求。
详见 7.3 节 `SetDataCallback` / `ClearDataCallback`。

### ALL_AI_ErrorThrow

```cpp
enum class ALL_AI_ErrorThrow {
	ALL_AI_PRINT_ERROR,			// 打印错误信息到标准输出
	ALL_AI_CALLBACK_FUNCTION,	// 通过回调函数上报错误信息
	ALL_AI_EXCEPTION_THROWING,	// 抛出 std::runtime_error 异常
	ALL_AI_NO_ERROR_THROW		// 不上报错误（默认）
};
```

错误上报策略枚举。库内所有可恢复错误统一经由 `DoErrorThrow` 按此策略处理。
无论选择哪种策略，接口的失败返回值约定不变（空 json / 空字符串 / `false`）。

---

## 2. 成员函数总览

| 分类 | 函数 | 功能 |
| --- | --- | --- |
| 构造 | `AI()` / `AI(transport, url, api_key, error_throw)` | 构造 AI 实例 |
| 配置 | `SetURL` / `SetKey` / `SetHttpTransport` | 设置聊天 URL、API Key、传输层 |
| 配置 | `SetErrorThrow` / `SetThrowErrorCallbackFunction` | 设置错误策略与错误回调 |
| 生命周期 | `InitAI` / `ReloadAI` | 初始化 / 运行中重载配置 |
| 对话请求 | `SendRequestFromBuilder_Post` / `SendRequestFromBuilder_Get` | 以构建器内容发送请求（主路径） |
| 对话请求 | `SendRequest` / `SendRequest_POST` / `SendRequest_GET` | 以显式 json 发送请求 |
| 机制层 | `SendRequestRaw`（两个重载） | 任意 URL 的原始 GET/POST，返回原始字符串 |
| 机制层 | `SendMultipartRequest` | multipart/form-data 表单请求 |
| 机制层 | `SetDataCallback` / `ClearDataCallback` | 二进制流逐块回调开关 |
| 组件访问 | `GetBuilder` / `GetBuilderData` / `GetTools` | 取用构建器与工具类 |
| 文件网关 | `Files.Upload` / `UploadBatch` / `ToMessages` | 文件上传与文件转对话 |
| 文件网关 | `Files.List` / `Info` / `Content` / `Delete` | 文件管理 |

---

## 3. 构造函数

### 3.1 默认构造函数

```cpp
explicit AI();
```

构造一个未配置的 AI 实例。使用前应依次调用 `SetURL`、`SetKey`、
`SetHttpTransport`，最后调用 `InitAI` 完成初始化。

**示例**

```cpp
ALL_AI::AI ai;
ai.SetHttpTransport(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>());
ai.SetURL("https://api.moonshot.cn/v1/chat/completions");
ai.SetKey("YOUR_API_KEY");
if (!ai.InitAI()) { /* 初始化失败 */ }
```

### 3.2 带参构造函数

```cpp
explicit AI(std::shared_ptr<IHttpTransport> transport,
	const std::string& url,
	const std::string& api_key,
	const ALL_AI_ErrorThrow all_ai_error_throw = ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW);
```

一步完成传输层、聊天 URL、API Key 与错误策略的配置，随后调用 `InitAI` 即可使用。

| 参数 | 说明 |
| --- | --- |
| `transport` | `IHttpTransport` 实现实例的共享指针，通常为 `HttpTransport::CurlHttpTransport` |
| `url` | 完整的聊天端点 URL。这是库内唯一保存的 URL，其余一切端点 URL 均为调用参数 |
| `api_key` | API Key，传输层据此生成 `Authorization: Bearer <api_key>` 认证头 |
| `all_ai_error_throw` | 错误上报策略，默认 `ALL_AI_NO_ERROR_THROW` |

**示例**

```cpp
ALL_AI::AI ai(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
	"https://api.moonshot.cn/v1/chat/completions",
	"YOUR_API_KEY",
	ALL_AI::ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW);
```

---

## 4. 配置函数

### 4.1 SetURL

```cpp
void SetURL(const std::string& url);
```

设置或更新聊天端点 URL。线程安全（持 `m_mutex_config`）。

| 参数 | 说明 |
| --- | --- |
| `url` | 完整的聊天端点 URL |

**备注**：在 `InitAI` 之后调用本函数仅更新成员变量，已初始化的传输层不受影响；
如需立即生效，请使用 `ReloadAI`。

### 4.2 SetKey

```cpp
void SetKey(const std::string& key);
```

设置或更新 API Key。线程安全。生效时机同 `SetURL`。

### 4.3 SetHttpTransport

```cpp
void SetHttpTransport(std::shared_ptr<IHttpTransport> transport);
```

设置或替换 HTTP 传输层实现。线程安全。

| 参数 | 说明 |
| --- | --- |
| `transport` | `IHttpTransport` 实现实例的共享指针；传入 `nullptr` 时忽略本次调用 |

**备注**：用于注入自定义传输层（如基于 CPR、Boost.Beast 的实现），
接口要求见 [HTTP 传输](/v3/api/transport.md)。

### 4.4 SetErrorThrow

```cpp
void SetErrorThrow(ALL_AI_ErrorThrow error_throw);
```

切换错误上报策略，取值见 1 节 `ALL_AI_ErrorThrow` 枚举。

### 4.5 SetThrowErrorCallbackFunction

```cpp
// C++17
void SetThrowErrorCallbackFunction(
	std::function<void(const std::string_view& message)> callback_function);
// C++14
void SetThrowErrorCallbackFunction(
	std::function<void(const std::string& message)> callback_function);
```

注册错误回调函数（继承自基类 `ThrowError`）。签名随 C++ 标准版本由宏
`__ALL_AI_CXX_VERSION` 自动适配。

**前提**：错误策略须先设置为 `ALL_AI_CALLBACK_FUNCTION`，否则回调不会被调用。

**示例**

```cpp
ai.SetErrorThrow(ALL_AI::ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION);
ai.SetThrowErrorCallbackFunction([](const std::string_view& message) {
	// 上报日志系统 / 写入文件等
});
```

---

## 5. 生命周期

### 5.1 InitAI

```cpp
bool InitAI();
```

初始化 AI 实例：校验配置完整性，并调用传输层的 `Initialize` 完成
认证头与基础 URL 的配置。

**返回值**：初始化成功返回 `true`；重复初始化、URL / API Key / 传输层任一未设置时
按错误策略处理并返回 `false`。

**备注**：

- 发送任何请求前必须成功调用一次本函数；
- 本函数与 `ReloadAI` 由 `m_mutex_ai_init` 保护，可与配置写操作并发调用，不会死锁。

### 5.2 ReloadAI

```cpp
bool ReloadAI(std::string url = "",
	std::string api_key = "",
	std::shared_ptr<IHttpTransport> transport = std::make_shared<HttpTransport::CurlHttpTransport>());
```

运行中重载配置并重新初始化传输层，适用于切换站点、轮换 Key 等场景。

| 参数 | 说明 |
| --- | --- |
| `url` | 新的聊天端点 URL；空字符串表示保留当前值 |
| `api_key` | 新的 API Key；空字符串表示保留当前值 |
| `transport` | 新的传输层实例；**默认值为一个全新的 `CurlHttpTransport`** |

**返回值**：传输层重新初始化的结果。

**注意**：第三个参数带默认值——`ai.ReloadAI(new_url)` 会将传输层替换为新的
`CurlHttpTransport`。仅修改 URL 而保留自定义传输层时，应显式传回原实例：
`ai.ReloadAI(new_url, "", original_transport)`。

**示例**：见 `Demo/DemoCN(Demo-Reload_AI.cpp)`。

---

## 6. 对话请求

对话请求发往构造 / `SetURL` / `ReloadAI` 注入的**聊天端点 URL**。
请求体 `stream=true` 时，传输层自动切换 SSE 接收并将事件流合并为单个 json，
下游取值路径与非流式完全一致。

### 6.1 SendRequestFromBuilder_Post

```cpp
nlohmann::json SendRequestFromBuilder_Post();
```

以构建器（`GetBuilder()`）当前内容作为请求体发送 POST 请求。
这是对话链路的主路径。

**返回值**：服务器响应 json；请求失败或响应无效时返回空 json `{}`。

**示例**

```cpp
ai.GetBuilder().SetValue("kimi-k2.6", "model");
ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER, "Hello");
ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

nlohmann::json response = ai.SendRequestFromBuilder_Post();
std::string content = ALL_AI::JsonGet<std::string>(response, "choices", 0, "message", "content");
```

### 6.2 SendRequestFromBuilder_Get

```cpp
nlohmann::json SendRequestFromBuilder_Get();
```

向聊天端点发送 GET 请求（空请求体）。返回值约定同上。
绝大多数对话站点不使用本函数，保留用于个别 GET 形态的兼容接口。

### 6.3 SendRequest / SendRequest_POST / SendRequest_GET

```cpp
nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json);
nlohmann::json SendRequest_POST(const nlohmann::json request_json);
nlohmann::json SendRequest_GET(const nlohmann::json request_json);
```

以显式给出的 json 作为请求体，向聊天端点发送请求，**不读取构建器内容**。
`SendRequest_POST` / `SendRequest_GET` 是 `SendRequest` 的便捷封装。

| 参数 | 说明 |
| --- | --- |
| `method` | `HttpMethod::POST` / `GET` / `DELETE` |
| `request_json` | 请求体 json（GET 时可为空对象） |

**返回值**：服务器响应 json；失败返回 `{}`。

**适用场景**：多线程环境下的"局部构建"模式（各线程持有独立请求体，
避免共享构建器产生数据竞争），见 10 节"线程安全"。

---

## 7. 机制层请求

机制层是适配任意站点端点的三个底层入口。**URL 一律由调用方显式传入**，
库不存储、不推导、不映射任何业务端点。

### 7.1 SendRequestRaw

```cpp
std::string SendRequestRaw(HttpMethod method, const std::string& url);
std::string SendRequestRaw(HttpMethod method, const std::string& url,
	const nlohmann::json& body);
```

向任意 URL 发送普通 HTTP 请求，返回**原始响应字符串**（不做 JSON 解析）。

| 参数 | 说明 |
| --- | --- |
| `method` | HTTP 方法 |
| `url` | 目标端点的完整 URL |
| `body` | （重载）JSON 请求体，TTS、图片/视频生成等端点需要 |

**返回值**：原始响应字符串；失败返回空字符串。
已通过 `SetDataCallback` 设置数据回调时，响应逐块交付回调，本函数返回空字符串。

**适用场景**：模型列表（GET）、文件内容获取、图片生成、视频生成任务创建与轮询等
一切"json 进 json 出"的端点。

**示例**

```cpp
std::string raw = ai.SendRequestRaw(ALL_AI::HttpMethod::GET,
	"https://api.moonshot.cn/v1/models");
nlohmann::json list = nlohmann::json::parse(raw, nullptr, false);
```

### 7.2 SendMultipartRequest

```cpp
nlohmann::json SendMultipartRequest(const std::string& url,
	const std::string& file_path,
	const std::string& file_field_name,
	const std::unordered_map<std::string, std::string>& form_fields);
```

发送 multipart/form-data 表单请求，返回解析后的 json。

| 参数 | 说明 |
| --- | --- |
| `url` | 目标端点的完整 URL |
| `file_path` | 本地文件路径；**留空表示纯字段表单**（不携带文件） |
| `file_field_name` | 表单中文件字段的名称（OpenAI 兼容接口为 `"file"`） |
| `form_fields` | 其余表单字段。字段名完全由调用方按站点文档给出，库不预设 |

**返回值**：服务器响应 json；失败返回 `{}`。

**适用场景**：语音转写（STT）、音色上传、自定义文件上传等一切
"multipart 表单进 json 出"的端点。

**示例**

```cpp
nlohmann::json stt = ai.SendMultipartRequest(
	"https://api.siliconflow.cn/v1/audio/transcriptions",
	"TestAudio.mp3", "file",
	{ {"model", "FunAudioLLM/SenseVoiceSmall"} });
std::string text = ALL_AI::JsonGet<std::string>(stt, "text");
```

### 7.3 SetDataCallback / ClearDataCallback

```cpp
void SetDataCallback(DataCallback data_callback);
void ClearDataCallback();
```

`SetDataCallback` 设置数据回调：其后的所有请求收到响应数据块时**逐块交付回调**
而不再收集，用于"json 进二进制流出"的端点（TTS 音频流）、SSE 与大文件下载。
`ClearDataCallback` 清除回调，恢复默认的"收完响应再解析"行为。

**备注**：

- 回调返回值遵循 libcurl 写回调语义：返回已消费字节数，不等于 `size` 时中止请求；
- 回调设置后对**之后的所有请求**生效，使用完毕必须调用 `ClearDataCallback()`，
  否则后续普通请求的响应也会被送入回调；
- 交付的数据内容由回调解释，库不做任何检查。

**示例**（TTS 二进制流写文件）

```cpp
std::ofstream out("tts.mp3", std::ios::binary);
ai.SetDataCallback([&out](const char* data, size_t size) -> size_t {
	out.write(data, static_cast<std::streamsize>(size));
	return size;
});
ai.SendRequestRaw(ALL_AI::HttpMethod::POST, tts_url, tts_body);
ai.ClearDataCallback();	// 恢复默认收集行为
```

---

## 8. 组件访问

### 8.1 GetBuilder

```cpp
JsonOperator::JsonRequestBuilder& GetBuilder();
```

返回请求构建器**引用**，用于以链式 / 分步方式构建请求体
（`SetValue` 深层设值、数组操作等）。构建器方法列表见
[JSON 工具](/v3/api/json-tools.md)。

### 8.2 GetBuilderData

```cpp
nlohmann::json GetBuilderData();
```

返回构建器当前内容的**副本**（持锁导出），等价于
`GetBuilder().BuilderToJson()`。适用于发送前检查请求体或将其另存。

### 8.3 GetTools

```cpp
JsonOperatorTools& GetTools();
```

返回 JSON 操作工具**引用**，主要用于消息历史管理
（`PushBackArray` / `PopBackArray` / `GetMessagesArray`），
详见 [JSON 工具](/v3/api/json-tools.md)。

---

## 9. 文件网关 Files

```cpp
FileGateway Files;	// AI 的公开值成员，随 AI 构造注入
```

`Files` 封装 OpenAI 兼容的 `/v1/files` 文件接口，自身零状态，
**所有 URL 参数一律必填**。文件网关只沉淀 REST 资源路径结构
（`{url}` / `{url}/{id}` / `{url}/{id}/content`），URL 主机部分由调用方给出。

### 9.1 相关类型

```cpp
// FileOperator 命名空间
enum class FilePurpose {
	FileExtract,	// "file-extract"：抽取文件内容（文档/文本类）
	Image,			// "image"：上传图片，用于视觉理解
	Video,			// "video"：上传视频，用于视频理解
	Batch			// "batch"：上传 JSONL 文件，用于批处理任务
};

struct FileUploadResult {
	std::string		file_path;		// 本地文件路径
	std::string		file_id;		// 上传成功时的文件 ID
	FileType		file_type;		// 自动识别出的文件类型
	bool			success;		// 是否上传成功
	nlohmann::json	raw_response;	// 服务器原始响应
};
```

### 9.2 Files.Upload

```cpp
nlohmann::json Upload(const std::string& file_path,
	const std::string& purpose,
	const std::string& url);
nlohmann::json Upload(const std::string& file_path,
	FileOperator::FilePurpose purpose,
	const std::string& url);
```

上传单个文件（multipart/form-data）。枚举重载内部经 `PurposeToString`
转换后转调字符串重载。

| 参数 | 说明 |
| --- | --- |
| `file_path` | 本地文件路径 |
| `purpose` | 文件用途：KIMI 为 `"file-extract"`，OpenAI 为 `"assistants"` / `"fine-tune"` 等 |
| `url` | 文件接口完整 URL，如 `https://api.moonshot.cn/v1/files` |

**返回值**：服务器响应 json（通常含 `id`、`bytes`、`filename`、`status`）；
失败返回 `{}`。取文件 ID：`ALL_AI::JsonGet<std::string>(result, "id")`。

**示例**

```cpp
nlohmann::json up = ai.Files.Upload("Book.txt", "file-extract",
	"https://api.moonshot.cn/v1/files");
std::string file_id = ALL_AI::JsonGet<std::string>(up, "id");
```

### 9.3 Files.UploadBatch

```cpp
std::vector<FileOperator::FileUploadResult> UploadBatch(
	const std::vector<std::string>& file_paths,
	const std::string& url);
```

批量上传：逐文件自动识别类型（`FileTypeDetector`）并推导默认 purpose
（图片 → `image`，视频 → `video`，其余 → `file-extract`）。
**单个文件失败不影响其他文件**，结果数组与输入路径一一对应，
逐项检查 `success` 字段即可。

### 9.4 Files.ToMessages

```cpp
nlohmann::json ToMessages(const std::vector<std::string>& file_paths,
	const std::string& url);
```

高层封装：一次调用将任意混合的本地文件转换为可直接用于对话的 messages 数组
（内部为策略模式，可经 `FileStrategyFactory::RegisterStrategy` 自定义）：

- 文档 / 音频：上传（`file-extract`）并抽取内容 → system 消息；
- 图片：base64 编码 → `image_url` content part；
- 视频：上传（`video`）并以文件 ID 引用 → `video_url` content part；
- 所有媒体 part 合并为一条 user 消息。

**返回值**：messages 数组。调用方应将用户问题追加到数组末尾后再发起对话。
策略内部的全部上传统一经过 `url` 参数。

**示例**：见 [常见用法 - 文件转对话](/v3/api/common-usage.md) 与
`Demo/DemoCN(FileDemo-V3.cpp)`。

### 9.5 Files.List

```cpp
nlohmann::json List(const std::string& url);	// GET {url}
```

获取已上传文件列表。返回服务器 json，失败返回 `{}`。

### 9.6 Files.Info

```cpp
nlohmann::json Info(const std::string& file_id, const std::string& url);	// GET {url}/{file_id}
```

获取单个文件的元信息。`file_id` 为空时按错误策略处理并返回 `{}`。

### 9.7 Files.Content

```cpp
std::string Content(const std::string& file_id, const std::string& url);	// GET {url}/{file_id}/content
```

获取文件内容，**返回原始字符串**（不解析）。KIMI 对 `file-extract` 文件返回
抽取文本（通常为含 `content` 字段的 JSON 字符串）；不同站点响应格式不一
（JSON 或纯文本），解析方式由调用方按站点文档决定。

### 9.8 Files.Delete

```cpp
nlohmann::json Delete(const std::string& file_id, const std::string& url);	// DELETE {url}/{file_id}
```

删除指定文件，返回服务器 json，失败返回 `{}`。

### 9.9 命名空间级自由函数

以下自由函数是对 `ai.Files` 成员的一行转发，供偏好扁平调用风格的代码使用，
语义与成员函数完全一致：

```cpp
ALL_AI::UploadFile(ai, path, purpose, url);		// = ai.Files.Upload
ALL_AI::UploadFiles(ai, paths, url);			// = ai.Files.UploadBatch
ALL_AI::FilesToMessages(ai, paths, url);		// = ai.Files.ToMessages
ALL_AI::GetFileList(ai, url);					// = ai.Files.List
ALL_AI::GetFileInfo(ai, file_id, url);			// = ai.Files.Info
ALL_AI::GetFileContent(ai, file_id, url);		// = ai.Files.Content
ALL_AI::DeleteFile(ai, file_id, url);			// = ai.Files.Delete
```

---

## 10. 线程安全

### 10.1 线程安全保证

- **请求发送**（`SendRequest` 系列 / 机制层 / 文件网关）：线程安全。
  传输层持有请求互斥锁，对同一 `AI` 实例的并发请求会被串行化；
  需要高并发吞吐量时应创建多个 `AI` 实例（各自持有独立传输层）。
- **配置读写**：读取配置（URL / Key / Transport / DataCallback）时先持
  `m_mutex_config` 取快照再解锁使用，不持锁执行网络请求；
  `InitAI` / `ReloadAI` 由 `m_mutex_ai_init` 保护，两把锁构成固定加锁顺序
  （先 config 后 init），避免死锁。

### 10.2 多线程推荐模式

并发场景下的数据竞争不发生在 `AI` 内部，而可能发生在**共享构建器**上。
推荐各线程在局部变量中构建请求体，经 `SendRequest(method, json)` 发送：

```cpp
// 各线程独立的请求体，互不干扰
nlohmann::json request;
request["model"] = "kimi-k2.6";
request["messages"] = nlohmann::json::array({
	{{"role", "user"}, {"content", "Hello"}}
});
nlohmann::json response = ai.SendRequest_POST(request);
```

多线程交替调用同一实例的 `GetBuilder().SetValue(...)` 后再
`SendRequestFromBuilder_Post()` 属于错误用法：虽然构建器方法内部有锁，
但交替写入仍会互相覆盖请求字段。

**示例**：见 `Demo/DemoCN(ChatDemo-V3-Thread.cpp)`。

---

## 11. 错误处理约定

库内所有可恢复错误统一经 `DoErrorThrow` 按 `SetErrorThrow` 配置的策略处理，
返回值约定如下表。调用方通过检查返回值即可感知失败，
无需依赖异常（除非显式选择 `ALL_AI_EXCEPTION_THROWING`）。

| 返回类型 | 失败值 |
| --- | --- |
| `nlohmann::json` | 空对象 `{}`（可用 `response.is_null() \|\| response.empty()` 判断） |
| `std::string` | 空字符串 |
| `bool` | `false` |
| `FileUploadResult` | `success == false`，`file_id` 为空 |
