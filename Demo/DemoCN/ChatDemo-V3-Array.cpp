#include "ALL-AI-V3.hpp"
#include <iostream>

using namespace std;

std::string url = "YOUR_URL";
std::string api_key = "YOUR_API_KEY";

int main()
{
	// 使用构建器示例

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

	ai.GetBuilder().CreateArray("messages", "array");

	// 增加
	for (int i = 0; i < 3; i++)
	{
		ai.GetBuilder().ArrayPushBack(i + 1, "messages", "array");
	}
	for (int i = 0; i < 3; i++)
	{
		ai.GetBuilder().ArrayPushFront(i + 10, "messages", "array");
	}
	ai.GetBuilder().ArrayInsert(1, 66, "messages", "array");
	std::cout << "请求数据(add)\n" << ai.GetBuilder().BuilderToJson().dump(2) << std::endl;

	// 删除
	ai.GetBuilder().ArrayDeleteBack("messages", "array");
	ai.GetBuilder().ArrayDeleteFront("messages", "array");
	ai.GetBuilder().ArrayDelete(2, "messages", "array");
	std::cout << "请求数据(delete)\n" << ai.GetBuilder().BuilderToJson().dump(2) << std::endl;

	// 获取
	std::cout << "获取数组元素(messages): " << ai.GetBuilder().GetArrayFront<int>("messages", "array").value() << std::endl;
	std::cout << "获取数组元素(messages): " << ai.GetBuilder().GetArrayBack<int>("messages", "array").value() << std::endl;
	std::cout << "获取数组元素(messages): " << ai.GetBuilder().GetArrayValue<int>(2, "messages", "array").value() << std::endl;
	std::cout << "获取数组长度(messages): " << ai.GetBuilder().GetArrayLength("messages", "array") << std::endl;

	// 清除
	ai.GetBuilder().ClearArray("messages", "array");
	std::cout << "请求数据(clear)\n" << ai.GetBuilder().BuilderToJson().dump(2) << std::endl;

	return 0;
}