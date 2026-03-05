# ALL-AI-Cpp V3 文档

欢迎使用 ALL-AI-Cpp V3 版本。此版本进行了重构，提供了更灵活的架构和更强大的功能。

主要改进：
- **依赖注入**：支持自定义 HTTP 传输层 (`IHttpTransport`)。
- **构建器模式**：更方便的 JSON 请求构建 (`JsonRequestBuilder`)。
- **解析器策略**：灵活的 JSON 响应解析 (`JsonResponceParser`)。
- **错误处理**：支持多种错误处理策略 (打印、回调、抛出异常)。
- **工具类**：简化的消息管理 (`JsonOperatorTools`)。

## 文档目录

* [快速上手 (Getting Started)](/v3/getting-started.md)
* [API 参考 (API Reference)](/v3/api/README.md)
  * [核心类 (Core)](/v3/api/core.md)
  * [JSON 工具 (Json Tools)](/v3/api/json-tools.md)
  * [HTTP 传输 (Http Transport)](/v3/api/transport.md)
* [示例 Demo (Demo Examples)](/v3/demo-explained.md)
