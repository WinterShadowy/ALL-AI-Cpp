#include "ALL-AI-V3.hpp"
#include <iostream>

using namespace std;

std::string url = "YOUR_URL";
std::string api_key = "YOUR_API_KEY";

void BuilderDemo_1(ALL_AI::AI& ai)
{
	std::cout << "\n\n === BuilderDemo_1 ===\n";
	ai.GetBuilder().SetValue("system", "messages", 0, "role");
	ai.GetBuilder().SetValue("you are a helpful assistant", "messages", 0, "content");
	ai.GetBuilder().SetValue("user", "messages", 1, "role");
	ai.GetBuilder().SetValue("Introduce Github to me. Answer me in English", "messages", 1, "content");
}

void BuilderDemo_2(ALL_AI::AI& ai)
{
	std::cout << "\n\n === BuilderDemo_2 ===\n";
	ai.GetBuilder().ArrayPushBack(nlohmann::json({ {"role", "system"}, {"content", "you are a helpful assistant."} }), "messages");
	ai.GetBuilder().ArrayPushBack(nlohmann::json({ {"role", "user"}, {"content", "Introduce Github to me. Answer me in English"} }), "messages");
}

int main()
{

	// Example of using a builder
	int useBuilderDemo = 1;

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

	// build request
	ai.GetBuilder().SetValue("gpt-4o-mini", "model");
	ai.GetBuilder().SetValue(false, "stream");
    // create an array
    // Before operating on an array, please confirm its existence or create one. 
    // For example, here an array with the field "messages" is created
	ai.GetBuilder().CreateArray("messages");
	
    // Usage of builder extension
	switch (useBuilderDemo)
	{
		case 1:
		{
			BuilderDemo_1(ai);
			break;
		}
		case 2:
		{
			BuilderDemo_2(ai);
			break;
		}
		default:
		{
			std::cerr << "Invalid useBuilderDemo value: " << useBuilderDemo << std::endl;
			return 1;
		}
	}
	
    // print result
	nlohmann::json response = ai.SendRequestFromBuilder_Post();
	std::cout << response.dump(2);

    // print content
	std::cout << "\n\n === content: ===\n" << ALL_AI::JsonGet<std::string>(response, "choices", 0, "message", "content");
	return 0;
}