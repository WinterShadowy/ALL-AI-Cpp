#include "../../include/ALL-AI-V3.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <utility>
#include <thread>
#include <chrono>

// 主测试程序：聚合现有Demo的核心能力，逐项测试并输出汇总报告
// 本测试用例由：Kimi K3编写

// windows.h is required to set the console to the UTF-8 code page on Windows
// Note: the DELETE macro defined in windows.h has already been undefined by ALL-AI-V3.hpp,
//       so including it after the header causes no conflict
#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>
#endif

// ==================== 站点配置（替换为你自己的URL与API Key） ====================
// KIMI（Moonshot）官方站点：负责对话、模型列表、文件网关、视频识别测试
std::string kimi_url = "YOUR_KIMI_URL"; // 替换为你自己的KIMI站点URL
std::string kimi_api_key = "YOUR_KIMI_API_KEY"; // 替换为你自己的KIMI API Key
const std::string KIMI_MODELS_URL = "YOUR_KIMI_MODELS_URL"; // 替换为你自己的KIMI模型接口URL
const std::string KIMI_FILES_URL = "YOUR_KIMI_FILES_URL"; // 替换为你自己的KIMI文件接口URL
const std::string KIMI_CHAT_MODEL = "YOUR_KIMI_CHAT_MODEL"; // 替换为你自己的KIMI聊天模型

// 硅基流动（SiliconFlow）官方站点：负责语音与图片生成测试
// 如果你使用的是AI中转站，将URL替换为中转站地址即可（v3.2起端点URL一律显式传入）
std::string sf_url = "YOUR_SF_URL"; // 替换为你自己的硅基流动站点URL
std::string sf_api_key = "YOUR_SF_API_KEY"; // 替换为你自己的硅基流动API Key
const std::string SF_STT_URL = "YOUR_SF_STT_URL"; // 替换为你自己的SF语音转写接口URL
const std::string SF_TTS_URL = "YOUR_SF_TTS_URL"; // 替换为你自己的SF语音合成接口URL
const std::string SF_IMAGE_GEN_URL = "YOUR_SF_IMAGE_GEN_URL"; // 替换为你自己的SF图片生成接口URL
const std::string SF_STT_MODEL = "YOUR_SF_STT_MODEL"; // 替换为你自己的SF语音转写模型
const std::string SF_TTS_MODEL = "YOUR_SF_TTS_MODEL"; // 替换为你自己的SF语音合成模型
const std::string SF_TTS_VOICE = "YOUR_SF_TTS_VOICE"; // 替换为你自己的SF语音合成声音
// 音频理解模型（chat completions）：必须是支持音频输入的对话模型（硅基流动目前为Qwen3-Omni系列）。
// 注意：ASR语音转写模型（如SenseVoiceSmall、XingChenAGI/XingChenASR）只服务于
// /v1/audio/transcriptions接口，填在这里会被服务器以400参数错误拒绝。
// 另经实测：硅基流动网关接受OpenAI约定的audio_url音频part，拒绝DashScope风格的input_audio part
const std::string SF_AUDIO_CHAT_MODEL = "YOUR_SF_AUDIO_CHAT_MODEL"; // 替换为你自己的SF音频聊天模型
const std::string SF_IMAGE_MODEL = "YOUR_SF_IMAGE_MODEL"; // 替换为你自己的SF图片生成模型

// 视频生成站点（中转站任务式接口：POST创建任务返回task_id，GET轮询任务状态直到完成）
// 参考 VideoDemo-V3.cpp：创建任务后按 任务URL/task_id 轮询，最长等待300秒
std::string gen_url = "YOUR_GEN_URL"; // 替换为你自己的视频生成接口URL
std::string gen_ask_task = "YOUR_GEN_ASK_TASK_URL"; // 替换为你自己的视频生成任务状态查询接口URL
std::string gen_api_key = "YOUR_GEN_API_KEY"; // 替换为你自己的视频生成API Key
const std::string VIDEO_GEN_MODEL = "YOUR_VIDEO_GEN_MODEL"; // 替换为你自己的视频生成模型
const int VIDEO_POLL_INTERVAL_MS = 5000;	// 轮询间隔（毫秒）
const int VIDEO_POLL_TIMEOUT_S = 300;		// 轮询最长等待（秒）

// 测试文件（注意：相对路径相对于程序的工作目录，Visual Studio调试时工作目录
// 默认为项目目录而非exe所在目录，测试失败时请先检查路径是否匹配）
const std::string DOC_PATH = "../TestFiles/TestDoc.txt";		// 一本小说的开头
const std::string IMAGE_PATH = "../TestFiles/TestImage.jpg";	// galgame角色图片
const std::string AUDIO_PATH = "../TestFiles/TestAudio.mp3";	// 中文绕口令语音
const std::string VIDEO_PATH = "../TestFiles/TestVideo.mp4";	// 视频文件（仓库中为空占位，请放入真实视频）
const std::string TTS_OUTPUT_PATH = "../TestFiles/TtsOutput.mp3";
const std::string AUDIO_REFERENCE = u8"八百标兵奔北坡，炮兵并排北边跑，炮兵怕把标兵碰，标兵怕碰炮兵炮。";

// ==================== 控制台美化（UTF-8 + ANSI颜色） ====================
static bool g_color_enabled = true;		// Linux/macOS终端默认支持ANSI颜色

// 控制台初始化：Windows下设置UTF-8代码页并尝试开启ANSI转义序列支持，
// 开启失败时自动降级为无颜色输出，兼容旧版控制台
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

// 颜色包装：颜色不可用时返回空字符串
const char* CReset() { return g_color_enabled ? "\033[0m" : ""; }
const char* CGreen() { return g_color_enabled ? "\033[32m" : ""; }
const char* CRed() { return g_color_enabled ? "\033[31m" : ""; }
const char* CYellow() { return g_color_enabled ? "\033[33m" : ""; }
const char* CCyan() { return g_color_enabled ? "\033[36m" : ""; }
const char* CGray() { return g_color_enabled ? "\033[90m" : ""; }

// ==================== 测试统计与输出 ====================
// 测试结果：PASS / FAIL / SKIP（SKIP时通过g_skip_note附带原因）
enum TestStatus {
	TEST_PASS = 0,
	TEST_FAIL = 1,
	TEST_SKIP = 2
};

struct TestRecord {
	std::string name;
	int status;		// TestStatus
	std::string note;		// SKIP原因等附加信息
};

std::vector<TestRecord> g_records;
std::string g_skip_note;		// 测试函数返回TEST_SKIP前填写的原因

void PrintBanner()
{
	std::cout << CCyan()
		<< u8"╔══════════════════════════════════════════════════════════════╗\n"
		<< u8"║        ALL-AI-V3 主测试程序 · MainTest                       ║\n"
		<< u8"║        对话 / 文件网关 / 视频识别 / 语音 / 图片与视频生成    ║\n"
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

// 测试步骤输出（灰色缩进）
void PrintStep(const std::string& step)
{
	std::cout << CGray() << u8"  ├─ " << CReset() << step << std::endl;
	return;
}

// 键值输出（缩进）
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
		<< u8"║                        测试汇总 · Summary                    ║\n"
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
		<< u8"合计: " << g_records.size()
		<< CGreen() << u8"  通过: " << pass_count << CReset()
		<< CRed() << u8"  失败: " << fail_count << CReset()
		<< CYellow() << u8"  跳过: " << skip_count << CReset()
		<< "\n"
		<< CCyan()
		<< u8"╚══════════════════════════════════════════════════════════════╝"
		<< CReset() << std::endl;
	return;
}

// ==================== 小工具 ====================

// API Key脱敏显示
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

// 读取本地文件大小（失败返回-1）
long GetFileSize(const std::string& file_path)
{
	std::ifstream file(file_path, std::ios::binary | std::ios::ate);
	if (!file.good())
	{
		return -1;
	}
	return static_cast<long>(file.tellg());
}

// 从文件列表json中提取文件ID集合（用于测试后清理本次新增的文件）
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

// ==================== KIMI（Moonshot）测试项 ====================

// 测试1：基础对话（构建器 + 工具类 + JsonGet取值）
int Test_BasicChat(ALL_AI::AI& ai)
{
	PrintStep(u8"构建请求（GetBuilder / GetTools）");
	ai.GetBuilder().ClearBuilder();
	ai.GetBuilder().SetValue(KIMI_CHAT_MODEL, "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM,
		u8"你是一个简洁的助手，始终用一句话回答。");
	ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER,
		u8"用一句话介绍 GitHub。");
	ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

	PrintStep(u8"发送请求并解析回复（SendRequestFromBuilder_Post + JsonGet）");
	nlohmann::json reply = ai.SendRequestFromBuilder_Post();
	std::string content = ALL_AI::JsonGet<std::string>(reply, "choices", 0, "message", "content");
	if (content.empty())
	{
		return TEST_FAIL;
	}
	PrintKV(u8"模型回复", content);
	return TEST_PASS;
}

// 测试2：模型列表（机制层SendRequestRaw的GET用法）
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
	PrintKV(u8"可用模型数", std::to_string(count));
	// 展示前3个模型ID
	for (size_t i = 0; i < count && i < 3; ++i)
	{
		PrintKV(u8"模型", ALL_AI::JsonGet<std::string>(model_list["data"][i], "id"));
	}
	return count > 0 ? TEST_PASS : TEST_FAIL;
}

// 测试3：文件网关底层流程（上传 / 内容 / 列表 / 删除）
int Test_FileGateway(ALL_AI::AI& ai)
{
	PrintStep(u8"上传文件（Files.Upload, purpose=file-extract）: " + DOC_PATH);
	nlohmann::json upload_result = ai.Files.Upload(DOC_PATH,
		ALL_AI::FileOperator::FilePurpose::FileExtract,
		KIMI_FILES_URL);
	std::string file_id = ALL_AI::JsonGet<std::string>(upload_result, "id");
	if (file_id.empty())
	{
		return TEST_FAIL;
	}
	PrintKV(u8"文件ID", file_id);

	PrintStep(u8"获取文件解析内容（Files.Content）");
	std::string raw_content = ai.Files.Content(file_id, KIMI_FILES_URL);
	nlohmann::json content_json = nlohmann::json::parse(raw_content, nullptr, false);
	std::string text = ALL_AI::JsonGet<std::string>(content_json, "content");
	if (text.empty())
	{
		return TEST_FAIL;
	}
	PrintKV(u8"内容长度", std::to_string(text.size()) + u8" 字符");

	PrintStep(u8"查询文件列表（Files.List）");
	std::set<std::string> ids = CollectFileIds(ai.Files.List(KIMI_FILES_URL));
	PrintKV(u8"当前文件数", std::to_string(ids.size()));

	PrintStep(u8"删除本次上传的文件（Files.Delete）: " + file_id);
	nlohmann::json delete_result = ai.Files.Delete(file_id, KIMI_FILES_URL);
	PrintKV(u8"删除结果", delete_result.is_null() ? u8"(空响应)" : delete_result.dump());
	return TEST_PASS;
}

// 测试4：多类型文件转对话（Files.ToMessages，策略模式），测试后清理新增文件
int Test_FilesToMessages(ALL_AI::AI& ai)
{
	// 记录转换前的文件集合，便于测试结束后仅清理本次新增的文件
	std::set<std::string> before = CollectFileIds(ai.Files.List(KIMI_FILES_URL));

	PrintStep(u8"转换文件为消息（文档→system消息，图片→base64内容part）");
	std::vector<std::string> file_paths = { DOC_PATH, IMAGE_PATH };
	nlohmann::json file_messages = ai.Files.ToMessages(file_paths, KIMI_FILES_URL);
	PrintKV(u8"生成消息数", std::to_string(file_messages.size()));

	bool ok = false;
	if (file_messages.size() == 2)
	{
		PrintStep(u8"追加用户问题并发起对话（需要视觉模型: " + KIMI_CHAT_MODEL + "）");
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
			PrintKV(u8"模型回复", content);
			ok = true;
		}
	}

	// 清理本次测试新增的文件（与转换前的集合做差集）
	PrintStep(u8"清理本次测试上传的文件");
	std::set<std::string> after = CollectFileIds(ai.Files.List(KIMI_FILES_URL));
	for (const std::string& id : after)
	{
		if (before.find(id) == before.end())
		{
			ai.Files.Delete(id, KIMI_FILES_URL);
			PrintKV(u8"已删除", id);
		}
	}
	return ok ? TEST_PASS : TEST_FAIL;
}

// 测试5：视频识别（上传视频→文件ID引用→对话；需要站点支持purpose=video与ms://引用）
int Test_VideoRecognition(ALL_AI::AI& ai)
{
	// 仓库中的TestVideo.mp4为0字节占位文件，无真实视频时跳过本测试
	long video_size = GetFileSize(VIDEO_PATH);
	if (video_size <= 0)
	{
		g_skip_note = u8"TestVideo.mp4 为空占位文件，请放入真实视频";
		return TEST_SKIP;
	}
	PrintKV(u8"视频文件", VIDEO_PATH + " (" + std::to_string(video_size) + " bytes)");

	PrintStep(u8"上传视频（Files.Upload, purpose=video）");
	nlohmann::json upload_result = ai.Files.Upload(VIDEO_PATH,
		ALL_AI::FileOperator::FilePurpose::Video,
		KIMI_FILES_URL);
	std::string video_id = ALL_AI::JsonGet<std::string>(upload_result, "id");
	if (video_id.empty())
	{
		return TEST_FAIL;
	}
	PrintKV(u8"文件ID", video_id);

	PrintStep(u8"构建视频消息（ContentPartBuilder.AddVideoFileId）并发起对话");
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

	// 无论对话成功与否，都清理本次上传的视频文件
	PrintStep(u8"清理本次上传的视频文件");
	ai.Files.Delete(video_id, KIMI_FILES_URL);
	PrintKV(u8"已删除", video_id);

	if (content.empty())
	{
		return TEST_FAIL;
	}
	PrintKV(u8"模型回复", content);
	return TEST_PASS;
}

// ==================== 硅基流动（SiliconFlow）测试项 ====================

// 测试6：语音转文本（机制层SendMultipartRequest：multipart表单进，json出）
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
	PrintKV(u8"识别结果", stt_text);
	PrintKV(u8"参考原文", AUDIO_REFERENCE);
	return TEST_PASS;
}

// 测试7：文本转语音（SetDataCallback数据回调：二进制流逐块写入文件）
int Test_TextToSpeech(ALL_AI::AI& ai)
{
	PrintStep(u8"POST json: " + SF_TTS_URL + u8"（响应为mp3二进制流）");
	std::ofstream tts_file(TTS_OUTPUT_PATH, std::ios::binary);
	if (!tts_file.good())
	{
		PrintKV(u8"错误", u8"无法打开输出文件: " + TTS_OUTPUT_PATH);
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
	ai.ClearDataCallback();		// 用完务必恢复默认的收集行为
	tts_file.close();

	long output_size = GetFileSize(TTS_OUTPUT_PATH);
	if (output_size <= 0)
	{
		return TEST_FAIL;
	}
	PrintKV(u8"输出文件", TTS_OUTPUT_PATH + " (" + std::to_string(output_size) + " bytes)");
	return TEST_PASS;
}

// 测试8：音频理解对话（ContentPartBuilder多模态消息）
// 经curl实测验证（2026-10）：硅基流动的chat completions接受OpenAI约定的
// audio_url内容part（{"type":"audio_url","audio_url":{"url":"data:audio/mp3;base64,..."}}），
// 而上游DashScope风格的input_audio part会被网关以400拒绝
//（code 20029 "Only text and image_url are supported"），且不要求stream+modalities。
// 库内置的ContentPartBuilder.AddAudioBase64生成的正是audio_url part，直接使用即可
int Test_AudioChat(ALL_AI::AI& ai)
{
	PrintStep(u8"构建音频多模态消息（ContentPartBuilder.AddAudioBase64）");
	nlohmann::json user_message = ALL_AI::FileOperator::ContentPartBuilder()
		.AddAudioBase64(AUDIO_PATH)
		.AddText(u8"这段音频里说了什么？请直接转写原文，不要输出多余内容。")
		.BuildUserMessage();

	// 音频文件读取失败时AddAudioBase64不会添加part，通过part数量校验
	if (!user_message["content"].is_array() || user_message["content"].size() < 2)
	{
		PrintKV(u8"错误", u8"音频文件读取失败: " + AUDIO_PATH);
		return TEST_FAIL;
	}

	PrintStep(u8"发送对话请求（模型: " + SF_AUDIO_CHAT_MODEL + "）");
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
	PrintKV(u8"模型回复", content);
	PrintKV(u8"参考原文", AUDIO_REFERENCE);
	return TEST_PASS;
}

// 测试9：图片生成（机制层SendRequestRaw：POST json进，json出；兼容data/images两种返回结构）
int Test_ImageGeneration(ALL_AI::AI& ai)
{
	const std::string prompt = u8"一只在月光下的樱花道上奔跑的柴犬，吉卜力动画风格";

	PrintStep(u8"POST json: " + SF_IMAGE_GEN_URL + u8"（模型: " + SF_IMAGE_MODEL + "）");
	PrintKV(u8"提示词", prompt);
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

	// 兼容两种返回结构：OpenAI格式 data[0].url / data[0].b64_json，硅基流动格式 images[0].url
	//（先判断键是否存在再取值，避免JsonGet对缺失路径按错误抛出方式打印干扰信息）
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
		PrintKV(u8"图片URL", image_url);
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
			PrintKV(u8"原始响应", resp.dump());
			return TEST_FAIL;
		}
		PrintKV(u8"图片数据", u8"b64_json（" + std::to_string(b64_json.size()) + u8" 字符）");
	}

	// 可选字段存在时才提取（避免JsonGet对缺失路径报错刷屏）
	if (resp.contains("seed"))
	{
		PrintKV(u8"随机种子", resp["seed"].dump());
	}
	return TEST_PASS;
}

// ==================== 视频生成站点（中转站）测试项 ====================

// 测试10：视频生成（任务式接口：POST创建任务，POST轮询直到完成，最长等待300秒）
int Test_VideoGeneration(ALL_AI::AI& ai)
{
	const std::string prompt = "一只在月光下的樱花道上奔跑的柴犬，吉卜力动画风格.";

	PrintStep(u8"POST json: " + gen_url + u8"（模型: " + VIDEO_GEN_MODEL + "）");
	PrintKV(u8"提示词", prompt);
	nlohmann::json create_body = {
		{"model", VIDEO_GEN_MODEL},
		{"prompt", prompt}
	};
	std::string raw = ai.SendRequestRaw(ALL_AI::HttpMethod::POST, gen_url, create_body);
	nlohmann::json create_resp = nlohmann::json::parse(raw, nullptr, false);
	if (!create_resp.is_object() || !create_resp.contains("requestId"))
	{
		PrintKV(u8"原始响应", raw);
		return TEST_FAIL;
	}

	std::string task_id = ALL_AI::JsonGet<std::string>(create_resp, "requestId");
	PrintKV(u8"任务ID", task_id);

	const std::string query_url = gen_ask_task;
	PrintStep(u8"开始轮询任务状态（每" + std::to_string(VIDEO_POLL_INTERVAL_MS / 1000)
		+ u8"秒一次，最长" + std::to_string(VIDEO_POLL_TIMEOUT_S) + u8"秒）");

	int waited = 0;
	while (waited <= VIDEO_POLL_TIMEOUT_S)
	{
		std::string task_raw = ai.SendRequestRaw(ALL_AI::HttpMethod::POST, query_url, create_resp);
		nlohmann::json task = nlohmann::json::parse(task_raw, nullptr, false);
		if (!task.is_object())
		{
			PrintKV(u8"轮询响应", task_raw);
			return TEST_FAIL;
		}

		// 状态字段名由站点约定，无status字段时展示原始响应供排查
		std::string status;
		if (task.contains("status") && task["status"].is_string())
		{
			status = ToLower(task["status"].get<std::string>());
		}
		PrintKV(u8"已等待 " + std::to_string(waited) + "s",
			status.empty() ? task.dump() : status);

		if (status == "succeeded" || status == "success" || status == "completed" ||
			status == "complete" || status == "done" || status == "finished" || status == "succeed")
		{
			PrintStep(u8"任务完成，完整结果：");
			std::cout << CGray() << u8"  │   " << CReset() << task.dump(2) << std::endl;
			return TEST_PASS;
		}
		if (status == "failed" || status == "failure" || status == "error" ||
			status == "cancelled" || status == "canceled" || status == "rejected")
		{
			PrintKV(u8"失败详情", task.dump());
			return TEST_FAIL;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(VIDEO_POLL_INTERVAL_MS));
		waited += VIDEO_POLL_INTERVAL_MS / 1000;
	}

	PrintKV(u8"错误", u8"轮询超时（" + std::to_string(VIDEO_POLL_TIMEOUT_S) + u8"秒）");
	return TEST_FAIL;
}

// ==================== 测试运行器 ====================
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
		PrintKV(u8"异常", e.what());
	}
	catch (...)
	{
		PrintKV(u8"异常", u8"未知异常");
	}
	RecordResult(name, status, status == TEST_SKIP ? g_skip_note : std::string());
	return;
}

int main()
{
	SetupConsole();
	PrintBanner();

	const int TOTAL_TESTS = 10;

	// 站点初始化（演示阶段使用 ALL_AI_PRINT_ERROR，HTTP状态码与服务器错误会直接打印）
	std::cout << CGray() << u8"初始化 AI 实例..." << CReset() << std::endl;

	ALL_AI::AI ai_kimi(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
		kimi_url, kimi_api_key, ALL_AI::ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR);
	bool kimi_ready = IsKeyConfigured(kimi_api_key) && ai_kimi.InitAI();
	std::cout << "  KIMI        : "
		<< (kimi_ready ? std::string(CGreen()) + u8"就绪" : std::string(CYellow()) + u8"未就绪")
		<< CReset() << CGray() << "  (key: " << MaskKey(kimi_api_key) << ")" << CReset() << std::endl;

	ALL_AI::AI ai_sf(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
		sf_url, sf_api_key, ALL_AI::ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR);
	bool sf_ready = IsKeyConfigured(sf_api_key) && ai_sf.InitAI();
	std::cout << "  SiliconFlow : "
		<< (sf_ready ? std::string(CGreen()) + u8"就绪" : std::string(CYellow()) + u8"未就绪")
		<< CReset() << CGray() << "  (key: " << MaskKey(sf_api_key) << ")" << CReset() << std::endl;

	ALL_AI::AI ai_gen(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
		gen_url, gen_api_key, ALL_AI::ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR);
	bool gen_ready = IsKeyConfigured(gen_api_key) && ai_gen.InitAI();
	std::cout << "  Generation  : "
		<< (gen_ready ? std::string(CGreen()) + u8"就绪" : std::string(CYellow()) + u8"未就绪")
		<< CReset() << CGray() << "  (key: " << MaskKey(gen_api_key) << ")" << CReset() << std::endl;

	const std::string KIMI_SKIP = u8"请配置 kimi_api_key";
	const std::string SF_SKIP = u8"请配置 sf_api_key";
	const std::string GEN_SKIP = u8"请配置 gen_url / gen_api_key";

	// KIMI（Moonshot）
	//RunTest(1, TOTAL_TESTS, u8"基础对话（构建器 + 工具类 + JsonGet）", kimi_ready, KIMI_SKIP, ai_kimi, Test_BasicChat);
	//RunTest(2, TOTAL_TESTS, u8"模型列表（SendRequestRaw · GET）", kimi_ready, KIMI_SKIP, ai_kimi, Test_ModelList);
	//RunTest(3, TOTAL_TESTS, u8"文件网关（上传 / 内容 / 列表 / 删除）", kimi_ready, KIMI_SKIP, ai_kimi, Test_FileGateway);
	//RunTest(4, TOTAL_TESTS, u8"多类型文件转对话（Files.ToMessages）", kimi_ready, KIMI_SKIP, ai_kimi, Test_FilesToMessages);
	//RunTest(5, TOTAL_TESTS, u8"视频识别（上传视频 + AddVideoFileId 对话）", kimi_ready, KIMI_SKIP, ai_kimi, Test_VideoRecognition);

	// 硅基流动（SiliconFlow）
	//RunTest(6, TOTAL_TESTS, u8"语音转文本（SendMultipartRequest）", sf_ready, SF_SKIP, ai_sf, Test_SpeechToText);
	RunTest(7, TOTAL_TESTS, u8"文本转语音（SetDataCallback 二进制流）", sf_ready, SF_SKIP, ai_sf, Test_TextToSpeech);
	RunTest(8, TOTAL_TESTS, u8"音频理解对话（ContentPartBuilder · audio_url）", sf_ready, SF_SKIP, ai_sf, Test_AudioChat);
	//RunTest(9, TOTAL_TESTS, u8"图片生成（SendRequestRaw · images/generations）", sf_ready, SF_SKIP, ai_sf, Test_ImageGeneration);

	// 视频生成站点（中转站任务式接口）
	//RunTest(10, TOTAL_TESTS, u8"视频生成（任务创建 + 轮询，最长300秒）", gen_ready, GEN_SKIP, ai_gen, Test_VideoGeneration);

	PrintSummary();

	// 有失败项时返回非零退出码，便于接入CI
	for (size_t i = 0; i < g_records.size(); ++i)
	{
		if (g_records[i].status == TEST_FAIL)
		{
			return 1;
		}
	}
	return 0;
}
