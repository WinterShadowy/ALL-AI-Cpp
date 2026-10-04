#include "ALL-AI-V3.hpp"
#include <iostream>

// This test case was written by: Kimi K3

// windows.h is required to set the console to the UTF-8 code page on Windows
// Note: the DELETE macro defined in windows.h has already been undefined by ALL-AI-V3.hpp,
//       so including it after the header causes no conflict
#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>
#endif

// https://api.moonshot.cn/v1/chat/completions
// If you are using an API relay station, replace the URL below with the relay address
std::string url = "https://api.moonshot.cn/v1/chat/completions";
std::string api_key = "YOUR_API_KEY";

// Full URL of the file endpoint (since v3.2 it is always passed explicitly by the caller;
// the library no longer derives it from the chat URL and holds no endpoint mapping)
const std::string FILES_URL = "https://api.moonshot.cn/v1/files";

// TestDoc.txt   - the beginning of a novel (text file)
// TestImage.jpg - a character image from a galgame (image file)
// TestVideo.mp4 - video file (placeholder; replace it with your own video)
// Note: relative paths are resolved against the working directory. When debugging in
//       Visual Studio, the working directory defaults to the project directory rather
//       than the exe directory; check this first if an upload fails
const std::string book_file_path = "../TestFiles/TestDoc.txt";
const std::string image_file_path = "../TestFiles/TestImage.jpg";
const std::string video_file_path = "../TestFiles/TestVideo.mp4";

int main()
{
#if (defined(_WIN32) || defined(_WIN64))
	// Set the console code page to UTF-8; otherwise UTF-8 Chinese text returned by the
	// server is displayed as garbled characters on a GBK console
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif

	// ALL_AI_PRINT_ERROR is recommended during the demo: the HTTP status code and the
	// server error message are printed directly to the console when a request fails
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

	// ==================== Step 1: low-level single-file flow (document) ====================
	// All file operations go through the file gateway ai.Files, with the target URL
	// passed explicitly as a parameter; the file-upload purpose for KIMI is "file-extract"
	std::cout << "\n========== Upload Book.txt ==========" << std::endl;
	nlohmann::json book_upload_result = ai.Files.Upload(book_file_path,
		ALL_AI::FileOperator::FilePurpose::FileExtract,
		FILES_URL);
	std::cout << "Upload Result: " << std::endl << book_upload_result.dump(2) << std::endl;

	// Use the stateless free function JsonGet to read a value safely by path: when the path
	// does not exist or the type does not match, the error is handled according to the
	// configured error mode and an empty value is returned, instead of throwing a
	// type_error like direct subscript access would
	std::string book_file_id = ALL_AI::JsonGet<std::string>(book_upload_result, "id");
	if (book_file_id.empty())
	{
		std::cout << "Upload Book.txt failed, file id is empty." << std::endl;
		std::cout << "Please check: 1. api_key is correct; 2. the API station supports the /v1/files interface; "
			<< "3. the relative file path matches the working directory." << std::endl;
		return 1;
	}
	std::cout << "Book File ID: " << book_file_id << std::endl;

	// Get the parsed file content (KIMI returns a JSON string in the form
	// {"content": "...", ...}); parse it without throwing, then extract the content
	// field with JsonGet
	std::string book_content_raw = ai.Files.Content(book_file_id, FILES_URL);
	nlohmann::json book_content_json = nlohmann::json::parse(book_content_raw, nullptr, false);
	std::string book_text = ALL_AI::JsonGet<std::string>(book_content_json, "content");
	std::cout << "\nBook Content: " << std::endl << book_text << std::endl;

	// Start the chat (KIMI's officially recommended way: put the file content into a
	// system message).
	// The u8 prefix guarantees Chinese literals are always stored as UTF-8: without the
	// /utf-8 option MSVC converts literals to GBK, producing invalid UTF-8 in the
	// request sent to the server
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

	// ==================== Step 2: high-level batch flow with multiple file types ====================
	// Files.ToMessages internally uses the strategy pattern to handle different file types:
	//   document (TestDoc.txt)  -> uploaded (file-extract) and extracted into a system message
	//   image    (TestImage.jpg) -> base64-encoded into an image_url content part,
	//                               merged into a single user message
	// Note: image understanding requires a vision-capable model (e.g. kimi-k2.5)
	std::cout << "\n========== Chat With Multiple Files ==========" << std::endl;
	std::vector<std::string> file_paths = { book_file_path, image_file_path, video_file_path };
	nlohmann::json file_messages = ai.Files.ToMessages(file_paths, FILES_URL);
	std::cout << "Files converted to " << file_messages.size() << " messages." << std::endl;

	// Append the user question after the file messages, then start the chat
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

	// ==================== Step 3: list files and clean up ====================
	// Iterate over the file list and delete every file uploaded during this demo
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
