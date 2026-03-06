#include "ALL-AI-V3.hpp"
#include <thread>
#include <chrono>

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

	ai.GetBuilder().SetValue("veo3.1-fast", "model");
	ai.GetBuilder().SetValue("Helicopter takes off.", "prompt");
	std::cout << "Builder Json: " << std::endl << ai.GetBuilderData().dump(2) << std::endl;
	nlohmann::json _out = ai.SendRequestFromBuilder_Post();
	std::cout << _out.dump(2) << std::endl;
	
	std::string _url = url + "/" + _out.at("task_id").get<std::string>();
	if (ai.ReloadAI(_url, api_key, std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>()))
	{
		std::cout << "AI reinitialized successfully." << std::endl;
	}
	else
	{
		std::cout << "AI reinitialization failed." << std::endl;
		return 1;
	}
	// 查询获取结果
	for (int i = 0; i < 400; ++i)
	{
		nlohmann::json _out = ai.SendRequestFromBuilder_Get();
		std::cout << "Get " << i << "\n json: ==> " << _out.dump(2) << std::endl;
		
		std::this_thread::sleep_for(std::chrono::milliseconds(2500));
	}

	return 0;
}
