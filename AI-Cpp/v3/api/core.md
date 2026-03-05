# AI 类 (AI Class)

`ALL_AI::AI` 是库的核心类，负责与 AI 服务的交互。它整合了 HTTP 传输、请求构建和响应解析的功能。

## 命名空间 (Namespace)

`ALL_AI`

## 包含头文件

```cpp
#include "ALL-AI-V3.hpp"
```

## 构造函数 (Constructor)

### 默认构造函数
```cpp
explicit AI();
```
创建一个未初始化的 AI 对象。在使用前必须通过 `SetURL`, `SetKey`, `SetHttpTransport` 和 `InitAI` 进行配置。

### 带参构造函数 (主要)
```cpp
explicit AI(std::shared_ptr<IHttpTransport> transport, 
    const std::string& url, 
    const std::string& api_key, 
    const ALL_AI_ErrorThrow all_ai_error_throw = ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW);
```
- **transport**: 一个实现了 `IHttpTransport` 接口的 HTTP 客户端实例 (例如 `CurlHttpTransport`)。
- **url**: API 服务器的 URL (例如 `https://api.openai.com/v1/chat/completions`)。
- **api_key**: API Key。
- **all_ai_error_throw**: 错误处理策略，默认为 `ALL_AI_NO_ERROR_THROW`。

## 初始化与配置 (Initialization & Configuration)

### InitAI
```cpp
bool InitAI();
```
初始化 HTTP 客户端并应用错误处理策略。
- **返回**: 成功返回 `true`，失败返回 `false`。
- **注意**: 在发送请求前必须调用此方法。

### ReloadAI
```cpp
bool ReloadAI(std::string url = "",
    std::string api_key = "", 
    std::shared_ptr<IHttpTransport> transport = std::make_shared<HttpTransport::CurlHttpTransport>());
```
重新加载配置。可以单独更新 URL、API Key 或 Transport。传入空字符串或 nullptr 表示保留当前值。
- **url**: 新的 URL。
- **api_key**: 新的 API Key。
- **transport**: 新的 HTTP 传输实例。

### SetURL
```cpp
void SetURL(const std::string& url);
```
设置或更新 API URL。

### SetKey
```cpp
void SetKey(const std::string& key);
```
设置或更新 API Key。

### SetHttpTransport
```cpp
void SetHttpTransport(std::shared_ptr<IHttpTransport> transport);
```
设置或更新 HTTP 传输层实现。

## 请求发送 (Request Sending)

这些方法会自动将响应数据加载到内部的 `Parser` 中，并返回解析后的 JSON 数据。

### SendRequestFromBuilder_Post
```cpp
nlohmann::json SendRequestFromBuilder_Post();
```
使用当前 `Builder` (`GetBuilder()`) 构建的 JSON 数据发送 POST 请求。
- **返回**: 服务器响应的 `nlohmann::json` 对象。
- **使用场景**: 发送聊天、图片生成等需要 Body 的请求。

### SendRequestFromBuilder_Get
```cpp
nlohmann::json SendRequestFromBuilder_Get();
```
发送 GET 请求。通常 GET 请求不包含 Body，但此方法仍会使用 Builder 状态（通常为空）。
- **返回**: 服务器响应的 `nlohmann::json` 对象。
- **使用场景**: 获取模型列表等。

### SendRequest (Manual)
手动发送请求，**不使用** Builder 中的数据，但会更新 Parser 的状态。此方法系列已被 `SendRequestFromBuilder_*` 内部调用。
```cpp
nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json);
nlohmann::json SendRequest_POST(const nlohmann::json request_json);
nlohmann::json SendRequest_GET(const nlohmann::json request_json);
```
- **method**: `HttpMethod::POST` 或 `HttpMethod::GET`。
- **request_json**: 自定义的 JSON 请求体。

## 获取组件 (Getters)

### GetBuilder
```cpp
JsonOperator::JsonRequestBuilder& GetBuilder();
```
获取请求构建器引用。用于设置请求参数 (如 model, messages, temperature 等)。

### GetBuilderData
```cpp
nlohmann::json GetBuilderData();
```
获取构建器当前已构建的 JSON 对象的副本。

### GetParser
```cpp
JsonOperator::JsonResponceParser& GetParser();
```
获取响应解析器引用。用于在请求后提取响应值。

### GetTools
```cpp
JsonOperatorTools& GetTools();
```
获取 JSON 操作工具引用。用于辅助构建复杂结构 (如消息列表)。

## 错误处理 (Error Handling)

### 枚举：ALL_AI_ErrorThrow
```cpp
enum class ALL_AI_ErrorThrow {
    ALL_AI_PRINT_ERROR,         // (未实现/保留)
    ALL_AI_CALLBACK_FUNCTION,   // 通过回调函数返回错误信息
    ALL_AI_EXCEPTION_THROWING,  // 抛出 standard exception (如 std::runtime_error)
    ALL_AI_NO_ERROR_THROW       // 忽略错误 (默认)
};
```

### SetErrorThrowCallbackFunction
```cpp
bool SetErrorThrowCallbackFunction(std::function<void(const std::string_view& message)> callback_func);
```
设置错误回调函数。
- **前提**: `all_ai_error_throw` 必须设置为 `ALL_AI_CALLBACK_FUNCTION`。
- **参数**: 接受一个 `std::string_view` (错误信息) 的函数或 Lambda。
- **返回**: 设置成功返回 `true`。

## 线程安全与并发特性 (Thread Safety & Concurrency)

V3 版本针对多线程环境进行了特别优化，使 `ALL_AI::AI` 能够在并发场景下安全使用。

### 1. 核心方法的线程安全性

- **请求发送 (`SendRequest` 系列)**: **完全线程安全**。
  - 即使多个线程同时调用同一个 `AI` 实例的 `SendRequest`，库内部会自动序列化这些调用，确保底层网络库 (如 libcurl) 的状态安全。
  - 但请注意，串行化意味着如果一个请求耗时很长，其他线程的请求会等待锁。如果需要极高的并发吞吐量，建议创建多个 `AI` 实例。

- **配置重载 (`ReloadAI`)**: **线程安全且非阻塞**。
  - 您可以在其他线程正在发送请求时安全地调用 `ReloadAI`。
  - 正在进行的请求会继续使用**旧配置**（旧 Transport/URL）直到完成，而新发起的请求将立即使用新配置。

### 2. 高并发最佳实践

在多线程环境下使用单一 `AI` 实例时，需要注意数据竞争不是发生在 `AI` 类内部，而是可能发生在**请求数据的构建**上。

> **推荐模式**: 在局部变量中构建 JSON，通过 `SendRequest(method, json)` 发送。

❌ **错误做法 (竞态条件)**:
如果多个线程共享同一个 `AI` 实例，并尝试修改其内部的 `Builder`：
```cpp
// Thread A
ai.GetBuilder().SetValue("message A", "content");
ai.SendRequestFromBuilder_Post();

// Thread B (同时运行)
ai.GetBuilder().SetValue("message B", "content"); // 可能覆盖 Thread A 的数据！
ai.SendRequestFromBuilder_Post();
```

✅ **正确做法 (局部构建)**:
每个线程维护自己的请求数据，互不干扰：
```cpp
// Thread A
nlohmann::json request_a;
request_a["content"] = "message A";
ai.SendRequest_POST(request_a);

// Thread B
nlohmann::json request_b;
request_b["content"] = "message B";
ai.SendRequest_POST(request_b);
```

### 3. Builder 与 Parser 的安全性

- `GetBuilder()` 和 `GetParser()` 返回的是对象的**引用**。
- 虽然 `AI` 类内部有锁保护引用的获取过程，但**多线程并发修改同一个引用对象的内部状态**仍然需要调用者自行加锁，或者遵循上述的“局部构建”原则。
