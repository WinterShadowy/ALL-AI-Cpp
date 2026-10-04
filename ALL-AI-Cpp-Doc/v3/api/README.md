# API 参考 (API Reference)

本文档详细介绍了 ALL-AI-Cpp V3 的 API 接口。

## 模块列表

### [核心类 (Core)](/v3/api/core.md)
包含 `ALL_AI::AI` 类（库的主要入口）与文件网关 `ai.Files`（上传 / 管理 / 文件转对话）。

### [JSON 工具 (Json Tools)](/v3/api/json-tools.md)
包含 `JsonRequestBuilder`、`JsonResponseParser`、无状态取值自由函数 `JsonGet`、
辅助工具 `JsonOperatorTools` 与多模态构建器 `ContentPartBuilder`。

### [HTTP 传输 (Http Transport)](/v3/api/transport.md)
包含 `IHttpTransport` 接口及其默认实现 `CurlHttpTransport`：`SendRequest`（JSON）、
`SendRequestRaw`（原始字符串 / 数据回调）、`SendMultipartRequest`（multipart 表单）。

### [常见用法 (Common Usage)](/v3/api/common-usage.md)
聊天、文件上传、语音转写、语音合成、多模态对话、图片/视频生成等常见场景的构建指南。
