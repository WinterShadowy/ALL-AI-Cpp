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
    virtual bool Initialize(const std::string& url, const std::string& api_key,
        const ALL_AI_ErrorThrow all_ai_error_throw) = 0;
    virtual nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json) = 0;
    virtual void ClearResource() = 0;
};
```
- **Initialize**: 初始化传输层，配置基础 URL（聊天端点）和认证信息。
- **SendRequest**: 执行 JSON 请求，返回解析后的 JSON 响应。
- **ClearResource**: 释放传输层持有的资源（如 curl 句柄）。

### 带默认实现的虚函数（机制层）

以下三个函数带有"不支持"的默认实现，具体传输类按需覆盖。
它们是**虚函数而非纯虚函数**——已存在的自定义传输类无需修改即可继续编译。

```cpp
// multipart/form-data 表单请求（文件上传、语音转写、音色上传等）
virtual nlohmann::json SendMultipartRequest(const std::string& url,
    const std::string& file_path,
    const std::string& file_field_name,
    const std::unordered_map<std::string, std::string>& form_fields);

// 普通 HTTP 请求，返回原始响应字符串（不做 JSON 解析）
virtual std::string SendRequestRaw(HttpMethod method, const std::string& url);

// 普通 HTTP 请求（可携带 JSON body），通过数据回调逐块交付响应（TTS 等二进制流）
virtual std::string SendRequestRaw(HttpMethod method, const std::string& url,
    const nlohmann::json* body, DataCallback data_callback);
```

- **SendMultipartRequest**: `file_path` 留空表示纯字段 multipart；
  `form_fields` 的字段名完全由调用方按站点文档传入（库不预设字段名）。
- **SendRequestRaw (4 参数)**: `data_callback` 为空时收完响应整体返回字符串；
  非空时响应逐块交给回调（通常返回空字符串，数据归回调处理）。

---

## CurlHttpTransport

基于 `libcurl` 库的默认 HTTP 传输实现（`final` 类），实现了上述全部接口。

### Initialize
```cpp
bool Initialize(const std::string& url, const std::string& api_key,
    const ALL_AI_ErrorThrow all_ai_error_throw) override;
```
配置认证头（`Authorization: Bearer <api_key>`）、基础 URL 和错误处理模式。

### SendRequest
```cpp
nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json) override;
```
执行同步 JSON 请求。
- 请求体 `stream=true` 时自动切换 `Accept: text/event-stream`，
  并在 JSON 解析失败时回退尝试 SSE 解析（`TryParseSseResponse`），
  合并 `choices[].message.content` 并保留 `sse_chunks`；
- 失败时按错误策略处理并返回空 json `{}`。

### 状态清理约定（维护者须知）

easy 句柄是复用的，每个请求函数开头都会清理上一次请求的残留标志：
`CURLOPT_POST / POSTFIELDS / POSTFIELDSIZE / NOBODY / MIMEPOST` 等。
**新增请求路径时必须延续这一约定**——`POSTFIELDSIZE` 曾被显式设置而未重置，
会导致后续请求体被按旧长度截断（详见 [更新日志 2026-10-03](/v3/changelog.md)）。

### 线程模型

所有请求函数持有同一把 `m_mutex_curl_request`，对同一传输实例的并发请求会被串行化。
需要高并发吞吐量时请创建多个 `AI` 实例（各自持有独立传输层）。

---

## 自定义传输层 (Custom Transport)

如果您需要使用其他网络库 (如 Boost.Beast, CPR 等)，只需继承 `IHttpTransport` 并实现接口：

```cpp
class MyBoostTransport : public ALL_AI::IHttpTransport {
    // 必须实现: Initialize / SendRequest / ClearResource
    // 按需覆盖: SendMultipartRequest / SendRequestRaw（两个重载）
};

// 使用
auto myTransport = std::make_shared<MyBoostTransport>();
ALL_AI::AI myAI(myTransport, "url", "key");
```

只实现纯虚函数的传输类可以立即用于对话链路；
文件网关（`ai.Files`）与机制层调用则需要覆盖对应的带默认实现的虚函数。
