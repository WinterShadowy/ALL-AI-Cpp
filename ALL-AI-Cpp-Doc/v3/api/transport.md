# HTTP 传输接口 (HTTP Transport)

`ALL_AI::IHttpTransport` 定义了与 AI 服务 API 进行底层通信的抽象接口。库默认提供基于 `libcurl` 的实现。

## 命名空间
`ALL_AI::HttpTransport` (对于具体实现) 或 `ALL_AI` (对于接口)。

## 包含头文件
```cpp
#include "ALL-AI-V3.hpp"
```

## IHttpTransport 接口

所有 HTTP 传输实现必须继承此类。

### 纯虚函数
```cpp
class IHttpTransport {
public:
    virtual bool Initialize(const std::string& url, const std::string& api_key, const ALL_AI_ErrorThrow all_ai_error_throw) = 0;
    virtual nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json) = 0;
    virtual void SetErrorCallbackFunction(std::function<void(const std::string_view& message)> callback_func) = 0;
};
```
- **Initialize**: 初始化传输层，配置基础 URL 和认证信息。
- **SendRequest**: 执行实际网络请求，并返回 JSON 格式响应。
- **SetErrorCallbackFunction**: 设置错误报告回调。

---

## CurlHttpTransport

基于 `libcurl` 库的默认 HTTP 传输实现。

### 构造函数
```cpp
CurlHttpTransport();
```
初始化内部状态。

### Initialize
```cpp
bool Initialize(const std::string& url, const std::string& api_key, const ALL_AI_ErrorThrow all_ai_error_throw) override;
```
配置请求头：
- `Content-Type: application/json`
- `Authorization: Bearer <api_key>`
- 设置 URL 和错误处理模式。

### SendRequest
```cpp
nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json) override;
```
执行同步 HTTP 请求。
- **method**: `HttpMethod::POST` (带 body) 或 `HttpMethod::GET` (通常带 URL 参数，但此处实现可能只是简单转换)。
- **request_json**: 请求体 JSON。
- **返回**: 
  - 成功: 解析后的 JSON 响应。
  - 失败 (连接错误/HTTP错误): 
    - 根据 `all_ai_error_throw` 设置:
      - `ALL_AI_EXCEPTION_THROWING`: 抛出 `std::runtime_error`。
      - `ALL_AI_CALLBACK_FUNCTION`: 调用回调函数，返回空 JSON。
      - `ALL_AI_PRINT_ERROR`: 打印错误至 stderr。
      - `ALL_AI_NO_ERROR_THROW`: 忽略错误，返回空 JSON。

### SetErrorCallbackFunction
```cpp
void SetErrorCallbackFunction(std::function<void(const std::string_view& message)> callback_func) override;
```
设置错误处理回调。仅在 error throw mode 为 `ALL_AI_CALLBACK_FUNCTION` 时有效。

---

## 自定义传输层 (Custom Transport)

如果您需要使用其他网络库 (如 Boost.Beast, CPR 等)，只需继承 `IHttpTransport` 并实现上述三个虚函数。

```cpp
class MyBoostTransport : public ALL_AI::IHttpTransport {
    // ... implement Initialize, SendRequest, SetErrorCallbackFunction
};

// 使用
auto myTransport = std::make_shared<MyBoostTransport>();
ALL_AI::AI myAI(myTransport, "url", "key");
```
