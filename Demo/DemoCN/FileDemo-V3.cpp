
#include "ALL-AI-V3.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

// 本测试用例由：Kimi K3编写

// windows.h is required to set the console to the UTF-8 code page on Windows
// Note: the DELETE macro defined in windows.h has already been undefined by ALL-AI-V3.hpp,
//       so including it after the header causes no conflict
#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>
#endif

// https://api.moonshot.cn/v1/chat/completions -> https://api.moonshot.cn/v1/files
// 如果你使用的是AI中转站，将下面的URL替换为中转站地址即可
std::string url = "https://api.moonshot.cn/v1/chat/completions";
std::string api_key = "YOUR_API_KEY";

// TestDoc.txt - 一本小说的开头（文本文件）
// TestImage.jpg - 一个galgame的角色图片（图片文件）
// 视频 - 请替换为您需要的视频
const std::string book_file_path = "../TestFiles/TestDoc.txt";
const std::string image_file_path = "../TestFiles/TestImage.jpg";
const std::string video_file_path = "../TestFiles/TestVideo.mp4";

std::string GetFileTextContent(const std::string& raw_content)
{
	try
	{
		nlohmann::json content_json = nlohmann::json::parse(raw_content);
		if (content_json.contains("content") && content_json["content"].is_string())
		{
			return content_json["content"].get<std::string>();
		}
	}
	catch (const nlohmann::json::parse_error&)
	{
		// 解析失败说明响应不是JSON，直接返回原始字符串
	}
	return raw_content;
}

/*
 ============================================================================
 Function: GetFileIdFromResult
 Description: 从上传文件的响应json中安全地提取文件id，
			 请求失败时库返回的是null型空json，直接调用value()会抛出type_error异常，
			 因此这里先判断json类型再提取
 Parameters:
	 - const nlohmann::json& upload_result: UploadFile返回的json
 Return: 提取成功返回文件id，否则返回空字符串
 ============================================================================
*/
std::string GetFileIdFromResult(const nlohmann::json& upload_result)
{
	if (upload_result.is_object() &&
		upload_result.contains("id") &&
		upload_result["id"].is_string())
	{
		return upload_result["id"].get<std::string>();
	}
	return "";
}

/*
 ============================================================================
 Function: CheckFileExists
 Description: 检查本地文件是否存在且可读，不存在时打印诊断信息（当前工作目录与尝试的绝对路径）
 Parameters:
	 - const std::string& file_path: 本地文件路径
 Return: 文件存在且可读返回true，否则返回false
 ============================================================================
*/
bool CheckFileExists(const std::string& file_path)
{
	std::ifstream file_check(file_path, std::ios::binary);
	if (!file_check.good())
	{
		std::cout << "[Error] Cannot open file: " << file_path << std::endl;
		std::cout << "[Error] Current working directory: "
			<< std::filesystem::current_path().string() << std::endl;
		std::cout << "[Error] Please check whether the file path is correct "
			<< "(note: the working directory differs between Visual Studio debugging "
			<< "and running the exe directly)." << std::endl;
		return false;
	}
	return true;
}

/*
 ============================================================================
 Function: GetMessageContent
 Description: 从聊天响应json中安全地提取assistant的消息内容，
			 兼容两种content形式：纯字符串（大多数模型）与数组形式的content parts（部分新模型），
			 content为null（如模型仅输出reasoning_content）时返回空字符串
 Parameters:
	 - const nlohmann::json& chat_result: SendRequestFromBuilder_Post返回的json
 Return: 返回消息内容字符串，提取失败返回空字符串
 ============================================================================
*/
std::string GetMessageContent(const nlohmann::json& chat_result)
{
	try
	{
		if (!chat_result.is_object() ||
			!chat_result.contains("choices") || !chat_result["choices"].is_array() ||
			chat_result["choices"].empty())
		{
			return "";
		}

		const nlohmann::json& message = chat_result["choices"][0]["message"];
		if (!message.contains("content") || message["content"].is_null())
		{
			return "";
		}

		// content为纯字符串（大多数模型）
		if (message["content"].is_string())
		{
			return message["content"].get<std::string>();
		}

		// content为数组形式的content parts（部分新模型），拼接其中的text部分
		if (message["content"].is_array())
		{
			std::string text;
			for (const nlohmann::json& part : message["content"])
			{
				if (part.is_object() && part.contains("text") && part["text"].is_string())
				{
					text += part["text"].get<std::string>();
				}
			}
			return text;
		}
	}
	catch (const nlohmann::json::exception&)
	{
		// 提取失败返回空字符串
	}
	return "";
}

int main()
{
#if (defined(_WIN32) || defined(_WIN64))
	// 将控制台代码页设置为UTF-8，否则服务器返回的UTF-8中文在GBK控制台上会显示为乱码
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif

	// 演示阶段建议使用 ALL_AI_PRINT_ERROR，请求失败时HTTP状态码与服务器错误信息会直接打印到控制台
	ALL_AI::AI ai(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
		url,
		api_key,
		ALL_AI::ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR);
	if (ai.InitAI())
	{
		std::cout << "AI initialized successfully." << std::endl;
	}
	else
	{
		std::cout << "AI initialization failed." << std::endl;
		return 1;
	}

	// 上传前先检查本地文件是否存在（上传失败最常见的原因是相对路径与工作目录不匹配）
	if (!CheckFileExists(book_file_path) || !CheckFileExists(image_file_path))
	{
		return 1;
	}

	// ==================== 第一步：单文件底层流程（文档文件） ====================
	// 演示UploadFile/GetFileContent等底层接口，KIMI的文件上传purpose为"file-extract"
	std::cout << "\n========== Upload Book.txt ==========" << std::endl;
	nlohmann::json book_upload_result = ai.UploadFile(book_file_path, ALL_AI::FileOperator::FilePurpose::FileExtract);
	std::cout << "Upload Result: " << std::endl << book_upload_result.dump(2) << std::endl;

	std::string book_file_id = GetFileIdFromResult(book_upload_result);
	if (book_file_id.empty())
	{
		std::cout << "Upload Book.txt failed, file id is empty." << std::endl;
		std::cout << "Please check: 1. api_key is correct; 2. the API station supports the /v1/files interface." << std::endl;
		return 1;
	}
	std::cout << "Book File ID: " << book_file_id << std::endl;

	// 获取文件解析内容并发起对话（KIMI官方推荐的文件对话方式：文件内容放入system消息）
	std::string book_text = GetFileTextContent(ai.GetFileContent(book_file_id));
	std::cout << "\nBook Content: " << std::endl << book_text << std::endl;

	ai.GetBuilder().SetValue("kimi-k2.5", "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM,
		"你是 Kimi，请阅读用户提供的文件内容并回答问题。文件内容如下：\n" + book_text);
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER,
		"这是哪本小说的开头？请简要赏析这段文字。");
	ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

	std::cout << "\n========== Chat With Book Content ==========" << std::endl;
	nlohmann::json book_chat_result = ai.SendRequestFromBuilder_Post();
	std::cout << "AI Reply: " << std::endl
		<< GetMessageContent(book_chat_result) << std::endl;

	// ==================== 第二步：批量多类型文件高层流程 ====================
	// FilesToMessages内部使用策略模式自动处理不同类型的文件：
	//   文档(Book.txt) -> 上传(file-extract)并抽取内容，生成system消息
	//   图片(ATRI.jpg) -> base64编码为image_url内容part，合并为一条user消息
	// 注意：图片理解需要使用支持视觉的模型（如kimi-k2.5）
	std::cout << "\n========== Chat With Multiple Files ==========" << std::endl;
	std::vector<std::string> file_paths = { book_file_path, image_file_path, video_file_path };
	nlohmann::json file_messages = ai.FilesToMessages(file_paths);
	std::cout << "Files converted to " << file_messages.size() << " messages." << std::endl;

	// 将用户问题追加到文件消息之后，再发起对话
	file_messages.push_back({
		{"role", "user"},
		{"content", "这是哪本小说的开头？另外请描述一下图片中的角色,她是谁？可能来自于什么作品？以及介绍一下视频中的人物，她是哪个角色？"}
		});

	ai.GetBuilder().ClearBuilder();
	ai.GetBuilder().SetValue("kimi-k2.5", "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetBuilder().SetValue(file_messages, "messages");
	//ai.GetBuilder().SetArrayValue("");

	nlohmann::json multi_chat_result = ai.SendRequestFromBuilder_Post();
	std::cout << "AI Reply: " << std::endl
		<< GetMessageContent(multi_chat_result) << std::endl;

	// ==================== 第三步：查看文件列表并清理 ====================
	// 遍历文件列表，删除本次演示上传的所有文件
	std::cout << "\n========== Delete All Files ==========" << std::endl;
	nlohmann::json file_list = ai.GetFileList();
	if (file_list.is_object() && file_list.contains("data") && file_list["data"].is_array())
	{
		for (const nlohmann::json& file_item : file_list["data"])
		{
			std::string file_id = GetFileIdFromResult(file_item);
			if (!file_id.empty())
			{
				std::cout << "Delete " << file_id << ": "
					<< ai.DeleteFile(file_id).dump(2) << std::endl;
			}
		}
	}

	return 0;
}