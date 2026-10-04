# 快速上手 (Getting Started)

## 安装与引入

将 `include` 目录下的头文件加入项目的包含目录，并在源文件中包含：

```cpp
#include "ALL-AI-V3.hpp"		// 中英双语注释版（主版本）
// #include "ALL-AI-V3-CN.hpp"	// 纯中文注释版
// #include "ALL-AI-V3-EN.hpp"	// 纯英文注释版
```

三个版本功能完全一致，仅注释语言不同。项目须同时安装并链接 `libcurl`
（安装问题见 [常见问题 (FAQ)](/FAQ/README.md)）。

兼容环境：MSVC (VS2022, Windows) 与 g++ (Linux / Windows)，
C++14 与 C++17 均可编译（库内通过 `__ALL_AI_CXX_VERSION` 宏自动适配版本差异）。

## 聊天示例 (Chat Demo)

以下是一个使用 AI 进行聊天的基本示例。

```cpp
#include "ALL-AI-V3.hpp"
#include <iostream>

std::string url = "YOUR_URL"; // 替换为 API URL（完整的聊天端点，如 https://api.moonshot.cn/v1/chat/completions）
std::string api_key = "YOUR_API_KEY"; // 替换为 API Key

int main()
{
    // 实例化 AI 对象
    // 使用 CurlHttpTransport 作为 HTTP 传输层
    // 设置错误处理为 ALL_AI_NO_ERROR_THROW (不抛出错误)
	ALL_AI::AI ai(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
		url,
		api_key,
		ALL_AI::ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW);

    // 初始化 AI
	if (ai.InitAI())
	{
		std::cout << "AI initialized successfully." << std::endl;
	}
	else
	{
		std::cout << "AI initialization failed." << std::endl;
		return 1;
	}

	// 设置请求参数
    // 使用 GetBuilder() 设置字段
	ai.GetBuilder().SetValue("gpt-3.5-turbo", "model");
	ai.GetBuilder().SetValue(false, "stream");

    // 添加消息
    // 使用 GetTools() 辅助构建消息列表
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM, "You are helpful assistant.");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER, "Introduce Github to me");

    // 将消息列表设置到 JSON 中
    ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

    // 发送 POST 请求
	nlohmann::json response = ai.SendRequestFromBuilder_Post();

    // 使用无状态自由函数 JsonGet 一行取值：
    // 路径不存在或类型不匹配时安全返回空字符串，不会抛出 type_error 异常
	std::string content = ALL_AI::JsonGet<std::string>(response, "choices", 0, "message", "content");
	std::cout << "content: " << content << std::endl;

	return 0;
}
```

## 关键步骤解析

1.  **创建 AI 实例**：传入一个 `IHttpTransport` 的实现（如 `CurlHttpTransport`）、URL 和 API Key。
    其中 URL 是**完整的聊天端点**——它是库内唯一保存的 URL，其余一切端点 URL 均为调用参数。
2.  **调用** `InitAI()`：初始化 HTTP 客户端。
3.  **构建请求**：通过 `ai.GetBuilder().SetValue()` 设置 JSON 请求体中的字段。
4.  **管理消息**：通过 `ai.GetTools()` 管理聊天历史（Message Array）。
5.  **发送请求**：调用 `ai.SendRequestFromBuilder_Post()` 发送并获取响应。
6.  **读取结果**：使用 `ALL_AI::JsonGet<T>(response, path...)` 按路径安全取值。

## 下一步

* 上传文件 / 文件对话：见 [API 参考 - 核心类](/v3/api/core.md) 的 **文件网关 `ai.Files`** 一节；
* 语音转写 / 语音合成 / 图片生成等站点端点：见 [常见用法](/v3/api/common-usage.md)；
* 完整可运行的示例：`Demo/DemoCN(MainTest-V3.cpp)` 十项集成测试，覆盖对话、文件、
  语音、图片生成、视频生成，见 [示例 Demo](/v3/demo-explained.md)。
