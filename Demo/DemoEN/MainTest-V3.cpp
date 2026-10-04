#include "../../include/ALL-AI-V3.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <utility>
#include <thread>
#include <chrono>

// Main test program: aggregates the core capabilities of the existing demos,
// runs them one by one and prints a summary report
// This test case was written by: Kimi K3

// windows.h is required to set the console to the UTF-8 code page on Windows
// Note: the DELETE macro defined in windows.h has already been undefined by ALL-AI-V3.hpp,
//       so including it after the header causes no conflict
#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>
#endif

// ==================== Station configuration (replace with your own URL and API Key) ====================
// KIMI (Moonshot) official site: chat, model list, file gateway, video recognition tests
std::string kimi_url = "YOUR_KIMI_URL"; // replace with your own KIMI site URL
std::string kimi_api_key = "YOUR_KIMI_API_KEY"; // replace with your own KIMI API Key
const std::string KIMI_MODELS_URL = "YOUR_KIMI_MODELS_URL"; // replace with your own KIMI models endpoint URL
const std::string KIMI_FILES_URL = "YOUR_KIMI_FILES_URL"; // replace with your own KIMI files endpoint URL
const std::string KIMI_CHAT_MODEL = "YOUR_KIMI_CHAT_MODEL"; // replace with your own KIMI chat model

// SiliconFlow official site: speech and image generation tests
// If you use an API relay station, replace the URLs with the relay address
// (since v3.2 endpoint URLs are always passed in explicitly)
std::string sf_url = "YOUR_SF_URL"; // replace with your own SiliconFlow site URL
std::string sf_api_key = "YOUR_SF_API_KEY"; // replace with your own SiliconFlow API Key
const std::string SF_STT_URL = "YOUR_SF_STT_URL"; // replace with your own SF speech-to-text endpoint URL
const std::string SF_TTS_URL = "YOUR_SF_TTS_URL"; // replace with your own SF text-to-speech endpoint URL
const std::string SF_IMAGE_GEN_URL = "YOUR_SF_IMAGE_GEN_URL"; // replace with your own SF image generation endpoint URL
const std::string SF_STT_MODEL = "YOUR_SF_STT_MODEL"; // replace with your own SF speech-to-text model
const std::string SF_TTS_MODEL = "YOUR_SF_TTS_MODEL"; // replace with your own SF text-to-speech model
const std::string SF_TTS_VOICE = "YOUR_SF_TTS_VOICE"; // replace with your own SF text-to-speech voice
// Audio understanding model (chat completions): must be a chat model that supports
// audio input (currently the Qwen3-Omni series on SiliconFlow).
// Note: ASR speech-to-text models (e.g. SenseVoiceSmall, XingChenAGI/XingChenASR) only
// serve the /v1/audio/transcriptions endpoint; using one here is rejected by the server
// with a 400 parameter error.
// Also verified by testing: the SiliconFlow gateway accepts the OpenAI-style audio_url
// audio part and rejects the DashScope-style input_audio part
const std::string SF_AUDIO_CHAT_MODEL = "YOUR_SF_AUDIO_CHAT_MODEL"; // replace with your own SF audio chat model
const std::string SF_IMAGE_MODEL = "YOUR_SF_IMAGE_MODEL"; // replace with your own SF image generation model

// Video generation station (relay-style task API: POST creates a task and returns
// a task_id, then poll the task status until completion)
// See VideoDemo-V3.cpp: after creating the task, poll taskURL/task_id, waiting up to 300s
std::string gen_url = "YOUR_GEN_URL"; // replace with your own video generation endpoint URL
std::string gen_ask_task = "YOUR_GEN_ASK_TASK_URL"; // replace with your own video generation task-status query URL
std::string gen_api_key = "YOUR_GEN_API_KEY"; // replace with your own video generation API Key
const std::string VIDEO_GEN_MODEL = "YOUR_VIDEO_GEN_MODEL"; // replace with your own video generation model
const int VIDEO_POLL_INTERVAL_MS = 5000;	// polling interval (milliseconds)
const int VIDEO_POLL_TIMEOUT_S = 300;		// maximum polling wait (seconds)

// Test files (note: relative paths are resolved against the working directory; when
// debugging in Visual Studio the working directory defaults to the project directory,
// not the exe directory - check the paths first if a test fails)
const std::string DOC_PATH = "../TestFiles/TestDoc.txt";		// the beginning of a novel
const std::string IMAGE_PATH = "../TestFiles/TestImage.jpg";	// a galgame character image
const std::string AUDIO_PATH = "../TestFiles/TestAudio.mp3";	// a Chinese tongue-twister speech clip
const std::string VIDEO_PATH = "../TestFiles/TestVideo.mp4";	// video file (an empty placeholder in the repo; provide a real video)
const std::string TTS_OUTPUT_PATH = "../TestFiles/TtsOutput.mp3";
const std::string AUDIO_REFERENCE = u8"八百标兵奔北坡，炮兵并排北边跑，炮兵怕把标兵碰，标兵怕碰炮兵炮。";

// ==================== Console styling (UTF-8 + ANSI colors) ====================
static bool g_color_enabled = true;		// Linux/macOS terminals support ANSI colors by default

// Console initialization: on Windows set the UTF-8 code page and try to enable ANSI
// escape sequence support; on failure fall back to plain output without colors,
// staying compatible with legacy consoles
void SetupConsole()
{
#if (defined(_WIN32) || defined(_WIN64))
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode = 0;
	if (hOut == INVALID_HANDLE_VALUE || !GetConsoleMode(hOut, &mode) ||
		!SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
	{
		g_color_enabled = false;
	}
#endif
	return;
}

// Color wrappers: return an empty string when colors are unavailable
const char* CReset() { return g_color_enabled ? "\033[0m" : ""; }
const char* CGreen() { return g_color_enabled ? "\033[32m" : ""; }
const char* CRed() { return g_color_enabled ? "\033[31m" : ""; }
const char* CYellow() { return g_color_enabled ? "\033[33m" : ""; }
const char* CCyan() { return g_color_enabled ? "\033[36m" : ""; }
const char* CGray() { return g_color_enabled ? "\033[90m" : ""; }

// ==================== Test statistics and output ====================
// Test result: PASS / FAIL / SKIP (for SKIP the reason is attached via g_skip_note)
enum TestStatus {
	TEST_PASS = 0,
	TEST_FAIL = 1,
	TEST_SKIP = 2
};

struct TestRecord {
	std::string name;
	int status;		// TestStatus
	std::string note;		// additional info such as the SKIP reason
};

std::vector<TestRecord> g_records;
std::string g_skip_note;		// filled in by a test function before it returns TEST_SKIP

void PrintBanner()
{
	std::cout << CCyan()
		<< u8"╔══════════════════════════════════════════════════════════════╗\n"
		<< u8"║          ALL-AI-V3 Main Test Suite · MainTest                ║\n"
		<< u8"║     Chat / Files / Video / Speech / Image & Video Generation ║\n"
		<< u8"╚══════════════════════════════════════════════════════════════╝"
		<< CReset() << std::endl;
	return;
}

void PrintSection(int index, int total, const std::string& title)
{
	std::cout << CCyan()
		<< u8"\n┌──────────────────────────────────────────────────────────────┐\n"
		<< CReset()
		<< CCyan() << u8"│ " << CReset()
		<< CYellow() << "[" << index << "/" << total << "] " << CReset()
		<< title << "\n"
		<< CCyan()
		<< u8"└──────────────────────────────────────────────────────────────┘"
		<< CReset() << std::endl;
	return;
}

// Test step output (gray, indented)
void PrintStep(const std::string& step)
{
	std::cout << CGray() << u8"  ├─ " << CReset() << step << std::endl;
	return;
}

// Key-value output (indented)
void PrintKV(const std::string& key, const std::string& value)
{
	std::cout << CGray() << u8"  │   " << CReset() << key << ": " << value << std::endl;
	return;
}

void RecordResult(const std::string& name, int status, const std::string& note)
{
	TestRecord record;
	record.name = name;
	record.status = status;
	record.note = note;
	g_records.push_back(record);

	std::cout << CGray() << u8"  └─ " << CReset();
	if (status == TEST_PASS)
	{
		std::cout << CGreen() << u8"✔ PASS" << CReset() << std::endl;
	}
	else if (status == TEST_FAIL)
	{
		std::cout << CRed() << u8"✘ FAIL" << CReset() << std::endl;
	}
	else
	{
		std::cout << CYellow() << u8"− SKIP" << CReset()
			<< CGray() << " (" << note << ")" << CReset() << std::endl;
	}
	return;
}

void PrintSummary()
{
	size_t pass_count = 0;
	size_t fail_count = 0;
	size_t skip_count = 0;

	std::cout << CCyan()
		<< u8"\n╔══════════════════════════════════════════════════════════════╗\n"
		<< u8"║                         Test Summary                         ║\n"
		<< u8"╠══════════════════════════════════════════════════════════════╣"
		<< CReset() << std::endl;

	for (size_t i = 0; i < g_records.size(); ++i)
	{
		const TestRecord& record = g_records[i];
		std::cout << CCyan() << u8"║ " << CReset();
		if (record.status == TEST_PASS)
		{
			++pass_count;
			std::cout << CGreen() << u8"✔" << CReset();
		}
		else if (record.status == TEST_FAIL)
		{
			++fail_count;
			std::cout << CRed() << u8"✘" << CReset();
		}
		else
		{
			++skip_count;
			std::cout << CYellow() << u8"−" << CReset();
		}
		std::cout << " " << record.name;
		if (!record.note.empty())
		{
			std::cout << CGray() << "  (" << record.note << ")" << CReset();
		}
		std::cout << std::endl;
	}

	std::cout << CCyan()
		<< u8"╠══════════════════════════════════════════════════════════════╣\n"
		<< u8"║ " << CReset()
		<< u8"Total: " << g_records.size()
		<< CGreen() << u8"  Passed: " << pass_count << CReset()
		<< CRed() << u8"  Failed: " << fail_count << CReset()
		<< CYellow() << u8"  Skipped: " << skip_count << CReset()
		<< "\n"
		<< CCyan()
		<< u8"╚══════════════════════════════════════════════════════════════╝"
		<< CReset() << std::endl;
	return;
}

// ==================== Small utilities ====================

// Masked display of an API Key
std::string MaskKey(const std::string& key)
{
	if (key.size() > 12)
	{
		return key.substr(0, 6) + "***" + key.substr(key.size() - 4);
	}
	return "***";
}

bool IsKeyConfigured(const std::string& key)
{
	return !key.empty() && key != "YOUR_API_KEY";
}

std::string ToLower(std::string str)
{
	for (size_t i = 0; i < str.size(); ++i)
	{
		str[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(str[i])));
	}
	return str;
}

// Read the size of a local file (-1 on failure)
long GetFileSize(const std::string& file_path)
{
	std::ifstream file(file_path, std::ios::binary | std::ios::ate);
	if (!file.good())
	{
		return -1;
	}
	return static_cast<long>(file.tellg());
}

// Extract the set of file IDs from a file-list json (used to clean up only the
// files added by this test run afterwards)
std::set<std::string> CollectFileIds(const nlohmann::json& file_list)
{
	std::set<std::string> ids;
	if (file_list.is_object() && file_list.contains("data") && file_list["data"].is_array())
	{
		for (const nlohmann::json& item : file_list["data"])
		{
			std::string id = ALL_AI::JsonGet<std::string>(item, "id");
			if (!id.empty())
			{
				ids.insert(id);
			}
		}
	}
	return ids;
}

// ==================== KIMI (Moonshot) tests ====================

// Test 1: basic chat (Builder + tools + JsonGet)
int Test_BasicChat(ALL_AI::AI& ai)
{
	PrintStep(u8"Build request (GetBuilder / GetTools)");
	ai.GetBuilder().ClearBuilder();
	ai.GetBuilder().SetValue(KIMI_CHAT_MODEL, "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM,
		u8"你是一个简洁的助手，始终用一句话回答。");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER,
		u8"用一句话介绍 GitHub。");
	ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

	PrintStep(u8"Send request and parse reply (SendRequestFromBuilder_Post + JsonGet)");
	nlohmann::json reply = ai.SendRequestFromBuilder_Post();
	std::string content = ALL_AI::JsonGet<std::string>(reply, "choices", 0, "message", "content");
	if (content.empty())
	{
		return TEST_FAIL;
	}
	PrintKV(u8"Model reply", content);
	return TEST_PASS;
}

// Test 2: model list (GET usage of the mechanism-layer SendRequestRaw)
int Test_ModelList(ALL_AI::AI& ai)
{
	PrintStep(u8"GET " + KIMI_MODELS_URL);
	std::string raw = ai.SendRequestRaw(ALL_AI::HttpMethod::GET, KIMI_MODELS_URL);
	nlohmann::json model_list = nlohmann::json::parse(raw, nullptr, false);
	if (!model_list.is_object() || !model_list.contains("data") || !model_list["data"].is_array())
	{
		return TEST_FAIL;
	}

	size_t count = model_list["data"].size();
	PrintKV(u8"Available models", std::to_string(count));
	// Show the first 3 model IDs
	for (size_t i = 0; i < count && i < 3; ++i)
	{
		PrintKV(u8"Model", ALL_AI::JsonGet<std::string>(model_list["data"][i], "id"));
	}
	return count > 0 ? TEST_PASS : TEST_FAIL;
}

// Test 3: low-level file gateway flow (upload / content / list / delete)
int Test_FileGateway(ALL_AI::AI& ai)
{
	PrintStep(u8"Upload file (Files.Upload, purpose=file-extract): " + DOC_PATH);
	nlohmann::json upload_result = ai.Files.Upload(DOC_PATH,
		ALL_AI::FileOperator::FilePurpose::FileExtract,
		KIMI_FILES_URL);
	std::string file_id = ALL_AI::JsonGet<std::string>(upload_result, "id");
	if (file_id.empty())
	{
		return TEST_FAIL;
	}
	PrintKV(u8"File ID", file_id);

	PrintStep(u8"Fetch parsed file content (Files.Content)");
	std::string raw_content = ai.Files.Content(file_id, KIMI_FILES_URL);
	nlohmann::json content_json = nlohmann::json::parse(raw_content, nullptr, false);
	std::string text = ALL_AI::JsonGet<std::string>(content_json, "content");
	if (text.empty())
	{
		return TEST_FAIL;
	}
	PrintKV(u8"Content length", std::to_string(text.size()) + u8" chars");

	PrintStep(u8"Query file list (Files.List)");
	std::set<std::string> ids = CollectFileIds(ai.Files.List(KIMI_FILES_URL));
	PrintKV(u8"Current file count", std::to_string(ids.size()));

	PrintStep(u8"Delete the file uploaded in this run (Files.Delete): " + file_id);
	nlohmann::json delete_result = ai.Files.Delete(file_id, KIMI_FILES_URL);
	PrintKV(u8"Delete result", delete_result.is_null() ? u8"(empty response)" : delete_result.dump());
	return TEST_PASS;
}

// Test 4: multi-type files to chat (Files.ToMessages, strategy pattern);
// newly added files are cleaned up after the test
int Test_FilesToMessages(ALL_AI::AI& ai)
{
	// Record the file set before conversion, so only the files added by this
	// run are cleaned up afterwards
	std::set<std::string> before = CollectFileIds(ai.Files.List(KIMI_FILES_URL));

	PrintStep(u8"Convert files to messages (document -> system message, image -> base64 content part)");
	std::vector<std::string> file_paths = { DOC_PATH, IMAGE_PATH };
	nlohmann::json file_messages = ai.Files.ToMessages(file_paths, KIMI_FILES_URL);
	PrintKV(u8"Messages generated", std::to_string(file_messages.size()));

	bool ok = false;
	if (file_messages.size() == 2)
	{
		PrintStep(u8"Append user question and start chat (requires vision model: " + KIMI_CHAT_MODEL + ")");
		file_messages.push_back({
			{"role", "user"},
			{"content", u8"这是哪本小说的开头？另外请用一句话描述图片中的角色。"}
			});

		ai.GetBuilder().ClearBuilder();
		ai.GetBuilder().SetValue(KIMI_CHAT_MODEL, "model");
		ai.GetBuilder().SetValue(false, "stream");
		ai.GetBuilder().SetValue(file_messages, "messages");

		nlohmann::json reply = ai.SendRequestFromBuilder_Post();
		std::string content = ALL_AI::JsonGet<std::string>(reply, "choices", 0, "message", "content");
		if (!content.empty())
		{
			PrintKV(u8"Model reply", content);
			ok = true;
		}
	}

	// Clean up the files added by this test (set difference against the pre-test set)
	PrintStep(u8"Clean up files uploaded in this test");
	std::set<std::string> after = CollectFileIds(ai.Files.List(KIMI_FILES_URL));
	for (const std::string& id : after)
	{
		if (before.find(id) == before.end())
		{
			ai.Files.Delete(id, KIMI_FILES_URL);
			PrintKV(u8"Deleted", id);
		}
	}
	return ok ? TEST_PASS : TEST_FAIL;
}

// Test 5: video recognition (upload video -> reference by file ID -> chat;
// requires the station to support purpose=video and ms:// references)
int Test_VideoRecognition(ALL_AI::AI& ai)
{
	// TestVideo.mp4 in the repo is a 0-byte placeholder; skip this test when no
	// real video is present
	long video_size = GetFileSize(VIDEO_PATH);
	if (video_size <= 0)
	{
		g_skip_note = u8"TestVideo.mp4 is an empty placeholder; please provide a real video";
		return TEST_SKIP;
	}
	PrintKV(u8"Video file", VIDEO_PATH + " (" + std::to_string(video_size) + " bytes)");

	PrintStep(u8"Upload video (Files.Upload, purpose=video)");
	nlohmann::json upload_result = ai.Files.Upload(VIDEO_PATH,
		ALL_AI::FileOperator::FilePurpose::Video,
		KIMI_FILES_URL);
	std::string video_id = ALL_AI::JsonGet<std::string>(upload_result, "id");
	if (video_id.empty())
	{
		return TEST_FAIL;
	}
	PrintKV(u8"File ID", video_id);

	PrintStep(u8"Build video message (ContentPartBuilder.AddVideoFileId) and start chat");
	nlohmann::json user_message = ALL_AI::FileOperator::ContentPartBuilder()
		.AddVideoFileId(video_id)
		.AddText(u8"请描述这段视频的内容。")
		.BuildUserMessage();
	nlohmann::json messages = nlohmann::json::array();
	messages.push_back(user_message);

	ai.GetBuilder().ClearBuilder();
	ai.GetBuilder().SetValue(KIMI_CHAT_MODEL, "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetBuilder().SetValue(messages, "messages");

	nlohmann::json reply = ai.SendRequestFromBuilder_Post();
	std::string content = ALL_AI::JsonGet<std::string>(reply, "choices", 0, "message", "content");

	// Whether the chat succeeds or not, clean up the uploaded video file
	PrintStep(u8"Clean up the uploaded video file");
	ai.Files.Delete(video_id, KIMI_FILES_URL);
	PrintKV(u8"Deleted", video_id);

	if (content.empty())
	{
		return TEST_FAIL;
	}
	PrintKV(u8"Model reply", content);
	return TEST_PASS;
}

// ==================== SiliconFlow tests ====================

// Test 6: speech to text (mechanism-layer SendMultipartRequest: multipart form in, json out)
int Test_SpeechToText(ALL_AI::AI& ai)
{
	PrintStep(u8"POST multipart: " + SF_STT_URL);
	nlohmann::json stt_result = ai.SendMultipartRequest(SF_STT_URL,
		AUDIO_PATH,
		"file",
		{ {"model", SF_STT_MODEL} });
	std::string stt_text = ALL_AI::JsonGet<std::string>(stt_result, "text");
	if (stt_text.empty())
	{
		return TEST_FAIL;
	}
	PrintKV(u8"Recognized text", stt_text);
	PrintKV(u8"Reference text", AUDIO_REFERENCE);
	return TEST_PASS;
}

// Test 7: text to speech (SetDataCallback: the binary stream is written to a
// file chunk by chunk)
int Test_TextToSpeech(ALL_AI::AI& ai)
{
	PrintStep(u8"POST json: " + SF_TTS_URL + u8" (response is an mp3 binary stream)");
	std::ofstream tts_file(TTS_OUTPUT_PATH, std::ios::binary);
	if (!tts_file.good())
	{
		PrintKV(u8"Error", u8"cannot open output file: " + TTS_OUTPUT_PATH);
		return TEST_FAIL;
	}

	ai.SetDataCallback([&tts_file](const char* data, size_t size) -> size_t {
		tts_file.write(data, static_cast<std::streamsize>(size));
		return size;
		});

	nlohmann::json tts_body = {
		{"model", SF_TTS_MODEL},
		{"input", u8"你站在桥上看风景，看风景的人在楼上看你。"},
		{"voice", SF_TTS_VOICE},
		{"response_format", "mp3"}
	};
	ai.SendRequestRaw(ALL_AI::HttpMethod::POST, SF_TTS_URL, tts_body);
	ai.ClearDataCallback();		// always restore the default collect behavior after use
	tts_file.close();

	long output_size = GetFileSize(TTS_OUTPUT_PATH);
	if (output_size <= 0)
	{
		return TEST_FAIL;
	}
	PrintKV(u8"Output file", TTS_OUTPUT_PATH + " (" + std::to_string(output_size) + " bytes)");
	return TEST_PASS;
}

// Test 8: audio understanding chat (ContentPartBuilder multimodal message)
// Verified by isolated curl tests (2026-10): SiliconFlow's chat completions accepts
// the OpenAI-style audio_url content part
// ({"type":"audio_url","audio_url":{"url":"data:audio/mp3;base64,..."}}), while the
// upstream DashScope-style input_audio part is rejected by the gateway with 400
// (code 20029 "Only text and image_url are supported"); stream+modalities is not required.
// The library's ContentPartBuilder.AddAudioBase64 produces exactly an audio_url part,
// so it can be used directly
int Test_AudioChat(ALL_AI::AI& ai)
{
	PrintStep(u8"Build audio multimodal message (ContentPartBuilder.AddAudioBase64)");
	nlohmann::json user_message = ALL_AI::FileOperator::ContentPartBuilder()
		.AddAudioBase64(AUDIO_PATH)
		.AddText(u8"这段音频里说了什么？请直接转写原文，不要输出多余内容。")
		.BuildUserMessage();

	// AddAudioBase64 adds no part when the audio file cannot be read; validate via the part count
	if (!user_message["content"].is_array() || user_message["content"].size() < 2)
	{
		PrintKV(u8"Error", u8"failed to read audio file: " + AUDIO_PATH);
		return TEST_FAIL;
	}

	PrintStep(u8"Send chat request (model: " + SF_AUDIO_CHAT_MODEL + ")");
	nlohmann::json messages = nlohmann::json::array();
	messages.push_back(user_message);
	ai.GetBuilder().ClearBuilder();
	ai.GetBuilder().SetValue(SF_AUDIO_CHAT_MODEL, "model");
	ai.GetBuilder().SetValue(messages, "messages");

	nlohmann::json reply = ai.SendRequestFromBuilder_Post();
	std::string content = ALL_AI::JsonGet<std::string>(reply, "choices", 0, "message", "content");
	if (content.empty())
	{
		return TEST_FAIL;
	}
	PrintKV(u8"Model reply", content);
	PrintKV(u8"Reference text", AUDIO_REFERENCE);
	return TEST_PASS;
}

// Test 9: image generation (mechanism-layer SendRequestRaw: POST json in, json out;
// compatible with both the data and images response shapes)
int Test_ImageGeneration(ALL_AI::AI& ai)
{
	const std::string prompt = u8"一只在月光下的樱花道上奔跑的柴犬，吉卜力动画风格";

	PrintStep(u8"POST json: " + SF_IMAGE_GEN_URL + u8" (model: " + SF_IMAGE_MODEL + ")");
	PrintKV(u8"Prompt", prompt);
	nlohmann::json image_body = {
		{"model", SF_IMAGE_MODEL},
		{"prompt", prompt},
		{"image_size", "1024x1024"},
		{"batch_size", 1}
	};
	std::string raw = ai.SendRequestRaw(ALL_AI::HttpMethod::POST, SF_IMAGE_GEN_URL, image_body);
	nlohmann::json resp = nlohmann::json::parse(raw, nullptr, false);
	if (!resp.is_object())
	{
		return TEST_FAIL;
	}

	// Compatible with two response shapes: OpenAI style data[0].url / data[0].b64_json,
	// SiliconFlow style images[0].url (check key existence before reading, so JsonGet
	// does not print distracting error output for missing paths)
	std::string image_url;
	if (resp.contains("data") && resp["data"].is_array() && !resp["data"].empty())
	{
		image_url = ALL_AI::JsonGet<std::string>(resp, "data", 0, "url");
	}
	if (image_url.empty() && resp.contains("images") && resp["images"].is_array() && !resp["images"].empty())
	{
		image_url = ALL_AI::JsonGet<std::string>(resp, "images", 0, "url");
	}
	if (!image_url.empty())
	{
		PrintKV(u8"Image URL", image_url);
	}
	else
	{
		std::string b64_json;
		if (resp.contains("data") && resp["data"].is_array() && !resp["data"].empty())
		{
			b64_json = ALL_AI::JsonGet<std::string>(resp, "data", 0, "b64_json");
		}
		if (b64_json.empty())
		{
			PrintKV(u8"Raw response", resp.dump());
			return TEST_FAIL;
		}
		PrintKV(u8"Image data", u8"b64_json (" + std::to_string(b64_json.size()) + u8" chars)");
	}

	// Optional fields are extracted only when present (avoids JsonGet error spam
	// for missing paths)
	if (resp.contains("seed"))
	{
		PrintKV(u8"Seed", resp["seed"].dump());
	}
	return TEST_PASS;
}

// ==================== Video generation station (relay) tests ====================

// Test 10: video generation (task-style API: POST creates a task, then poll until
// completion, waiting up to 300 seconds)
int Test_VideoGeneration(ALL_AI::AI& ai)
{
	const std::string prompt = "一只在月光下的樱花道上奔跑的柴犬，吉卜力动画风格.";

	PrintStep(u8"POST json: " + gen_url + u8" (model: " + VIDEO_GEN_MODEL + ")");
	PrintKV(u8"Prompt", prompt);
	nlohmann::json create_body = {
		{"model", VIDEO_GEN_MODEL},
		{"prompt", prompt}
	};
	std::string raw = ai.SendRequestRaw(ALL_AI::HttpMethod::POST, gen_url, create_body);
	nlohmann::json create_resp = nlohmann::json::parse(raw, nullptr, false);
	if (!create_resp.is_object() || !create_resp.contains("requestId"))
	{
		PrintKV(u8"Raw response", raw);
		return TEST_FAIL;
	}

	std::string task_id = ALL_AI::JsonGet<std::string>(create_resp, "requestId");
	PrintKV(u8"Task ID", task_id);

	const std::string query_url = gen_ask_task;
	PrintStep(u8"Start polling task status (every " + std::to_string(VIDEO_POLL_INTERVAL_MS / 1000)
		+ u8"s, up to " + std::to_string(VIDEO_POLL_TIMEOUT_S) + u8"s)");

	int waited = 0;
	while (waited <= VIDEO_POLL_TIMEOUT_S)
	{
		std::string task_raw = ai.SendRequestRaw(ALL_AI::HttpMethod::POST, query_url, create_resp);
		nlohmann::json task = nlohmann::json::parse(task_raw, nullptr, false);
		if (!task.is_object())
		{
			PrintKV(u8"Poll response", task_raw);
			return TEST_FAIL;
		}

		// The status field name is station-specific; when there is no status field,
		// show the raw response for troubleshooting
		std::string status;
		if (task.contains("status") && task["status"].is_string())
		{
			status = ToLower(task["status"].get<std::string>());
		}
		PrintKV(u8"Waited " + std::to_string(waited) + "s",
			status.empty() ? task.dump() : status);

		if (status == "succeeded" || status == "success" || status == "completed" ||
			status == "complete" || status == "done" || status == "finished" || status == "succeed")
		{
			PrintStep(u8"Task completed, full result:");
			std::cout << CGray() << u8"  │   " << CReset() << task.dump(2) << std::endl;
			return TEST_PASS;
		}
		if (status == "failed" || status == "failure" || status == "error" ||
			status == "cancelled" || status == "canceled" || status == "rejected")
		{
			PrintKV(u8"Failure details", task.dump());
			return TEST_FAIL;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(VIDEO_POLL_INTERVAL_MS));
		waited += VIDEO_POLL_INTERVAL_MS / 1000;
	}

	PrintKV(u8"Error", u8"polling timed out (" + std::to_string(VIDEO_POLL_TIMEOUT_S) + u8"s)");
	return TEST_FAIL;
}

// ==================== Test runner ====================
typedef int (*TestFunc)(ALL_AI::AI&);

void RunTest(int index, int total, const std::string& name,
	bool station_ready, const std::string& skip_reason,
	ALL_AI::AI& ai, TestFunc func)
{
	PrintSection(index, total, name);
	if (!station_ready)
	{
		RecordResult(name, TEST_SKIP, skip_reason);
		return;
	}

	g_skip_note.clear();
	int status = TEST_FAIL;
	try
	{
		status = func(ai);
	}
	catch (const std::exception& e)
	{
		PrintKV(u8"Exception", e.what());
	}
	catch (...)
	{
		PrintKV(u8"Exception", u8"unknown exception");
	}
	RecordResult(name, status, status == TEST_SKIP ? g_skip_note : std::string());
	return;
}

int main()
{
	SetupConsole();
	PrintBanner();

	const int TOTAL_TESTS = 10;

	// Station initialization (ALL_AI_PRINT_ERROR is used during the demo: the HTTP
	// status code and server errors are printed directly)
	std::cout << CGray() << u8"Initializing AI instances..." << CReset() << std::endl;

	ALL_AI::AI ai_kimi(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
		kimi_url, kimi_api_key, ALL_AI::ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR);
	bool kimi_ready = IsKeyConfigured(kimi_api_key) && ai_kimi.InitAI();
	std::cout << "  KIMI        : "
		<< (kimi_ready ? std::string(CGreen()) + u8"ready" : std::string(CYellow()) + u8"not ready")
		<< CReset() << CGray() << "  (key: " << MaskKey(kimi_api_key) << ")" << CReset() << std::endl;

	ALL_AI::AI ai_sf(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
		sf_url, sf_api_key, ALL_AI::ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR);
	bool sf_ready = IsKeyConfigured(sf_api_key) && ai_sf.InitAI();
	std::cout << "  SiliconFlow : "
		<< (sf_ready ? std::string(CGreen()) + u8"ready" : std::string(CYellow()) + u8"not ready")
		<< CReset() << CGray() << "  (key: " << MaskKey(sf_api_key) << ")" << CReset() << std::endl;

	ALL_AI::AI ai_gen(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
		gen_url, gen_api_key, ALL_AI::ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR);
	bool gen_ready = IsKeyConfigured(gen_api_key) && ai_gen.InitAI();
	std::cout << "  Generation  : "
		<< (gen_ready ? std::string(CGreen()) + u8"ready" : std::string(CYellow()) + u8"not ready")
		<< CReset() << CGray() << "  (key: " << MaskKey(gen_api_key) << ")" << CReset() << std::endl;

	const std::string KIMI_SKIP = u8"please configure kimi_api_key";
	const std::string SF_SKIP = u8"please configure sf_api_key";
	const std::string GEN_SKIP = u8"please configure gen_url / gen_api_key";

	// KIMI (Moonshot)
	//RunTest(1, TOTAL_TESTS, u8"Basic Chat (Builder + Tools + JsonGet)", kimi_ready, KIMI_SKIP, ai_kimi, Test_BasicChat);
	//RunTest(2, TOTAL_TESTS, u8"Model List (SendRequestRaw · GET)", kimi_ready, KIMI_SKIP, ai_kimi, Test_ModelList);
	//RunTest(3, TOTAL_TESTS, u8"File Gateway (Upload / Content / List / Delete)", kimi_ready, KIMI_SKIP, ai_kimi, Test_FileGateway);
	//RunTest(4, TOTAL_TESTS, u8"Files to Messages (Files.ToMessages)", kimi_ready, KIMI_SKIP, ai_kimi, Test_FilesToMessages);
	//RunTest(5, TOTAL_TESTS, u8"Video Recognition (upload video + AddVideoFileId chat)", kimi_ready, KIMI_SKIP, ai_kimi, Test_VideoRecognition);

	// SiliconFlow
	//RunTest(6, TOTAL_TESTS, u8"Speech to Text (SendMultipartRequest)", sf_ready, SF_SKIP, ai_sf, Test_SpeechToText);
	RunTest(7, TOTAL_TESTS, u8"Text to Speech (SetDataCallback binary stream)", sf_ready, SF_SKIP, ai_sf, Test_TextToSpeech);
	RunTest(8, TOTAL_TESTS, u8"Audio Understanding Chat (ContentPartBuilder · audio_url)", sf_ready, SF_SKIP, ai_sf, Test_AudioChat);
	//RunTest(9, TOTAL_TESTS, u8"Image Generation (SendRequestRaw · images/generations)", sf_ready, SF_SKIP, ai_sf, Test_ImageGeneration);

	// Video generation station (relay-style task API)
	//RunTest(10, TOTAL_TESTS, u8"Video Generation (task creation + polling, up to 300s)", gen_ready, GEN_SKIP, ai_gen, Test_VideoGeneration);

	PrintSummary();

	// Return a non-zero exit code when any test fails, for CI integration
	for (size_t i = 0; i < g_records.size(); ++i)
	{
		if (g_records[i].status == TEST_FAIL)
		{
			return 1;
		}
	}
	return 0;
}
