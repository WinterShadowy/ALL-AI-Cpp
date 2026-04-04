#include "ALL-AI-V3.hpp"
#include <iostream>
#include <string>

using namespace std;

std::string url = "YOUR_URL";
std::string api_key = "YOUR_API_KEY";

int main()
{
	ALL_AI::AI ai(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
		url,
		api_key,
		ALL_AI::ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW);

	if (!ai.InitAI())
	{
		std::cout << "AI initialization failed." << std::endl;
		return 1;
	}

	std::cout << "AI initialized successfully." << std::endl;

	// 设置请求参数
	ai.GetBuilder().SetValue("gpt-3.5-turbo", "model");
	ai.GetBuilder().SetValue(true, "stream");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM, "You are a helpful assistant.");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER, "Introduce Github to me.Answer me in English");
	ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

	// 发送请求并获取响应
	nlohmann::json response = ai.SendRequestFromBuilder_Post();

	// 处理响应
	if (!response.contains("sse_chunks") || !response["sse_chunks"].is_array())
	{
		std::cout << "Server did not return SSE chunks, fallback to merged content:" << std::endl;
		if (response.contains("choices") && response["choices"].is_array() && !response["choices"].empty())
		{
			std::cout << response["choices"][0]["message"]["content"].get<std::string>() << std::endl;
		}
		else
		{
			std::cout << response.dump(2) << std::endl;
		}
		return 0;
	}

	std::cout << "\n===== Stream Output =====\n";
	std::string full_text;

	cout << "json: \n" << response.dump(2) << endl;

	cout << "\n\n===== Merged Content =====\n";

	for (const auto& chunk : response["sse_chunks"])
	{
		if (!chunk.contains("choices") || !chunk["choices"].is_array())
		{
			continue;
		}

		for (const auto& choice : chunk["choices"])
		{
			if (!choice.contains("delta") || !choice["delta"].is_object())
			{
				continue;
			}

			const auto& delta = choice["delta"];
			if (!delta.contains("content") || !delta["content"].is_string())
			{
				continue;
			}

			std::string piece = delta["content"].get<std::string>();
			full_text += piece;
			std::cout << piece << std::flush;
		}
	}

	// Print the final merged content
	std::cout << "\n\n===== Final Merged Content =====\n";
	if (response.contains("choices") && response["choices"].is_array() && !response["choices"].empty())
	{
		std::cout << response["choices"][0]["message"]["content"].get<std::string>() << std::endl;
	}
	else
	{
		std::cout << full_text << std::endl;
	}

	return 0;
}
