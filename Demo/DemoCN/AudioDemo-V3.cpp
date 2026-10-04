#include "ALL-AI-V3.hpp"
#include <iostream>
#include <fstream>

// 本测试用例由：Kimi K3编写

// windows.h is required to set the console to the UTF-8 code page on Windows
// Note: the DELETE macro defined in windows.h has already been undefined by ALL-AI-V3.hpp,
//       so including it after the header causes no conflict
#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>
#endif

// 硅基流动（SiliconFlow）官方站点
// 如果你使用的是AI中转站，将下面的URL替换为中转站地址即可
std::string url = "YOUR_URL";
std::string api_key = "YOUR_API_KEY";	// 替换为你自己的API Key

// 特殊端点的完整URL（自v3.2起一律由调用方显式传入，库不存储、不推导、不映射端点）
// 使用中转站时按中转站的实际路径替换即可
const std::string STT_URL = "YOUR_STT_URL";
const std::string TTS_URL = "YOUR_TTS_URL";

// 语音识别模型：硅基流动的 FunAudioLLM/SenseVoiceSmall（另有 TeleAI/TeleSpeechASR 等可选），
// 注意：ASR模型只服务于 /v1/audio/transcriptions 接口，不能用于下方的 chat completions
const std::string stt_model = "YOUR_STT_MODEL";

// 语音合成模型：硅基流动的 fnlp/MOSS-TTSD-v0.5，voice为该模型自带的音色
const std::string tts_model = "YOUR_TTS_MODEL";
const std::string tts_voice = "YOUR_TTS_VOICE";

// 音频理解模型（chat completions）：必须是支持音频输入的对话模型（硅基流动目前为Qwen3-Omni系列），
// 需替换为你的账户中可用的模型；ASR模型（如SenseVoiceSmall/XingChenASR）不能用于此接口
const std::string audio_chat_model = "YOUR_AUDIO_CHAT_MODEL";

// TestAudio.mp3 - 一段中文绕口令语音（TTS合成的测试音频），参考原文用于对照识别结果，
// 如果你替换成自己的音频文件，请同步修改下面的参考原文。
// 注意：相对路径相对于程序的工作目录，Visual Studio调试时工作目录默认为项目目录而非exe所在目录。
// u8前缀保证中文字面量始终以UTF-8编码存储：MSVC未开启/utf-8选项时默认把字面量转为GBK编码，
// 会导致控制台打印乱码、发送给服务器的文本变成非法UTF-8
const std::string audio_file_path = "../TestFiles/TestAudio.mp3";
const std::string tts_output_path = "../TestFiles/TtsOutput.mp3";
const std::string audio_reference_text = u8"八百标兵奔北坡，炮兵并排北边跑，炮兵怕把标兵碰，标兵怕碰炮兵炮。";

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

	// ==================== 第一部分：语音转文本（STT） ====================
	// 语音转写是典型的"multipart表单 + 返回json"端点，直接使用机制层SendMultipartRequest：
	// URL与表单字段（字段名、模型名）全部由调用方显式给出，库不预设任何字段名
	std::cout << "\n========== Speech To Text ==========" << std::endl;
	nlohmann::json stt_result = ai.SendMultipartRequest(STT_URL,
		audio_file_path,
		"file",
		{ {"model", stt_model} });
	std::cout << "STT Result: " << std::endl << stt_result.dump(2) << std::endl;

	// 使用无状态自由函数JsonGet按路径安全取值：路径不存在或类型不匹配时按错误抛出方式处理
	// 并返回空值，不会像直接下标取值那样抛出type_error异常
	std::string stt_text = ALL_AI::JsonGet<std::string>(stt_result, "text");
	if (stt_text.empty())
	{
		std::cout << "SpeechToText failed, text is empty." << std::endl;
		std::cout << "Please check: 1. api_key is correct; 2. the API station supports the /v1/audio/transcriptions interface; "
			<< "3. the relative file path matches the working directory." << std::endl;
		return 1;
	}
	std::cout << "\nRecognized Text: " << stt_text << std::endl;
	std::cout << "Reference  Text: " << audio_reference_text << std::endl;

	// ==================== 第二部分：文本转语音（TTS，二进制流响应） ====================
	// TTS返回的是音频二进制数据而非json：通过SetDataCallback设置数据回调，
	// 响应数据块逐块交给回调处理（此处直接写入文件），库本身不检查交付的数据；
	// 请求完成后务必用ClearDataCallback恢复默认的"收完响应再解析"行为
	std::cout << "\n========== Text To Speech ==========" << std::endl;
	std::ofstream tts_file(tts_output_path, std::ios::binary);
	if (!tts_file.good())
	{
		std::cout << "Cannot open output file: " << tts_output_path << std::endl;
		return 1;
	}

	ai.SetDataCallback([&tts_file](const char* data, size_t size) -> size_t {
		tts_file.write(data, static_cast<std::streamsize>(size));
		return size;	// 返回已消费的字节数；不等于size时libcurl会中止本次请求
		});

	nlohmann::json tts_body = {
		{"model", tts_model},
		{"input", u8"你站在桥上看风景，看风景的人在楼上看你。"},
		{"voice", tts_voice},
		{"response_format", "mp3"}
	};
	ai.SendRequestRaw(ALL_AI::HttpMethod::POST, TTS_URL, tts_body);
	ai.ClearDataCallback();
	tts_file.close();

	// 简单校验输出文件是否写入成功（存在且非空）
	std::ifstream tts_check(tts_output_path, std::ios::binary | std::ios::ate);
	if (tts_check.good() && tts_check.tellg() > 0)
	{
		std::cout << "TTS audio saved to: " << tts_output_path
			<< " (" << tts_check.tellg() << " bytes)" << std::endl;
	}
	else
	{
		std::cout << "TTS failed, no audio data was written." << std::endl;
		std::cout << "Please check: 1. api_key is correct; 2. the model " << tts_model
			<< " is available in your account." << std::endl;
		return 1;
	}

	// ==================== 第三部分：音频理解（chat completions） ====================
	// 使用库自带的 ContentPartBuilder（Builder模式）链式构建多模态user消息：
	// AddAudioBase64 读取音频文件并编码为 audio_url 内容part（data URI），
	// 与图片的 AddImageBase64 是同一套OpenAI格式约定。
	// 站点差异实测结论（2026-10，curl隔离验证）：硅基流动网关接受OpenAI约定的
	// audio_url音频part，拒绝上游DashScope风格的input_audio part
	//（400，code 20029 "Only text and image_url are supported"），且不要求stream+modalities；
	// 若切换到阿里百炼等上游站点，需按其文档改用input_audio part并开启stream，
	// 站点差异由调用方按本文所述格式自行适配
	std::cout << "\n========== Audio Understanding Chat ==========" << std::endl;
	nlohmann::json user_message = ALL_AI::FileOperator::ContentPartBuilder()
		.AddAudioBase64(audio_file_path)
		.AddText(u8"这段音频里说了什么？请直接转写原文，不要输出多余内容。")
		.BuildUserMessage();

	// 音频文件读取失败时AddAudioBase64不会添加part，通过part数量校验
	if (!user_message["content"].is_array() || user_message["content"].size() < 2)
	{
		std::cout << "Failed to read or encode the audio file: " << audio_file_path << std::endl;
		return 1;
	}

	nlohmann::json messages = nlohmann::json::array();
	messages.push_back(user_message);
	ai.GetBuilder().SetValue(audio_chat_model, "model");
	ai.GetBuilder().SetValue(false, "stream");
	ai.GetBuilder().SetValue(messages, "messages");

	nlohmann::json audio_reply = ai.SendRequestFromBuilder_Post();
	std::string reply = ALL_AI::JsonGet<std::string>(audio_reply, "choices", 0, "message", "content");
	if (reply.empty())
	{
		std::cout << "Audio understanding failed, reply is empty." << std::endl;
		std::cout << "Please check: 1. api_key is correct; 2. the model " << audio_chat_model
			<< " is available in your account and supports audio input." << std::endl;
		return 1;
	}
	std::cout << "\nModel Reply:    " << reply << std::endl;
	std::cout << "Reference Text: " << audio_reference_text << std::endl;
	return 0;
}
