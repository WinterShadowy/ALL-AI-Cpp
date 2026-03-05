# 快速上手 (Getting Started)

## 安装与引入

将 `include` 目录下的 `ALL-AI-V3.hpp` 包含到你的项目中。

```cpp
#include "ALL-AI-V3.hpp"
```

确保你也安装并链接了 `libcurl` 库。

## 聊天示例 (Chat Demo)

以下是一个使用 AI 进行聊天的基本示例。

```cpp
#include "ALL-AI-V3.hpp"
#include <iostream>

std::string url = "YOUR_URL"; // 替换为 API URL
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

    // 发送 POST 请求并打印结果
    // dump(2) 用于美化输出 JSON
	std::cout << ai.SendRequestFromBuilder_Post().dump(2);

	return 0;
}
```

## 关键步骤解析

1.  **创建 AI 实例**：你需要传入一个 `IHttpTransport` 的实现（如 `CurlHttpTransport`），URL 和 API Key。
2.  **调用** `InitAI()`：初始化 HTTP 客户端。
3.  **构建请求**：使用 `ai.GetBuilder().SetValue()` 设置 JSON 请求体中的字段。
4.  **管理消息**：使用 `ai.GetTools()` 管理聊天历史（Message Array）。
5.  **发送请求**：使用 `ai.SendRequestFromBuilder_Post()` 发送并获取响应。
