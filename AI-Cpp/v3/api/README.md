# API 参考 (API Reference)

本文档详细介绍了 ALL-AI-Cpp V3 的 API 接口。

## 模块列表

### [核心类 (Core)](/v3/api/core.md)
包含 `ALL_AI::AI` 类，这是库的主要入口点。

### [JSON 工具 (Json Tools)](/v3/api/json-tools.md)
包含 `JsonRequestBuilder`, `JsonResponceParser` 以及 `JsonOperatorTools`，用于构建和解析 JSON 数据。

### [HTTP 传输 (Http Transport)](/v3/api/transport.md)
包含 `IHttpTransport` 接口及其默认实现 `CurlHttpTransport`，用于自定义底层网络通信。
