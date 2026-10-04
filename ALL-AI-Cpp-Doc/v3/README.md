# ALL-AI-Cpp V3 文档

欢迎使用 ALL-AI-Cpp V3 版本。此版本进行了重构，提供了更灵活的架构和更强大的功能。

主要改进：
- **依赖注入**：支持自定义 HTTP 传输层 (`IHttpTransport`)。
- **构建器模式**：更方便的 JSON 请求构建 (`JsonRequestBuilder`)。
- **无状态解析**：一行取值自由函数 `ALL_AI::JsonGet<T>(json, path...)`，路径不存在时安全返回空值。
- **文件网关**：`ai.Files` 统一提供上传 / 批量上传 / 文件转对话 / 列表 / 信息 / 内容 / 删除。
- **机制层开放**：`SendRequestRaw`（任意 GET/POST）、`SendMultipartRequest`（multipart 表单）、`SetDataCallback`（二进制流回调），适配任意站点端点。
- **多模态构建**：`ContentPartBuilder` 链式组装图片 / 音频 / 视频 content part。
- **错误处理**：支持多种错误处理策略 (打印、回调、抛出异常)。
- **线程安全**：配置读写持锁快照、传输层请求串行化，支持多线程并发使用。

设计原则：**沉淀结构，不沉淀字段名**——库只拥有不变的机制（可靠的 curl 收发 + 数据加工工具），
端点 URL、表单字段、响应字段等易变语义完全交给调用方。

## 文档目录

* [快速上手 (Getting Started)](/v3/getting-started.md)
* [API 参考 (API Reference)](/v3/api/README.md)
  * [核心类 (Core)](/v3/api/core.md)
  * [JSON 工具 (Json Tools)](/v3/api/json-tools.md)
  * [HTTP 传输 (Http Transport)](/v3/api/transport.md)
  * [常见用法 (Common Usage)](/v3/api/common-usage.md)
* [示例 Demo (Demo Examples)](/v3/demo-explained.md)
* [设计架构 (Design Architecture)](/v3/design-architecture.md)
  * [核心架构 (Core Architecture)](/v3/design/core-architecture.md)
  * [文件操作架构 (File Operation Architecture)](/v3/design/file-operation-architecture.md)
* [站点适配指南 (Site Adaptation)](/v3/endpoint-abstraction.md)
* [更新日志 (Changelog)](/v3/changelog.md)
