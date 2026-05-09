#include "ALL-AI-V3-En.hpp"
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
	ai.GetBuilder().AppendToArray(nlohmann::json({ {"role", "system"}, {"content", "you are a helpful assistant."} }), "messages");
	ai.GetBuilder().AppendToArray(nlohmann::json({ {"role", "user"}, {"content", "Introduce Github to me. Answer me in English"} }), "messages");
}

int main()
{

	// 使用构建器示例
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

    // 构建请求
	ai.GetBuilder().SetValue("gpt-4o-mini", "model");
	ai.GetBuilder().SetValue(false, "stream");
    // 创建一个数组
    // 在操作某个数组前，请确定数组存在，或者是创建一个数组。比如这李创建了字段为“messages”的数组
	ai.GetBuilder().CreateArray("messages");
	
    // 构建器扩展用法
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
	
    // 打印结果
	std::cout << ai.SendRequestFromBuilder_Post().dump(2);

	std::cout << "\n\n === conetent: ===\n" << ai.GetParser().GetValue<std::string>("choices", 0, "message", "content");
	return 0;
}