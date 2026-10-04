# V3 站点适配指南（Site Adaptation）

> 对应版本：v3.2（2026-10）
> 涉及文件：`include/ALL-AI-V3.hpp` / `ALL-AI-V3-CN.hpp` / `ALL-AI-V3-EN.hpp`，
> 以及 `DemoCN` / `DemoEN` 下的 `FileDemo-V3.cpp`、`AudioDemo-V3.cpp`、`MainTest-V3.cpp`。

本文档说明 v3.2 的站点适配原则与适配方法，面向需要对接不同 API 站点
（官方站或中转站）的使用者，内容包括：

1. URL 一律显式传入的设计原因
2. 机制层三大入口对全部请求形状的覆盖
3. 真实站点差异案例（实测数据）
4. v3.2 变更摘要与迁移对照
5. 扩展方式

---

## 一、URL 一律显式传入的设计原因

v3.2 之前曾短暂存在一套端点推导机制（`ApiEndpoint` 枚举 + 自动推导 + 注册覆盖），
经讨论与审计后整体移除。移除依据：

> 端点映射（功能 → URL）的内容全部来自各站点的官方文档，
> 库既不生产也无法验证这些数据。由库维护映射表，只会随站点调整而过期。

上传行为本身（curl + multipart + json）在各站点间几乎一致，
**长期变化的变量是 URL 与字段名**。因此 v3.2 采用以下约定：

- **库内唯一保存的 URL 是聊天 URL**（构造 / `SetURL` 注入），其余一切 URL 均为调用参数；
- 库不存储、不推导、不映射任何业务端点——端点表属于用户的配置资产
  （建议集中声明在配置区，或经 `ReloadAI` 动态切换）；
- 只有稳定结构（如 `/v1/files/{id}/content` 的 REST 资源路径）沉淀为库代码
  （`ai.Files` 文件网关）；
- 响应数据由调用方处理，库提供取值工具（`JsonGet`）与自定义解析通道
  （`DataCallback`），不为可能的站点变动预设兜底逻辑。

设计原则概括为：**沉淀结构，不沉淀字段名。**

## 二、机制层三大入口：覆盖全部请求形状

对真实站点接口的归纳表明，所有已知端点只有三种请求形状：

```text
┌─────────────────────────────────────────────────────────────────────┐
│  形状 1：json 进，json 出                                             │
│  ai.SendRequestRaw(HttpMethod::POST, url, body) -> std::string       │
│  例：图片生成、视频生成任务创建/轮询、模型列表（GET）                    │
├─────────────────────────────────────────────────────────────────────┤
│  形状 2：multipart 表单进，json 出                                    │
│  ai.SendMultipartRequest(url, file_path, field_name, form_fields)    │
│  例：语音转写(STT)、音色上传、文件上传                                 │
├─────────────────────────────────────────────────────────────────────┤
│  形状 3：json 进，二进制数据流出                                       │
│  ai.SetDataCallback(...) + ai.SendRequestRaw(POST, url, body)        │
│  例：文本转语音(TTS)、大文件下载                                      │
└─────────────────────────────────────────────────────────────────────┘
```

语音类接口曾被视为"一种行为"，但硅基流动的实例表明它是**三种形状不同的请求**
（STT = multipart、音色上传 = multipart 多字段、TTS = json 进二进制出），
其差异仅为字段名——无可沉淀的结构。因此库不提供 `SpeechToText` 之类的
行为接口，统一由机制层入口 + Demo 示例（`AudioDemo-V3.cpp`）覆盖。

## 三、真实站点差异案例（实测数据）

以下案例来自 2026-10 对硅基流动网关的 curl 隔离实测，
是"库不兜底、差异由调用方适配"原则的直接依据。

### 案例 1：音频理解对话的 part 格式

同一模型（`Qwen/Qwen3-Omni-30B-A3B-Instruct`），两种音频 part 写法结果相反：

| 请求变体 | 结果 |
| --- | --- |
| 纯文本（基线） | ✅ 200 |
| `input_audio` part（DashScope 风格） | ❌ 400，code 20029 "Only text and image_url are supported." |
| `audio_url` part（OpenAI 约定，data URI） | ✅ 200，正确转写出音频原文 |
| `audio_url` + `modalities` / `stream` | ✅ 200（字段被容忍，但非必需） |

结论：硅基流动网关接受 OpenAI 约定的 `audio_url`，拒绝上游 DashScope 风格的
`input_audio`（尽管其官方文档仅列出 `image_url`）。库内置的
`ContentPartBuilder::AddAudioBase64` 生成的即为 `audio_url` part，可直接使用；
切换到阿里百炼等上游站点时，须按对应站点文档手工组装 `input_audio` part
（base64 编码可由 `JsonOperatorTools::FileToBase64` 完成）并开启 `stream`。

### 案例 2：TTS 属于形状 3，而非对话

```text
POST /v1/audio/speech     json 进 -> mp3 二进制流出
```

二进制流不是 json，无法经对话链路接收。应使用 `SetDataCallback` 将数据块
交付用户回调（写文件 / 送入播放器 / 自定义解析），**返回数据的解释由回调完成，
库不检查交付内容**。

### 案例 3：视频生成属于任务式接口

```text
POST /v1/video/submit   ->  {"id": "task_xxx"}          （创建任务）
GET  /v1/video/status?task_id=...  ->  {"status": "..."} （轮询直到完成）
```

创建 + 轮询两步、字段名各站点不同。以形状 1 发送两次请求，
按站点文档用 `JsonGet` 取值即可（参考 `MainTest-V3.cpp` 第 10 项，最长等待 300 秒）。

## 四、v3.2 变更摘要与迁移

### 移除的接口（v3.2.0）

| 移除项 | 替代方案 |
| --- | --- |
| `ApiEndpoint` 枚举、端点推导 / 注册机制 | URL 一律经调用参数显式传入 |
| `AI::SetEndpointUrl / SetEndpointSuffix / GetEndpointUrl` | 集中声明端点常量，或经 `ReloadAI` 切换 |
| `AI::SpeechToText`（曾短暂存在） | `SendMultipartRequest`（形状 2） |
| `AI::UploadFile / UploadFiles / FilesToMessages / GetFileList / GetFileInfo / GetFileContent / DeleteFile` | `ai.Files` 文件网关对应成员（`Upload` / `UploadBatch` / `ToMessages` / `List` / `Info` / `Content` / `Delete`） |
| `AI::GetParser`（有状态解析器入口） | 无状态自由函数 `ALL_AI::JsonGet<T>(json, path...)` |
| 构建器 `GetBuilder()`（按值返回 json） | `BuilderToJson()` |

> 保留项：`AI::GetBuilder()`（返回构建器**引用**，高频接口，未弃用）、
> 命名空间级自由函数 `ALL_AI::UploadFile(ai, ...)` 等（对 `ai.Files` 成员的一行转发，
> 供偏好扁平调用风格的代码使用）。

### 迁移对照

```cpp
// 旧（已移除）                          // 新（v3.2）
ai.UploadFile(p, purpose, url);          ai.Files.Upload(p, purpose, url);
ai.UploadFiles(paths, url);              ai.Files.UploadBatch(paths, url);
ai.FilesToMessages(paths, url);          ai.Files.ToMessages(paths, url);
ai.GetFileList(url);                     ai.Files.List(url);
ai.GetFileInfo(id, url);                 ai.Files.Info(id, url);
ai.GetFileContent(id, url);              ai.Files.Content(id, url);
ai.DeleteFile(id, url);                  ai.Files.Delete(id, url);

ai.GetParser().Parse(resp);              std::string s =
ai.GetParser().GetValue<std::string>(     ALL_AI::JsonGet<std::string>(
	"choices", 0, "message", "content");     resp, "choices", 0, "message", "content");
```

## 五、扩展方式

| 需求 | 方法 |
| --- | --- |
| 适配一个新站点 | 在配置区声明该站点的端点常量，经机制层三大入口 + `JsonGet` 直接对接 |
| 站点字段名不同 | 由调用方组装 `form_fields` / 请求体 json，库不预设字段名 |
| 站点返回结构不同 | 按站点文档用 `JsonGet` 取值；二进制 / SSE 使用 `DataCallback` 自定义解析 |
| 某类文件更换处理方式 | 经 `FileStrategyFactory::RegisterStrategy` 注册自定义策略 |
| 更换 HTTP 库 | 实现 `IHttpTransport` 子类，经 `SetHttpTransport` 注入 |
