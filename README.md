觉得好用不妨点个⭐（star）吧！

# ALL-AI-Cpp

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-14%2F17-blue.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey.svg)]()
[![libcurl](https://img.shields.io/badge/libcurl-%E2%89%A5%207.56.0-orange.svg)](https://curl.se/libcurl/)
[![Header Only](https://img.shields.io/badge/header--only-yes-brightgreen.svg)]()

## A lightweight header-only C++ library for OpenAI-compatible AI APIs

ALL-AI-Cpp provides C++ applications with convenient access to **any
OpenAI-compatible AI provider** (Moonshot/KIMI, SiliconFlow, relay stations,
and others), covering chat, files, speech, images, video, and multimodal
understanding. The library requires only a C++14/17 compiler and
[libcurl](https://curl.se/libcurl/); [nlohmann/json](https://github.com/nlohmann/json)
is bundled. The entire library consists of a single header.

ALL-AI-Cpp 是一个轻量级 Header-Only C++ 库，用于对接任意 OpenAI 兼容站点
（Moonshot/KIMI、硅基流动、各类中转站），覆盖对话、文件、语音、图片、
视频与多模态理解。**中文完整说明见文末折叠区。**

---

<details open>
<summary><b>English (default expanded, click to collapse / 默认展开，点击收起)</b></summary>

## Requirements

The library has minimal requirements:

- A C++14 / C++17 compatible compiler (MSVC VS2022, g++, or Clang; C++17 recommended)
- [libcurl](https://curl.se/libcurl/) **≥ 7.56.0** (the multipart mime API is
  required; refer to [Everything curl](https://everything.curl.dev/get) for the
  development package, e.g. `libcurl4-openssl-dev` on Debian/Ubuntu)

Verified platforms: Windows 11 (MSVC / MSYS2 MinGW64) and Ubuntu 22.04 (g++).

## Current implementation coverage

All known OpenAI-style endpoints fall into one of **three request shapes**, for
each of which the library provides a mechanism-layer entry. Endpoints without a
dedicated wrapper can therefore be accessed directly, with the URL and field
names passed explicitly as documented by the provider:

| Request shape | Mechanism-layer entry | Typical endpoints |
| --- | --- | --- |
| JSON in → JSON out | `SendRequestRaw` (GET/POST) | model list, image generation, video task creation/polling |
| multipart form in → JSON out | `SendMultipartRequest` | speech-to-text, voice upload, file upload |
| JSON in → binary stream out | `SetDataCallback` + `SendRequestRaw` | text-to-speech, large file download |

The following endpoints have been verified against real providers (the KIMI
official site and SiliconFlow; see the 10-item integration suite
[`MainTest-V3.cpp`](Demo/DemoEN/MainTest-V3.cpp)):

| API category | Endpoint | Status | Demo |
| --- | --- | --- | --- |
| Models | List models `GET /v1/models` | ✅ | [`ModelListsDemo-V3.cpp`](Demo/DemoEN/ModelListsDemo-V3.cpp) |
| Chat | Create chat completion | ✅ | [`ChatDemo-V3.cpp`](Demo/DemoEN/ChatDemo-V3.cpp) |
| Chat | Streaming chat (SSE, automatically merged) | ✅ | [`ChatDemo-V3-Stream.cpp`](Demo/DemoEN/ChatDemo-V3-Stream.cpp) |
| Chat | Concurrent requests (thread-safe) | ✅ | [`ChatDemo-V3-Thread.cpp`](Demo/DemoEN/ChatDemo-V3-Thread.cpp) |
| Files | Upload file | ✅ | [`FileDemo-V3.cpp`](Demo/DemoEN/FileDemo-V3.cpp) |
| Files | List / retrieve / content / delete | ✅ | [`FileDemo-V3.cpp`](Demo/DemoEN/FileDemo-V3.cpp) |
| Files | Files-to-chat (document → system message, image → base64) | ✅ | [`FileDemo-V3.cpp`](Demo/DemoEN/FileDemo-V3.cpp) |
| Audio | Create transcription (STT) | ✅ | [`AudioDemo-V3.cpp`](Demo/DemoEN/AudioDemo-V3.cpp) |
| Audio | Create speech (TTS, binary stream) | ✅ | [`AudioDemo-V3.cpp`](Demo/DemoEN/AudioDemo-V3.cpp) |
| Audio | Audio understanding (`audio_url` content part) | ✅ | [`AudioDemo-V3.cpp`](Demo/DemoEN/AudioDemo-V3.cpp) |
| Images | Create image | ✅ | [`ImageDemo-V3.cpp`](Demo/DemoEN/ImageDemo-V3.cpp) |
| Video | Video recognition (file-ID reference) | ✅ | [`MainTest-V3.cpp`](Demo/DemoEN/MainTest-V3.cpp) |
| Video | Video generation (task creation + polling) | ✅ | [`VideoDemo-V3.cpp`](Demo/DemoEN/VideoDemo-V3.cpp) |

Endpoints introduced by a provider in the future can be accessed immediately
through `SendRequestRaw` / `SendMultipartRequest`, without requiring a library
update.

## Design principle

> **Preserve structure, not field names.**

The library owns only what never changes: reliable curl transport and
data-shaping tools. Endpoint URLs, form field names, and response field names —
which vary between providers — are always passed explicitly by the caller. The
library never stores, derives, or maps business endpoints.

## Installation

The library consists of a single header (plus the bundled nlohmann/json). Copy
the [`include/`](include/) directory into your project and include one of the
three header variants; they are identical in functionality and differ only in
comment language:

```cpp
#include "ALL-AI-V3.hpp"      // bilingual comments (main version)
// #include "ALL-AI-V3-CN.hpp"  // Chinese comments only
// #include "ALL-AI-V3-EN.hpp"  // English comments only
```

> `include/ALL-AI.hpp` is the legacy V2 header, retained for existing projects.
> New projects should use V3.

## Usage

### Simple showcase

```cpp
#include "ALL-AI-V3.hpp"
#include <iostream>

std::string url = "YOUR_URL";         // full chat endpoint, e.g. https://api.moonshot.cn/v1/chat/completions
std::string api_key = "YOUR_API_KEY"; // your API key

int main()
{
	// Create the AI instance: transport implementation, URL, API key, error policy
	ALL_AI::AI ai(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
		url, api_key, ALL_AI::ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW);
	if (!ai.InitAI())
	{
		std::cout << "AI initialization failed." << std::endl;
		return 1;
	}

	// Build the request
	ai.GetBuilder().SetValue("gpt-3.5-turbo", "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM, "You are a helpful assistant.");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER, "Introduce GitHub to me");
	ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

	// Send the request and extract the reply with JsonGet
	nlohmann::json response = ai.SendRequestFromBuilder_Post();
	std::string content = ALL_AI::JsonGet<std::string>(response, "choices", 0, "message", "content");
	std::cout << content << std::endl;
	return 0;
}
```

`JsonGet<T>` provides stateless value extraction: an empty value is returned
safely when the path is missing or the type does not match, instead of throwing
`type_error`.

Speech-to-text follows the same pattern — a multipart form whose field names
are given as documented by the provider:

```cpp
nlohmann::json stt = ai.SendMultipartRequest(stt_url, "voice.mp3", "file",
	{ {"model", stt_model} });
std::string text = ALL_AI::JsonGet<std::string>(stt, "text");
```

### Feature highlights

- **File gateway** `ai.Files` — upload / batch upload / files-to-messages /
  list / info / content / delete for OpenAI-compatible `/v1/files` endpoints.
- **Multimodal builder** — `ContentPartBuilder` assembles image, audio, and
  video content parts in a fluent chain (`image_url`, `audio_url` data URIs,
  file-ID references).
- **Request builder** — `JsonRequestBuilder` with complete JSON array
  operations (create, push, insert, delete, query, clear).
- **Pluggable error handling** — silent, print-to-console, callback, or
  exception.
- **Dependency injection** — implement `IHttpTransport` to integrate a custom
  HTTP stack; `CurlHttpTransport` is the built-in default.
- **Thread safety** — locked configuration snapshots and serialized transport
  requests, verified by the multi-threaded demo.
- **Runtime reconfiguration** — `ReloadAI` switches the URL and API key at
  runtime.

## Demo gallery

Each demo is provided in both Chinese ([`Demo/DemoCN/`](Demo/DemoCN/)) and
English ([`Demo/DemoEN/`](Demo/DemoEN/)) versions with identical code. Test
assets reside in `Demo/TestFiles/` (`TestVideo.mp4` is an empty placeholder;
replace it with a real video file before running the video demos). In addition
to the endpoint demos listed above:

| Demo | Description |
| --- | --- |
| [`MainTest-V3.cpp`](Demo/DemoEN/MainTest-V3.cpp) | 10-item integration suite; unconfigured providers are skipped automatically, and failures produce a non-zero exit code (CI-friendly) |
| [`ChatDemo-V3-Builder.cpp`](Demo/DemoEN/ChatDemo-V3-Builder.cpp) | request builder usage variants |
| [`ArrayDemo-V3.cpp`](Demo/DemoEN/ArrayDemo-V3.cpp) | builder JSON array operations |
| [`Demo-Reload_AI.cpp`](Demo/DemoEN/Demo-Reload_AI.cpp) | runtime URL configuration switching |

### Building the demos

**Linux (g++):**

```bash
cd ALL-AI-Cpp
mkdir build && cd build
cmake ..
make
```

**Windows (MSYS2 MinGW64):**

```bash
g++ -std=c++17 -I include Demo/DemoEN/MainTest-V3.cpp -o MainTest.exe -lcurl
```

**Windows (MSVC):** add `include` to the include directories, link
`libcurl.lib`, and define `CURL_STATICLIB` when using the static libcurl.

> Relative paths in the demos (e.g. `../TestFiles/TestAudio.mp3`) are resolved
> against the working directory. Run the executable from within `Demo/DemoCN`
> or `Demo/DemoEN`, or adjust the paths accordingly.

## Documentation

The complete documentation (docsify) is located in
[`ALL-AI-Cpp-Doc/`](ALL-AI-Cpp-Doc/README.md):

- [Getting started](ALL-AI-Cpp-Doc/v3/getting-started.md)
- [API reference](ALL-AI-Cpp-Doc/v3/api/README.md) — [core](ALL-AI-Cpp-Doc/v3/api/core.md) · [JSON tools](ALL-AI-Cpp-Doc/v3/api/json-tools.md) · [transport](ALL-AI-Cpp-Doc/v3/api/transport.md) · [common usage](ALL-AI-Cpp-Doc/v3/api/common-usage.md)
- [Demo walkthrough](ALL-AI-Cpp-Doc/v3/demo-explained.md)
- [Design & architecture](ALL-AI-Cpp-Doc/v3/design-architecture.md)
- [Site adaptation guide](ALL-AI-Cpp-Doc/v3/endpoint-abstraction.md)
- [Changelog](ALL-AI-Cpp-Doc/v3/changelog.md) (current version: v3.2.1)

> **Reading the documentation locally:** the documentation is built with
> [docsify](https://github.com/docsifyjs/docsify/) and must be served over HTTP
> — opening the Markdown files directly from disk will not render the site.
> Serve the documentation directory with any static file server, for example:
>
> ```bash
> # Option 1: docsify CLI (see https://docsify.js.org/#/quickstart)
> npm i docsify-cli -g
> docsify serve ALL-AI-Cpp-Doc
>
> # Option 2: Python's built-in server
> cd ALL-AI-Cpp-Doc && python -m http.server 3000
> ```
>
> Then visit `http://localhost:3000` (docsify CLI defaults to port 3000 as well).

Online manual: [ALL-AI-Cpp 使用手册/开发文档](https://doc.cpluscottage.top/web/#/642380673)

## Contributing & feedback

If this project is useful to you, please consider starring the repository.
Issues and pull requests are welcome.

## License

MIT

</details>

---

<details>
<summary><b>中文版说明（默认收起，点击展开）</b></summary>

## 项目简介

ALL-AI-Cpp 是一个轻量级的 **Header-Only** C++ 库，旨在简化 C++ 与 AI 模型的
交互。整个库仅由一个头文件构成（外加内置的 nlohmann/json），当前 V3 版本
（v3.2.1）已覆盖全部 OpenAI 风格的请求类型。

## 环境要求

本项目仅依赖以下环境：

- 支持 C++14 / C++17 的编译器（MSVC VS2022、g++ 或 Clang，推荐 C++17）
- [libcurl](https://curl.se/libcurl/) **≥ 7.56.0**（multipart mime API 需要；
  开发包安装请参考 [Everything curl](https://everything.curl.dev/get)，
  Debian/Ubuntu 为 `libcurl4-openssl-dev`）

已验证平台：Windows 11（MSVC / MSYS2 MinGW64）、Ubuntu 22.04（g++）。

## 端点覆盖情况

所有已知 OpenAI 风格端点均可归纳为**三种请求形状**，机制层分别提供对应入口。
未提供专门封装的端点，亦可按站点文档显式给出 URL 与字段名直接接入：

| 请求形状 | 机制层入口 | 典型端点 |
| --- | --- | --- |
| json 进，json 出 | `SendRequestRaw`（GET/POST） | 模型列表、图片生成、视频任务创建/轮询 |
| multipart 表单进，json 出 | `SendMultipartRequest` | 语音转写、音色上传、文件上传 |
| json 进，二进制流出 | `SetDataCallback` + `SendRequestRaw` | 语音合成、大文件下载 |

以下端点均已在真实站点联调通过（KIMI 官方站与硅基流动；
见十项集成测试 [`MainTest-V3.cpp`](Demo/DemoCN/MainTest-V3.cpp)）：

| API 分类 | 端点 | 状态 | 示例 |
| --- | --- | --- | --- |
| 模型 | 列出模型 `GET /v1/models` | ✅ | [`ModelListsDemo-V3.cpp`](Demo/DemoCN/ModelListsDemo-V3.cpp) |
| 对话 | 创建对话补全 | ✅ | [`ChatDemo-V3.cpp`](Demo/DemoCN/ChatDemo-V3.cpp) |
| 对话 | 流式对话（SSE，自动合并） | ✅ | [`ChatDemo-V3-Stream.cpp`](Demo/DemoCN/ChatDemo-V3-Stream.cpp) |
| 对话 | 多线程并发请求（线程安全） | ✅ | [`ChatDemo-V3-Thread.cpp`](Demo/DemoCN/ChatDemo-V3-Thread.cpp) |
| 文件 | 上传文件 | ✅ | [`FileDemo-V3.cpp`](Demo/DemoCN/FileDemo-V3.cpp) |
| 文件 | 列表 / 信息 / 内容 / 删除 | ✅ | [`FileDemo-V3.cpp`](Demo/DemoCN/FileDemo-V3.cpp) |
| 文件 | 文件转对话（文档 → system 消息，图片 → base64） | ✅ | [`FileDemo-V3.cpp`](Demo/DemoCN/FileDemo-V3.cpp) |
| 语音 | 语音转写（STT） | ✅ | [`AudioDemo-V3.cpp`](Demo/DemoCN/AudioDemo-V3.cpp) |
| 语音 | 语音合成（TTS，二进制流） | ✅ | [`AudioDemo-V3.cpp`](Demo/DemoCN/AudioDemo-V3.cpp) |
| 语音 | 音频理解（`audio_url` 内容 part） | ✅ | [`AudioDemo-V3.cpp`](Demo/DemoCN/AudioDemo-V3.cpp) |
| 图片 | 文生图 | ✅ | [`ImageDemo-V3.cpp`](Demo/DemoCN/ImageDemo-V3.cpp) |
| 视频 | 视频识别（文件 ID 引用） | ✅ | [`MainTest-V3.cpp`](Demo/DemoCN/MainTest-V3.cpp) |
| 视频 | 视频生成（任务创建 + 轮询） | ✅ | [`VideoDemo-V3.cpp`](Demo/DemoCN/VideoDemo-V3.cpp) |

对于站点后续新增的端点，可通过 `SendRequestRaw` / `SendMultipartRequest`
直接接入，无需等待库版本更新。

## 设计原则

> **沉淀结构，不沉淀字段名。**

库仅负责不变的机制（可靠的 curl 收发与数据加工工具）；端点 URL、表单字段名、
响应字段名等随站点而异的语义，一律由调用方显式传入。库不存储、不推导、
不映射任何业务端点。

## 安装与集成

库本体仅包含一个头文件（外加内置的 nlohmann/json）。将 [`include/`](include/)
目录复制到项目中，并包含三个头文件变体之一即可；三者功能完全一致，
仅注释语言不同：

```cpp
#include "ALL-AI-V3.hpp"      // 中英双语注释版（主版本）
// #include "ALL-AI-V3-CN.hpp"  // 纯中文注释版
// #include "ALL-AI-V3-EN.hpp"  // 纯英文注释版
```

> `include/ALL-AI.hpp` 为旧版 V2 头文件，仅为既有项目保留；新项目请使用 V3。

## 快速开始

```cpp
#include "ALL-AI-V3.hpp"
#include <iostream>

std::string url = "YOUR_URL";         // 完整的聊天端点，如 https://api.moonshot.cn/v1/chat/completions
std::string api_key = "YOUR_API_KEY"; // 你的 API Key

int main()
{
	// 创建 AI 实例：传输层实现、URL、API Key、错误处理策略
	ALL_AI::AI ai(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
		url, api_key, ALL_AI::ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW);
	if (!ai.InitAI())
	{
		std::cout << "AI initialization failed." << std::endl;
		return 1;
	}

	// 构建请求
	ai.GetBuilder().SetValue("gpt-3.5-turbo", "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM, "You are a helpful assistant.");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER, "Introduce GitHub to me");
	ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

	// 发送请求，并用 JsonGet 提取回复
	nlohmann::json response = ai.SendRequestFromBuilder_Post();
	std::string content = ALL_AI::JsonGet<std::string>(response, "choices", 0, "message", "content");
	std::cout << content << std::endl;
	return 0;
}
```

`JsonGet<T>` 提供无状态取值：路径不存在或类型不匹配时安全返回空值，
不会抛出 `type_error` 异常。

语音转写的调用方式相同——multipart 表单字段名按站点文档给出：

```cpp
nlohmann::json stt = ai.SendMultipartRequest(stt_url, "voice.mp3", "file",
	{ {"model", stt_model} });
std::string text = ALL_AI::JsonGet<std::string>(stt, "text");
```

## 功能特性

- **文件网关** `ai.Files`：上传 / 批量上传 / 文件转对话 / 列表 / 信息 /
  内容 / 删除，适配 OpenAI 兼容的 `/v1/files` 端点。
- **多模态构建**：`ContentPartBuilder` 以链式调用组装图片、音频、视频
  content part（`image_url`、`audio_url` data URI、文件 ID 引用）。
- **请求构建器**：`JsonRequestBuilder`，提供完整的 JSON 数组操作
  （创建、尾插、头插、插入、删除、查询、清空）。
- **错误处理策略**：静默 / 打印 / 回调 / 抛出异常，四种可选。
- **依赖注入**：实现 `IHttpTransport` 即可接入自定义 HTTP 传输层；
  内置默认实现 `CurlHttpTransport`。
- **线程安全**：配置读写持锁快照、传输层请求串行化，已经多线程并发示例验证。
- **运行时重配置**：`ReloadAI` 支持在运行期间切换 URL 与 API Key。

## 示例清单

每个示例均提供中英双版（[`Demo/DemoCN/`](Demo/DemoCN/) 与
[`Demo/DemoEN/`](Demo/DemoEN/)），代码完全一致。测试资源位于
`Demo/TestFiles/`（`TestVideo.mp4` 为空占位文件，运行视频示例前请替换为
真实视频）。除上文端点表所列示例外：

| 示例 | 说明 |
| --- | --- |
| [`MainTest-V3.cpp`](Demo/DemoCN/MainTest-V3.cpp) | 十项集成测试；未配置密钥的站点自动跳过，存在失败项时返回非零退出码（便于接入 CI） |
| [`ChatDemo-V3-Builder.cpp`](Demo/DemoCN/ChatDemo-V3-Builder.cpp) | 请求构建器用法对照 |
| [`ArrayDemo-V3.cpp`](Demo/DemoCN/ArrayDemo-V3.cpp) | 构建器 JSON 数组操作 |
| [`Demo-Reload_AI.cpp`](Demo/DemoCN/Demo-Reload_AI.cpp) | 运行期间切换 URL 配置 |

### 编译运行示例

**Linux (g++)：**

```bash
cd ALL-AI-Cpp
mkdir build && cd build
cmake ..
make
```

**Windows (MSYS2 MinGW64)：**

```bash
g++ -std=c++17 -I include Demo/DemoCN/MainTest-V3.cpp -o MainTest.exe -lcurl
```

**Windows (MSVC)**：在项目属性中添加 `include` 包含目录，链接
`libcurl.lib`；使用静态库时需定义 `CURL_STATICLIB`。

> 示例中的相对路径（如 `../TestFiles/TestAudio.mp3`）相对于程序的工作目录。
> 请在 `Demo/DemoCN` 或 `Demo/DemoEN` 目录下运行可执行文件，或相应调整路径。

## 文档

完整文档（docsify）位于 [`ALL-AI-Cpp-Doc/`](ALL-AI-Cpp-Doc/README.md)：

- [快速上手](ALL-AI-Cpp-Doc/v3/getting-started.md)
- [API 参考](ALL-AI-Cpp-Doc/v3/api/README.md)：[核心类](ALL-AI-Cpp-Doc/v3/api/core.md) · [JSON 工具](ALL-AI-Cpp-Doc/v3/api/json-tools.md) · [HTTP 传输](ALL-AI-Cpp-Doc/v3/api/transport.md) · [常见用法](ALL-AI-Cpp-Doc/v3/api/common-usage.md)
- [示例讲解](ALL-AI-Cpp-Doc/v3/demo-explained.md)
- [设计架构](ALL-AI-Cpp-Doc/v3/design-architecture.md)
- [站点适配指南](ALL-AI-Cpp-Doc/v3/endpoint-abstraction.md)
- [更新日志](ALL-AI-Cpp-Doc/v3/changelog.md)（当前版本：v3.2.1）

> **本地阅读文档的说明：** 本文档基于 [docsify](https://github.com/docsifyjs/docsify/)
> 构建，必须通过 HTTP 服务访问——直接双击打开 Markdown 文件无法呈现完整站点。
> 可使用任意静态文件服务器挂载文档目录，例如：
>
> ```bash
> # 方式一：docsify CLI（参见 https://docsify.js.org/#/zh-cn/quickstart）
> npm i docsify-cli -g
> docsify serve ALL-AI-Cpp-Doc
>
> # 方式二：Python 内置服务器
> cd ALL-AI-Cpp-Doc && python -m http.server 3000
> ```
>
> 随后访问 `http://localhost:3000`（docsify CLI 默认端口同为 3000）。

在线文档：[ALL-AI-Cpp 使用手册/开发文档](https://doc.cpluscottage.top/web/#/642380673)

## 支持与反馈

若本项目对您有所帮助，欢迎 Star 支持。Issue 与 Pull Request 均欢迎。

## 开源协议

MIT

</details>
