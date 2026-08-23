
#include "ALL-AI-V3.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

// This test case was written by: Kimi K3

// windows.h is required to set the console to the UTF-8 code page on Windows
// Note: the DELETE macro defined in windows.h has already been undefined by ALL-AI-V3.hpp,
//       so including it after the header causes no conflict
#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>
#endif

// https://api.moonshot.cn/v1/chat/completions -> https://api.moonshot.cn/v1/files
// If you are using an AI relay station, simply replace the URL below with the relay address
std::string url = "https://api.moonshot.cn/v1/chat/completions";
std::string api_key = "YOUR_API_KEY";

// TestDoc.txt - the opening of a novel (text file)
// TestImage.jpg - a character image from a galgame (image file)
// Video - please replace it with your own video
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
		// Parse failure means the response is not JSON; return the raw string as-is
	}
	return raw_content;
}

/*
 ============================================================================
 Function: GetFileIdFromResult
 Description: Safely extract the file id from an upload response JSON.
			 On failure the library returns a null-type empty JSON, and calling value()
			 on it directly would throw a type_error exception, so the JSON type
			 is checked before extraction
 Parameters:
	 - const nlohmann::json& upload_result: The JSON returned by UploadFile
 Return: Returns the file id on success, or an empty string otherwise
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
 Description: Check whether a local file exists and is readable. If not, print diagnostic
			 information (the current working directory and the attempted path)
 Parameters:
	 - const std::string& file_path: Local file path
 Return: Returns true if the file exists and is readable, false otherwise
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
 Description: Safely extract the assistant's message content from a chat response JSON.
			 Compatible with both content forms: a plain string (most models) and an
			 array of content parts (some newer models). Returns an empty string when
			 content is null (e.g. the model only outputs reasoning_content)
 Parameters:
	 - const nlohmann::json& chat_result: The JSON returned by SendRequestFromBuilder_Post
 Return: Returns the message content string, or an empty string on extraction failure
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

		// content is a plain string (most models)
		if (message["content"].is_string())
		{
			return message["content"].get<std::string>();
		}

		// content is an array of content parts (some newer models); concatenate the text parts
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
		// Return an empty string on extraction failure
	}
	return "";
}

int main()
{
#if (defined(_WIN32) || defined(_WIN64))
	// Set the console code page to UTF-8, otherwise UTF-8 Chinese returned by the server
	// will be displayed as mojibake on a GBK console
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif

	// For demos, ALL_AI_PRINT_ERROR is recommended: when a request fails, the HTTP status
	// code and the server's error message are printed directly to the console
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

	// Check that the local files exist before uploading (the most common cause of upload
	// failure is a mismatch between the relative path and the working directory)
	if (!CheckFileExists(book_file_path) || !CheckFileExists(image_file_path))
	{
		return 1;
	}

	// ==================== Step 1: Low-level flow with a single file (document) ====================
	// Demonstrates low-level APIs such as UploadFile/GetFileContent.
	// The purpose for KIMI file upload is "file-extract"
	std::cout << "\n========== Upload TestDoc.txt ==========" << std::endl;
	nlohmann::json book_upload_result = ai.UploadFile(book_file_path, ALL_AI::FileOperator::FilePurpose::FileExtract);
	std::cout << "Upload Result: " << std::endl << book_upload_result.dump(2) << std::endl;

	std::string book_file_id = GetFileIdFromResult(book_upload_result);
	if (book_file_id.empty())
	{
		std::cout << "Upload TestDoc.txt failed, file id is empty." << std::endl;
		std::cout << "Please check: 1. api_key is correct; 2. the API station supports the /v1/files interface." << std::endl;
		return 1;
	}
	std::cout << "Book File ID: " << book_file_id << std::endl;

	// Get the extracted file content and start a chat
	// (the file-chat approach officially recommended by KIMI: put the file content into a system message)
	std::string book_text = GetFileTextContent(ai.GetFileContent(book_file_id));
	std::cout << "\nBook Content: " << std::endl << book_text << std::endl;

	ai.GetBuilder().SetValue("kimi-k2.5", "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM,
		"You are Kimi. Please read the file content provided by the user and answer questions. File content:\n" + book_text);
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER,
		"Which novel does this opening come from? Please briefly appreciate this passage.");
	ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

	std::cout << "\n========== Chat With Book Content ==========" << std::endl;
	nlohmann::json book_chat_result = ai.SendRequestFromBuilder_Post();
	std::cout << "AI Reply: " << std::endl
		<< GetMessageContent(book_chat_result) << std::endl;

	// ==================== Step 2: High-level flow with multiple file types in batch ====================
	// FilesToMessages internally uses the strategy pattern to handle different file types:
	//   Document (TestDoc.txt)  -> upload (file-extract) and extract content into a system message
	//   Image (TestImage.jpg)   -> base64-encoded into an image_url content part
	//   Video (TestVideo.mp4)   -> upload (purpose=video) and reference by file ID as a video_url part
	//   All media parts are merged into a single user message
	// Note: image/video understanding requires a vision-capable model (e.g. kimi-k2.5)
	std::cout << "\n========== Chat With Multiple Files ==========" << std::endl;
	std::vector<std::string> file_paths = { book_file_path, image_file_path, video_file_path };
	nlohmann::json file_messages = ai.FilesToMessages(file_paths);
	std::cout << "Files converted to " << file_messages.size() << " messages." << std::endl;

	// Append the user question after the file messages, then start the chat
	file_messages.push_back({
		{"role", "user"},
		{"content", "Which novel does this opening come from? Also, please describe the character in the image - who is she and which work might she come from? And introduce the character in the video - who is she?"}
		});

	ai.GetBuilder().ClearBuilder();
	ai.GetBuilder().SetValue("kimi-k2.5", "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetBuilder().SetValue(file_messages, "messages");

	nlohmann::json multi_chat_result = ai.SendRequestFromBuilder_Post();
	std::cout << "AI Reply: " << std::endl
		<< GetMessageContent(multi_chat_result) << std::endl;

	// ==================== Step 3: List files and clean up ====================
	// Iterate over the file list and delete all files uploaded during this demo
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
