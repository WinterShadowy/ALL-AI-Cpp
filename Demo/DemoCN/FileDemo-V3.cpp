#include "ALL-AI-V3.hpp"
#include <iostream>

// 本测试用例由：Kimi K3编写

// windows.h is required to set the console to the UTF-8 code page on Windows
// Note: the DELETE macro defined in windows.h has already been undefined by ALL-AI-V3.hpp,
//       so including it after the header causes no conflict
#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>
#endif

// https://api.moonshot.cn/v1/chat/completions
// 如果你使用的是AI中转站，将下面的URL替换为中转站地址即可
std::string url = "https://api.moonshot.cn/v1/chat/completions";
std::string api_key = "YOUR_API_KEY";

// 文件接口URL（自v3.2起由调用方显式传入，库不再从对话URL推导，也不内置任何端点映射）
const std::string FILES_URL = "https://api.moonshot.cn/v1/files";

// TestDoc.txt  - 一本小说的开头（文本文件）
// TestImage.jpg - 一个galgame的角色图片（图片文件）
// TestVideo.mp4 - 视频文件（占位，请替换为你需要的视频）
// 注意：相对路径相对于程序的工作目录，Visual Studio调试时工作目录默认为项目目录而非exe所在目录，
//       上传失败时请先检查该路径是否匹配
const std::string book_file_path = "../TestFiles/TestDoc.txt";
const std::string image_file_path = "../TestFiles/TestImage.jpg";
const std::string video_file_path = "../TestFiles/TestVideo.mp4";

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

	// ==================== 第一步：单文件底层流程（文档文件） ====================
	// 文件操作统一走文件网关 ai.Files，目标URL作为参数显式传入
	// KIMI的文件上传purpose为"file-extract"
	std::cout << "\n========== Upload Book.txt ==========" << std::endl;
	nlohmann::json book_upload_result = ai.Files.Upload(book_file_path,
		ALL_AI::FileOperator::FilePurpose::FileExtract,
		FILES_URL);
	std::cout << "Upload Result: " << std::endl << book_upload_result.dump(2) << std::endl;

	// 使用无状态自由函数JsonGet按路径安全取值：路径不存在或类型不匹配时按错误抛出方式处理
	// 并返回空值，不会像直接下标取值那样抛出type_error异常
	std::string book_file_id = ALL_AI::JsonGet<std::string>(book_upload_result, "id");
	if (book_file_id.empty())
	{
		std::cout << "Upload Book.txt failed, file id is empty." << std::endl;
		std::cout << "Please check: 1. api_key is correct; 2. the API station supports the /v1/files interface; "
			<< "3. the relative file path matches the working directory." << std::endl;
		return 1;
	}
	std::cout << "Book File ID: " << book_file_id << std::endl;

	// 获取文件解析内容（KIMI返回 {"content": "...", ...} 形式的JSON字符串），
	// 先按不抛异常的方式解析为json，再用JsonGet提取content字段
	std::string book_content_raw = ai.Files.Content(book_file_id, FILES_URL);
	nlohmann::json book_content_json = nlohmann::json::parse(book_content_raw, nullptr, false);
	std::string book_text = ALL_AI::JsonGet<std::string>(book_content_json, "content");
	std::cout << "\nBook Content: " << std::endl << book_text << std::endl;

	// 发起对话（KIMI官方推荐的文件对话方式：文件内容放入system消息）
	// u8前缀保证中文字面量始终以UTF-8编码存储：MSVC未开启/utf-8选项时默认把字面量转为GBK编码，
	// 会导致发送给服务器的文本变成非法UTF-8
	ai.GetBuilder().SetValue("kimi-k2.5", "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM,
		u8"你是 Kimi，请阅读用户提供的文件内容并回答问题。文件内容如下：\n" + book_text);
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER,
		u8"这是哪本小说的开头？请简要赏析这段文字。");
	ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

	std::cout << "\n========== Chat With Book Content ==========" << std::endl;
	nlohmann::json book_reply = ai.SendRequestFromBuilder_Post();
	std::cout << "AI Reply: " << std::endl
		<< ALL_AI::JsonGet<std::string>(book_reply, "choices", 0, "message", "content") << std::endl;

	// ==================== 第二步：批量多类型文件高层流程 ====================
	// Files.ToMessages内部使用策略模式自动处理不同类型的文件：
	//   文档(TestDoc.txt) -> 上传(file-extract)并抽取内容，生成system消息
	//   图片(TestImage.jpg) -> base64编码为image_url内容part，合并为一条user消息
	// 注意：图片理解需要使用支持视觉的模型（如kimi-k2.5）
	std::cout << "\n========== Chat With Multiple Files ==========" << std::endl;
	std::vector<std::string> file_paths = { book_file_path, image_file_path, video_file_path };
	nlohmann::json file_messages = ai.Files.ToMessages(file_paths, FILES_URL);
	std::cout << "Files converted to " << file_messages.size() << " messages." << std::endl;

	// 将用户问题追加到文件消息之后，再发起对话
	file_messages.push_back({
		{"role", "user"},
		{"content", u8"这是哪本小说的开头？另外请描述一下图片中的角色,她是谁？可能来自于什么作品？以及介绍一下视频中的人物，她是哪个角色？"}
		});

	ai.GetBuilder().ClearBuilder();
	ai.GetBuilder().SetValue("kimi-k2.5", "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetBuilder().SetValue(file_messages, "messages");

	nlohmann::json files_reply = ai.SendRequestFromBuilder_Post();
	std::cout << "AI Reply: " << std::endl
		<< ALL_AI::JsonGet<std::string>(files_reply, "choices", 0, "message", "content") << std::endl;

	// ==================== 第三步：查看文件列表并清理 ====================
	// 遍历文件列表，删除本次演示上传的所有文件
	std::cout << "\n========== Delete All Files ==========" << std::endl;
	nlohmann::json file_list = ai.Files.List(FILES_URL);
	if (file_list.is_object() && file_list.contains("data") && file_list["data"].is_array())
	{
		for (const nlohmann::json& file_item : file_list["data"])
		{
			std::string file_id = ALL_AI::JsonGet<std::string>(file_item, "id");
			if (!file_id.empty())
			{
				std::cout << "Delete " << file_id << ": "
					<< ai.Files.Delete(file_id, FILES_URL).dump(2) << std::endl;
			}
		}
	}

	return 0;
}
