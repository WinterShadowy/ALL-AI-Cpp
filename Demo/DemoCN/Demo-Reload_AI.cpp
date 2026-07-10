#include "ALL-AI-V3.hpp"
#include <iostream>

using namespace std;

// 可以将下面的url和api_key替换为你自己的API站的URL和API Key
// 这里使用了Moonshot的API站作为示例，你需要根据你自己的情况进行修改
std::string url_models = "https://api.moonshot.cn/v1/models";
std::string url_chat = "https://api.moonshot.cn/v1/chat/completions";
std::string api_key = "YOUR_API_KEY";

int main()
{
	ALL_AI::AI ai(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
		url_models,
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
	std::cout << ai.SendRequestFromBuilder_Get().dump(2);

	std::cout << "\nReloading AI with new URL...\n" << std::endl;
	if (ai.ReloadAI(url_chat))
	{
		std::cout << "AI reloaded successfully." << std::endl;
	}
	else
	{
		std::cout << "AI reload failed." << std::endl;
		return 1;
	}

	ai.GetBuilder().SetValue("kimi-k2.5", "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM, "You are helpful assistant.");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER, "Introduce Github to me.");
	ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");
	std::cout << ai.SendRequestFromBuilder_Post().dump(2);

	std::cout << "content: " << ai.GetParser().GetValue<std::string>("choices", 0, "message", "content");

	return 0;
}