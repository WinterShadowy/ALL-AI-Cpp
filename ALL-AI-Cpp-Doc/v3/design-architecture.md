# V3 设计架构

本页说明 ALL-AI-Cpp V3 的核心设计思路、模块分层，以及 SSE 流返回时建议替换的函数位置。

## 0. 全景 ASCII 架构图

下面这张图对应 V3 的主路径（构建请求 -> 发送 -> 解析 -> 读取）：

```text
+-----------------------------------------------------------------------------------+
|                                   Application Layer                               |
|      你的业务代码 / Demo / CLI / GUI / Server                                    |
|  (GetTools, GetBuilder, SendRequestFromBuilder_Post, GetParser)                  |
+---------------------------------------------+-------------------------------------+
                                              |
                                              v
+-----------------------------------------------------------------------------------+
|                                   Facade Layer                                    |
|                                       AI                                          |
|      InitAI / ReloadAI / SendRequest / SendRequest_POST / SendRequest_GET         |
+-------------------------------+------------------------------+---------------------+
                                |                              |
                                |                              |
                                v                              v
+-------------------------------+----------+      +-----------+---------------------+
|         Request Strategy                 |      |        Response Strategy         |
|          JsonRequestBuilder              |      |         JsonResponceParser       |
|   SetValue / _setValue / GetBuilder      |      |   Parse / GetData / GetValue     |
+-------------------------------+----------+      +-----------+---------------------+
                                \                              /
                                 \                            /
                                  \                          /
                                   v                        v
                         +----------------------------------------+
                         |            Transport Layer             |
                         |            IHttpTransport              |
                         |       (interface abstraction)          |
                         +-------------------+--------------------+
                                             |
                                             v
                         +----------------------------------------+
                         |          CurlHttpTransport             |
                         |  Initialize / SendRequest / callback   |
                         |  WriteCallback / TryParseSseResponse   |
                         +-------------------+--------------------+
                                             |
                                             v
                         +----------------------------------------+
                         |         External AI HTTP API           |
                         |   JSON / SSE(text/event-stream)        |
                         +----------------------------------------+
```

## 1. 架构分层

V3 的整体链路可以理解为四层：

1. 应用层：你的业务代码，负责组装 prompt、处理返回。
2. 门面层：`ALL_AI::AI`，统一暴露初始化、发送请求、读取解析结果等能力。
3. 策略层：`JsonRequestBuilder`、`JsonResponceParser`、`JsonOperatorTools`，负责请求构建和响应解析。
4. 传输层：`IHttpTransport` 抽象接口和 `CurlHttpTransport` 默认实现，负责 HTTP 通信。

## 2. 关键对象协作

一次 POST 请求的大致流程如下：

1. `AI::GetBuilder()` 构建请求 JSON。
2. `AI::SendRequestFromBuilder_Post()` 触发发送。
3. `AI::SendRequest(...)` 调用 `IHttpTransport::SendRequest(...)`。
4. `CurlHttpTransport::SendRequest(...)` 使用 libcurl 发起请求，收集响应。
5. 返回 JSON 给 `JsonResponceParser`，再由 `AI::GetParser()` 提供字段读取。

补充说明：

1. `AI` 是门面对象，尽量把复杂度封装在 transport 和 parser 层。
2. `IHttpTransport` 是可替换点，后续可接入 Boost.Beast、CPR、WinHTTP 等。
3. parser 层是数据契约稳定点，外部业务尽量只依赖 parser 输出结构。

## 3. SSE 流返回支持现状

当前 V3 已兼容以下行为：

1. 当请求体中 `stream=true` 时，传输层会使用 `Accept: text/event-stream`。
2. 若响应不是单个 JSON，而是 SSE 的 `data: ...` 事件流，会自动尝试解析。
3. 解析后会保留 `sse_chunks`（所有流片段），并合并成可直接读取的 `choices[].message.content`。

这意味着你可以继续使用同步接口，只是网络完成后再一次性拿到已合并结果。

## 3.1 SSE 处理链路 ASCII 图

```text
request_json(stream=true)
    |
    v
AI::SendRequest_POST / SendRequestFromBuilder_Post
    |
    v
CurlHttpTransport::SendRequest
    |
    +--> set header: Accept: text/event-stream
    |
    +--> curl_easy_perform (buffer append by WriteCallback)
    |
    +--> try parse whole JSON
        |
        +-- success --> normal JSON path
        |
        +-- fail --> TryParseSseResponse
                   |
                   +--> scan lines: data: {...}
                   +--> parse chunk json list (sse_chunks)
                   +--> merge delta/content -> choices[].message.content
                   +--> return merged json
```

## 4. SSE 相关函数

在请求尝试解析时：
```cpp
try
{
	json_result = nlohmann::json::parse(str_Buffer);
}
```
当处理出现错误时，会在catch语句中尝试解析SSE流，下方的函数是一个简单实现：
```cpp
TryParseSseResponse(const std::string& response, nlohmann::json& json_result)
```
此函数会尝试解析SSE流，并拼凑数据为一个``nlohmann::json``，如果您有其它需求，您可以重构/重载上述函数。
