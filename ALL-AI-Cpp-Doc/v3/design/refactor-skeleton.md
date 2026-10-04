# V3 重构骨架：

## 0. 设计原则

1. **库只拥有"不变的机制"** （可靠的 curl 收发 + 数据加工工具），把"易变的配置与语义"（端点 URL、表单字段、响应字段） **完全** 还给用户——库不存储、不推导、不映射任何业务端点；
2. **沉淀结构，不沉淀字段名**——只有稳定结构（如 `/v1/files/{id}/content`
   的 REST 资源路径）才沉淀为库代码；随站点变化的字段名留给用户与 Demo 菜谱；
3. **无中介**——常用路径不允许出现"先取一个对象再干活"的两段式调用；
4. **解析无状态化**——取值是纯粹的路径导航，不需要对象持有响应；
5. 依赖方向单向：子对象可以调 AI 的发送能力；AI 绝不依赖子对象的内部结构；
6. 改动最小化：对话链路（GetBuilder / GetTools / SendRequestFromBuilder_*）一行不动。

**库内唯一的 URL 是聊天 URL**（`m_url`，`SendRequest` 的发送目标，构造/SetURL 注入）。
其余一切 URL 都是调用参数。

## 0.1 调用形态总览（新写法）

```cpp
ALL_AI::AI ai(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(), url, api_key);
ai.InitAI();

// ---- 对话（与之前完全一致）----
ai.GetBuilder().SetValue("kimi-k2.5", "model");
ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER, "你好");
ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");
auto resp = ai.SendRequestFromBuilder_Post();
std::string text = ALL_AI::JsonGet<std::string>(resp, "choices", 0, "message", "content");

// ---- 端点表是用户自己的资产，库不参与 ----
const std::string FILES_URL = "https://api.moonshot.cn/v1/files";   // 或来自用户的配置文件

// ---- 文件（领域子对象直达，URL 是参数）----
auto up  = ai.Files.Upload("Book.txt", "file-extract", FILES_URL);
std::string fid   = ALL_AI::JsonGet<std::string>(up, "id");
std::string ftext = ai.Files.Content(fid, FILES_URL);
auto msgs  = ai.Files.ToMessages({ "Book.txt", "girl.png" }, FILES_URL);

// ---- 语音/音色/TTS：无网关，用传输层两个通道直接组合（Demo 提供菜谱）----
// STT：multipart，表单字段由用户的站点文档决定
auto stt = ai.SendMultipartRequest(STT_URL, "voice.mp3", "file",
                                   {{"model", "FunAudioLLM/SenseVoiceSmall"}});
std::string stt_text = ALL_AI::JsonGet<std::string>(stt, "text");

// TTS：json POST + 二进制响应走 DataCallback
std::ofstream ofs("out.mp3", std::ios::binary);
ai.SetDataCallback([&](const char* data, size_t size) -> size_t {
	ofs.write(data, size); return size;
});
ai.SendRequestRaw(ALL_AI::HttpMethod::POST, TTS_URL);
ai.ClearDataCallback();
```

用户侧管理自己端点表的推荐写法：

```cpp
// my_endpoints.json：{"files": "https://...", "stt": "https://..."}
const nlohmann::json my_eps = nlohmann::json::parse(ReadAllText("my_endpoints.json"));
ai.Files.Upload("Book.txt", "file-extract", my_eps["files"].get<std::string>());
```

## 1. 所有权与生命周期

```text
+-------------------------- AI (不可拷贝/不可移动, 地址稳定) ----------------------------+
|                                                                                    
|   配置(m_mutex_config):  m_url  m_api_key  m_transport  m_error_throw_method        
|                          ^^^^^^ 库内唯一的URL（聊天发送目标）                         
|                                                                                     
|   售前高频工具(私有成员, 访问器不变):                                                 
|     JsonRequestBuilder  m_builder;    // GetBuilder() / GetBuilderData()            
|     JsonOperatorTools   m_tools;      // GetTools()                                 
|     JsonResponseParser  m_parser;     // 仅供兼容转发 GetParser() 与内部使用          
|                                                                                     
|   领域子对象(公有成员, 构造注入 *this, 无自身状态):                                    
|     FileGateway   Files;        // 文件菜谱（URL 来自调用参数）                       
+--------------------------------------------------------------------------------------+
```

`Files` 为**值成员**，构造时注入 `AI&`，自身**不保存任何 URL**——
每次调用的目标地址由调用方传入。AI 含互斥锁本就不可拷贝，地址稳定，反向引用恒有效。

## 2. AI 类骨架

```cpp
class AI : public ThrowError {
public:
	// ---- 生命周期（不变）----
	explicit AI();
	explicit AI(std::shared_ptr<IHttpTransport> transport,
	            const std::string& url,
	            const std::string& api_key,
	            const ALL_AI_ErrorThrow all_ai_error_throw = ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW);
	~AI();
	AI(const AI&) = delete;
	AI& operator=(const AI&) = delete;

	bool InitAI();                      // 不变
	bool ReloadAI();                    // 语义纯净：只重载连接配置

	// ---- 配置（不变）----
	void SetErrorThrow(ALL_AI_ErrorThrow error_throw);
	void SetURL(const std::string& url);
	void SetKey(const std::string& key);
	void SetHttpTransport(std::shared_ptr<IHttpTransport> transport);

	// ---- 售前高频工具（不变，签名原样保留）----
	JsonOperator::JsonRequestBuilder& GetBuilder();
	nlohmann::json GetBuilderData();
	JsonOperatorTools& GetTools();
	nlohmann::json SendRequestFromBuilder_Get();
	nlohmann::json SendRequestFromBuilder_Post();
	// （待确认）JsonOperator::JsonRequestBuilder CreateBuilder();  // 工厂，每次独立实例

	// ---- 收发（两个通道，不统一入口）----
	nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json);
	nlohmann::json SendRequest_POST(const nlohmann::json request_json);
	nlohmann::json SendRequest_GET(const nlohmann::json request_json);
	std::string    SendRequestRaw(HttpMethod method, const std::string& url);   // 由 private 提升为 public
	nlohmann::json SendMultipartRequest(const std::string& url,
	            const std::string& file_path,                                   // 允许为空（纯字段 multipart）
	            const std::string& file_field_name,
	            const std::unordered_map<std::string, std::string>& form_fields); // 新增公开薄封装

	// ---- 数据回调（新增，见第 4 节）----
	void SetDataCallback(DataCallback callback);   // nullptr 恢复默认（收完转json）
	void ClearDataCallback();

	// ---- 领域子对象（公有值成员，无自身状态）----
	FileGateway   Files;

	// ---- 兼容转发（deprecated，一个版本后删除）----
	JsonOperator::JsonResponseParser& GetParser();      // 返回内部 m_parser
	nlohmann::json UploadFile(const std::string& path, const std::string& purpose,
	            const std::string& files_url);          // 直接转发 Files.Upload(path, purpose, files_url)
	nlohmann::json UploadFile(const std::string& path, FileOperator::FilePurpose purpose,
	            const std::string& files_url);
	std::vector<FileOperator::FileUploadResult> UploadFiles(
	            const std::vector<std::string>& paths, const std::string& files_url);
	nlohmann::json FilesToMessages(const std::vector<std::string>& paths, const std::string& files_url);
	nlohmann::json GetFileList(const std::string& files_url);
	nlohmann::json GetFileInfo(const std::string& file_id, const std::string& files_url);
	std::string    GetFileContent(const std::string& file_id, const std::string& files_url);
	nlohmann::json DeleteFile(const std::string& file_id, const std::string& files_url);
	// 注：SpeechToText 不留转发——其替代写法（SendMultipartRequest + 表单字段）
	//     比原签名更直白，旧调用直接按编译错误迁移即可

private:
	std::string m_url;
	std::string m_api_key;
	std::shared_ptr<IHttpTransport> m_transport;
	std::mutex  m_mutex_config;
	std::mutex  m_mutex_ai_init;

	JsonOperator::JsonRequestBuilder  m_builder;
	JsonOperatorTools                 m_tools;
	JsonOperator::JsonResponseParser  m_parser;   // 仅兼容与内部使用
};
```

注意兼容转发的红利：旧文件接口本来就有 `files_url` 显式参数，
**新签名与之天然对应，转发零损耗、语义零偏移**。唯一的破坏点是
旧签名中 `files_url = ""`（依赖推导）的用法——推导已删，空 URL 将收到
明确的 `DoErrorThrow` 提示（"请传入目标 URL"）。

**从 AI 中彻底移除的**：
`ApiEndpoint` 枚举（保留一个版本兼容）/ `SetEndpointUrl` / `SetEndpointSuffix` / `GetEndpointUrl` /
`MakeEndpointURL` / `MakeFilesURL` / `GetDefaultEndpointSuffix` /
`SendFileRawRequest` / `ParseRawToJson`（逻辑分别并入 `SendRequestRaw` 与解析工具）/
`m_endpoint_urls` / `m_endpoint_suffixes` / `SpeechToText`（降为 Demo 菜谱）。

> **编者注（2026-10-03，最终实施）**：本文是设计讨论稿，其中"保留一个版本兼容"属过渡方案。
> 由于这些弃用接口**从未推送到公开仓库**，最终实施为**全部直接删除、不设兼容期**，
> 与 [更新日志](../changelog.md) v3.2.0/v3.2.1 条目一致。下文对照表中"过渡期处理"一列
> 同理，请以更新日志为准。

## 3. FileGateway 骨架

```cpp
class FileGateway {
public:
	explicit FileGateway(AI& ai);       // 只能由 AI 构造注入

	nlohmann::json Upload(const std::string& file_path,
	            const std::string& purpose,
	            const std::string& url);                 // url 必填，留空 -> DoErrorThrow
	nlohmann::json Upload(const std::string& file_path,
	            FileOperator::FilePurpose purpose,
	            const std::string& url);
	std::vector<FileOperator::FileUploadResult> UploadBatch(
	            const std::vector<std::string>& file_paths,
	            const std::string& url);
	nlohmann::json ToMessages(const std::vector<std::string>& file_paths,
	            const std::string& url);                 // 策略上传统一走此 url
	nlohmann::json List(const std::string& url);
	nlohmann::json Info(const std::string& file_id, const std::string& url);
	std::string    Content(const std::string& file_id, const std::string& url);
	nlohmann::json Delete(const std::string& file_id, const std::string& url);

private:
	AI& m_ai;   // 无其他成员：不存 URL、不存配置
};
```

内部流程（以 `Upload` 为例，两行逻辑）：

```text
url 为空 -> DoErrorThrow("AI: url is empty, please pass the file endpoint url explicitly")
m_ai.SendMultipartRequest(url, path, "file", {purpose=...})
```

`ToMessages` 的连锁调整：策略家族 `Process(AI& ai, path, out_messages, out_parts)`
内部的上传调用改为 `ai.Files.Upload(path, purpose, url)`，`url` 由
`ToMessages` 透传（`IFileProcessStrategy::Process` 属库内接口，允许签名演进）。

## 3.1 无状态解析：JsonGet

把 `JsonResponceParser::GetValue` 的路径导航逻辑抽为自由函数，
**消灭 Parse/GetValue 两步调用**：

```cpp
namespace ALL_AI {
	// 按路径安全取值：路径不存在或类型不匹配时按错误抛出方式处理并返回 T{}
	// 路径键支持字符串（对象键）与整数（数组下标），与 JsonResponceParser::GetValue 一致
	template <typename T, typename... Args>
	T JsonGet(const nlohmann::json& data, Args&&... keys);
}
```

- `JsonResponceParser` 类**保留不动**（兼容旧代码、连续读同一响应的场景仍可用）；
- `JsonGet` 的实现复用解析器内部已有的路径导航（`_getValue`），只是不再要求先 `Parse`；
- FileDemo 的取值代码从两行三步变一行：
  `std::string id = ALL_AI::JsonGet<std::string>(upload_result, "id");`

## 3.2 自由函数拼写（可选便利层，一行转发）

```cpp
namespace ALL_AI {
	nlohmann::json UploadFile(AI& ai, const std::string& path,
	            const std::string& purpose, const std::string& url);
	std::vector<FileOperator::FileUploadResult> UploadFiles(AI& ai,
	            const std::vector<std::string>& paths, const std::string& url);
	nlohmann::json FilesToMessages(AI& ai, const std::vector<std::string>& paths,
	            const std::string& url);
	nlohmann::json GetFileList(AI& ai, const std::string& url);
	nlohmann::json GetFileInfo(AI& ai, const std::string& file_id, const std::string& url);
	std::string    GetFileContent(AI& ai, const std::string& file_id, const std::string& url);
	nlohmann::json DeleteFile(AI& ai, const std::string& file_id, const std::string& url);
}
```

每个函数一行转发到 `ai.Files.*`，两种风格自由混用，无重复实现。

## 4. 传输层新增：数据回调（DataCallback）

为二进制响应（TTS 音频流）、流式数据、大文件下载提供机制（不是语义兜底）：

```cpp
namespace ALL_AI {
	using DataCallback = std::function<size_t(const char* data, size_t size)>;
	// 返回值语义同 libcurl write callback：返回已消费字节数，不等于 size 则中止请求
}

// IHttpTransport 新增虚函数（带默认实现，老子类不破坏）：
virtual std::string SendRequestRaw(HttpMethod method,
            const std::string& url,
            DataCallback callback);   // 默认：忽略 callback，走现有收完返回逻辑
```

`AI::SetDataCallback` 把回调透传给当前 transport；发送时若已设置回调，
响应体逐块交给用户，AI 不再尝试 json 解析。
`SendMultipartRequest` 的 `file_path` 同步放开为可空（纯字段 multipart，如某些音色接口
不带文件的变体）——这是传输层仅有的两处演进，**不引入统一请求入口**。

## 5. 旧接口 → 新接口对照表

| 旧接口 | 去向 | 过渡期处理 |
| --- | --- | --- |
| `GetBuilder()` / `GetBuilderData()` / `GetTools()` / `SendRequestFromBuilder_*` | **不动** | 无 |
| `SendRequest` 系列 / `InitAI` / `ReloadAI` / 配置四件套 | **不动** | 无 |
| `GetParser()`（Parse + GetValue 两步） | `JsonGet<T>(json, path...)` 一行；有状态需求仍可 `ai.GetParser()` | 旧函数保留，标 deprecated |
| `ai.UploadFile(path, purpose, files_url)` | `ai.Files.Upload(path, purpose, files_url)` | 转发，**参数天然对应**；`files_url` 留空（旧推导用法）报明确错误 |
| `ai.UploadFiles(paths, files_url)` | `ai.Files.UploadBatch(paths, files_url)` | 转发 |
| `ai.FilesToMessages(paths)` | `ai.Files.ToMessages(paths, url)` | 旧签名补 url 参数；留空报错 |
| `ai.GetFileList/GetFileInfo/GetFileContent/DeleteFile(..., files_url)` | `ai.Files.List/Info/Content/Delete(..., url)` | 转发 |
| `ai.SpeechToText(path, model)` | **删除**；替代写法：`ai.SendMultipartRequest(url, path, "file", {{"model", m}})`（Demo 菜谱） | 不留转发，编译错误即迁移指引 |
| `SetEndpointUrl / SetEndpointSuffix / GetEndpointUrl` | **删除**（端点概念整体出库） | 保留空实现 + deprecated 注释一个版本 |
| `SendRequestRaw`（private） | 提升为 `AI::SendRequestRaw`（public） | —— |
| `SendFileRawRequest`（private） | 逻辑并入 `AI::SendRequestRaw` | —— |
| `ParseRawToJson`（private） | 并入解析工具（JsonGet 所在实现） | —— |
| `MakeEndpointURL` / `MakeFilesURL` / `GetDefaultEndpointSuffix` | **删除** | —— |
| `ApiEndpoint` 枚举 | **删除** | 保留一个版本兼容旧签名 |
| `m_endpoint_urls` / `m_endpoint_suffixes` | **删除**（端点概念出库） | —— |

**不动的部分**：`JsonRequestBuilder` / `JsonResponseParser` / `JsonOperatorTools` /
`FileOperator`（FileTypeDetector、ContentPartBuilder、FileStrategyFactory）/
`IHttpTransport` 现有三个虚函数 / `ThrowError` / 全部宏与错误策略。
策略家族仅 `Process` 增加 url 透传参数（库内接口演进）。

## 6. 线程安全清单（不劣于现状）

| 对象 | 锁 | 保护内容 |
| --- | --- | --- |
| AI | `m_mutex_config` | m_url / m_api_key / m_transport / 数据回调指针 |
| AI | `m_mutex_ai_init` | InitAI / ReloadAI（先 config 后 init 的顺序不变） |
| CurlHttpTransport | `m_mutex_curl_request` | 单次请求串行（现状不变） |

`FileGateway` **零状态**（只有 `AI&`），天然线程安全；
端点表已出库，其锁随之消失——全库锁数量相比现状**净减少**。
约定不变：**持锁取快照，解锁发请求**；错误一律 `DoErrorThrow`。

## 7. 实施顺序

1. 新增 `FileGateway`（构造注入 AI&、URL 参数化）与 `JsonGet`，AI 增加公有值成员 `Files`
   （此步骤后四个变体 + 双标准编译验证）；
2. AI 的文件/解析旧接口改为转发 + deprecated 注释（显式 url 参数直通）；
3. 传输层演进：`DataCallback` 与 `SendRequestRaw` 回调重载、`SendMultipartRequest`
   的 `file_path` 放开为可空、`AI` 增加 `SetDataCallback` / `ClearDataCallback`；
4. `SendMultipartRequest` 公开薄封装；删除推导三件套与端点双 map、删除 `SpeechToText`
   （`ApiEndpoint` 兼容枚举暂留）；
5. 新增自由函数拼写（一行转发）；
6. 更新 Demo：FileDemo 改新链路（URL 常量在顶部声明）+ `JsonGet`；
   AudioDemo 改为菜谱式（STT 用 SendMultipartRequest、TTS 用 DataCallback）；
   对话 Demo 仅把两步解析换成 `JsonGet`；
7. 一个版本后删除 deprecated 转发与兼容枚举；
8. 同步四个头文件变体；更新文档（架构两页 + changelog + "用户端点表"菜谱）。

## 8. 待确认点

1. 子对象命名：`Files` 是否认可（成员名即领域名，首字母大写与类名风格一致）？
2. `JsonGet` 命名：还是 `JsonValue` / `GetJsonValue`？（自由函数与类方法同名空间，需避歧义）
3. `CreateBuilder()` 工厂方法是否本版本就加（修共享 builder 并发隐患）？
4. deprecated 转发保留几个版本：建议 1 个版本即删，是否接受？
5. 旧 `files_url = ""` 留空用法的处理：直接报错（当前方案），还是保留一个
   deprecated 的"从聊天 URL 推导"私有辅助以缓冲一个版本？（建议直接报错，
   错误信息里写清新用法）
6. `SpeechToText` 不留转发直接删除（当前方案），是否接受？
   （它刚加入一个版本，用户面极小，替代写法比原签名更直白）
