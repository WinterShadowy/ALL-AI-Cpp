#include "ALL-AI-V3.hpp"
#include <iostream>

using namespace std;

std::string url = "YOUR_URL";
std::string api_key = "YOUR_API_KEY";

int main()
{
	ALL_AI::AI ai(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
		url,
		api_key,
		ALL_AI::ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW);
	if (ai.InitAI())
	{
		std::cout << "AI initialized successfully." << std::endl;
	}
	else
	{
		std::cout << "AI initialization failed." << std::endl;
		return 1;
	}
	ai.GetBuilder().SetValue("gpt-3.5-turbo", "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM, "You are helpful assistant.");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER, "Introduce Github to me");
	ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");
	std::cout << ai.SendRequestFromBuilder_Post().dump(2);

	// 解析响应 - 从解析器中获取回复的内容
	std::cout << "conetent: " << ai.GetParser().GetValue<std::string>("choices", 0, "message", "content");
	return 0;
}
