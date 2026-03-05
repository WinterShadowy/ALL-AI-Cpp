#include "ALL-AI-V3.hpp"

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

	ai.GetBuilder().SetValue("gpt-4o-image", "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetBuilder().SetValue(0.9, "temperature");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER, "a cute dog.");
	ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");
	std::cout << "Builder Json: " << std::endl << ai.GetBuilderData().dump(2) << std::endl;
	std::cout << ai.SendRequestFromBuilder_Post().dump(2);
	return 0;
}