# 核心架构（对话主链路）

本页说明 ALL-AI-Cpp V3 的对话主链路设计：一次对话请求如何从业务代码出发，
经过构建、发送、解析，最终变成可读取的结果。

> 文件上传、文件对话、语音等多模态能力见
> [文件操作架构](/v3/design/file-operation-architecture.md)。

## 目录

1. [全景 ASCII 架构图](#1-全景-ascii-架构图)
2. [架构分层](#2-架构分层)
3. [关键对象协作](#3-关键对象协作)
4. [错误处理策略](#4-错误处理策略)
5. [SSE 流返回支持](#5-sse-流返回支持)
6. [维护者指南：常见改动落在哪一层](#6-维护者指南常见改动落在哪一层)

## 1. 全景 ASCII 架构图

下面这张图对应 V3 的主路径（构建请求 -> 发送 -> 一行取值）：

```text
+-----------------------------------------------------------------------------------+
|                                   Application Layer                               |
|      用户业务代码 / Demo / CLI / GUI / Server                                 |
|  (GetTools, GetBuilder, SendRequestFromBuilder_Post, JsonGet)                     |
+---------------------------------------------+-------------------------------------+
                                              |
                                              v
+-----------------------------------------------------------------------------------+
|                                   Facade Layer                                    |
|                                       AI                                          |
|   InitAI / ReloadAI / SendRequest / SendRequestRaw / SendMultipartRequest         |
|   SetDataCallback / Files(文件网关)                                                |
+-------------------------------+------------------------------+---------------------+
                                |                              |
                                |                              |
                                v                              v
+-------------------------------+----------+      +-----------+---------------------+
|         Request Strategy                 |      |        Response Access           |
|          JsonRequestBuilder              |      |   JsonGet（自由函数，无状态）      |
|   SetValue / BuilderToJson / Clear       |      |   JsonResponseParser（底层引擎）   |
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
                         |  SendRequest / SendRequestRaw          |
                         |  SendMultipartRequest                  |
                         |  WriteCallback / TryParseSseResponse   |
                         +-------------------+--------------------+
                                             |
                                             v
                         +----------------------------------------+
                         |         External AI HTTP API           |
                         |   JSON / SSE(text/event-stream)        |
                         +----------------------------------------+
```

## 2. 架构分层

V3 的整体链路可以理解为四层：

1. **应用层**：用户业务代码，负责组装 prompt、处理返回。
2. **门面层**：`ALL_AI::AI`，统一暴露初始化、发送请求、文件网关、数据回调等能力。
3. **策略层**：`JsonRequestBuilder`、`JsonGet` / `JsonResponseParser`、`JsonOperatorTools`，负责请求构建和响应解析。
4. **传输层**：`IHttpTransport` 抽象接口和 `CurlHttpTransport` 默认实现，负责 HTTP 通信。

各层职责速查表：

| 层 | 主要类型 | 职责 | 可替换性 |
| --- | --- | --- | --- |
| 应用层 | 用户代码 | 组装消息、读取结果 | 完全自由 |
| 门面层 | `AI` | 生命周期、配置、发送入口、文件网关、工具取用 | 一般不改 |
| 策略层 | `JsonRequestBuilder` / `JsonGet` / `JsonOperatorTools` | JSON 构建 / 取值 / 消息管理 | 可替换实现 |
| 传输层 | `IHttpTransport` / `CurlHttpTransport` | HTTP 通信 | 依赖注入，可整体替换 |

## 3. 关键对象协作

一次 POST 请求的大致流程如下：

1. `AI::GetBuilder()` 构建请求 JSON。
2. `AI::SendRequestFromBuilder_Post()` 触发发送（内部 `BuilderToJson()` 导出副本）。
3. `AI::SendRequest(...)` 调用 `IHttpTransport::SendRequest(...)`。
4. `CurlHttpTransport::SendRequest(...)` 使用 libcurl 发起请求，收集响应并解析。
5. 调用方用 `ALL_AI::JsonGet<T>(response, path...)` 一行取值。

```text
业务代码
   |
   |  ai.GetTools().PushBack(...)          // 组装 messages
   |  ai.GetBuilder().SetValue(...)        // 构建 model / messages / stream...
   v
AI::SendRequestFromBuilder_Post()
   |
   |  BuilderToJson() -> nlohmann::json
   v
AI::SendRequest(HttpMethod::POST, request_json)
   |
   v
IHttpTransport::SendRequest(method, request_json)
   |
   v
CurlHttpTransport (libcurl)
   |
   v
服务器响应 (json / SSE)
   |
   v
ALL_AI::JsonGet<std::string>(response, "choices", 0, "message", "content")
```

补充说明：

1. `AI` 是门面对象，尽量把复杂度封装在 transport 层。
2. `IHttpTransport` 是可替换点，后续可接入 Boost.Beast、CPR、WinHTTP 等。
3. 取值是**无状态**的：`JsonGet` 是纯粹的路径导航，不持有响应对象；
   响应 json 的所有权始终在调用方手里。

## 4. 错误处理策略

库内所有可恢复错误统一走 `DoErrorThrow(...)`，行为由用户通过
`AI::SetErrorThrow(...)` 配置（打印 / 回调 / 抛出异常 / 静默）。
`SetThrowErrorCallbackFunction` 可注册自定义错误回调。

对维护者的约定：**新增代码不要直接 `throw` 或 `std::cerr`**，
一律调用 `DoErrorThrow`，让用户的错误策略全局生效。

## 5. SSE 流返回支持

当前 V3 已兼容以下行为：

1. 当请求体中 `stream=true` 时，传输层会使用 `Accept: text/event-stream`。
2. 若响应不是单个 JSON，而是 SSE 的 `data: ...` 事件流，会自动尝试解析。
3. 解析后会保留 `sse_chunks`（所有流片段），并合并成可直接读取的 `choices[].message.content`。

这意味着同步接口仍然适用：网络完成后一次性返回已合并结果，
**下游 `JsonGet` 的取值路径与非流式完全一致**。

### 5.1 SSE 处理链路 ASCII 图

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

### 5.2 SSE 相关函数

在请求尝试解析时：

```cpp
try
{
	json_result = nlohmann::json::parse(str_Buffer);
}
```

当处理出现错误时，会在 catch 语句中尝试解析 SSE 流，下方的函数是一个简单实现：

```cpp
TryParseSseResponse(const std::string& response, nlohmann::json& json_result)
```

此函数会尝试解析 SSE 流，并拼凑数据为一个 `nlohmann::json`；如有其它需求，可重构或重载该函数。

## 6. 维护者指南：常见改动落在哪一层

| 需求 | 应修改/使用的位置 |
| --- | --- |
| 更换 HTTP 库（如 CPR / WinHTTP） | 实现 `IHttpTransport` 子类，`AI::SetHttpTransport` 注入 |
| 调整请求体结构 | `JsonRequestBuilder::SetValue` / 应用层组装逻辑 |
| 调整响应读取方式 | `ALL_AI::JsonGet<T>`，或业务层自行解析 json |
| 新增一种错误处理方式 | `DoErrorThrow` 的分支 + `SetErrorThrow` 的枚举 |
| 改 SSE 合并策略 | `CurlHttpTransport::TryParseSseResponse` |
| 文件上传 / 文件对话 / 语音与多模态 | 见 [文件操作架构](/v3/design/file-operation-architecture.md) |
