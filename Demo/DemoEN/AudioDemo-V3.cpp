#include "ALL-AI-V3.hpp"
#include <iostream>
#include <fstream>

// This test case was written by: Kimi K3

// windows.h is required to set the console to the UTF-8 code page on Windows
// Note: the DELETE macro defined in windows.h has already been undefined by ALL-AI-V3.hpp,
//       so including it after the header causes no conflict
#if (defined(_WIN32) || defined(_WIN64))
#include <windows.h>
#endif

// SiliconFlow official site
// If you are using an API relay station, replace the URL below with the relay address
std::string url = "YOUR_URL";
std::string api_key = "YOUR_API_KEY";

// Full URLs of the special endpoints (since v3.2 they are always passed explicitly by
// the caller; the library never stores, derives, or maps endpoint URLs).
// When using a relay station, replace them with the relay's actual paths
const std::string STT_URL = "YOUR_STT_URL";
const std::string TTS_URL = "YOUR_TTS_URL";

// Speech recognition model: FunAudioLLM/SenseVoiceSmall on SiliconFlow
// (TeleAI/TeleSpeechASR is also available).
// Note: ASR models only serve the /v1/audio/transcriptions endpoint;
// they CANNOT be used for the chat completions request below
const std::string stt_model = "YOUR_STT_MODEL";

// Text-to-speech model: fnlp/MOSS-TTSD-v0.5 on SiliconFlow; voice is one of its
// built-in speakers
const std::string tts_model = "YOUR_TTS_MODEL";
const std::string tts_voice = "YOUR_TTS_VOICE";

// Audio understanding model (chat completions): must be a chat model that supports
// audio input (currently the Qwen3-Omni series on SiliconFlow). Replace it with a
// model available in your account; ASR models (e.g. SenseVoiceSmall/XingChenASR)
// cannot be used with this endpoint
const std::string audio_chat_model = "YOUR_AUDIO_CHAT_MODEL";

// TestAudio.mp3 - a Chinese tongue-twister speech clip (TTS-generated test audio);
// the reference text is used to compare against the recognition results.
// If you replace it with your own audio file, update the reference text accordingly.
// Note: relative paths are resolved against the working directory. When debugging in
// Visual Studio, the working directory defaults to the project directory rather than
// the exe directory.
// The u8 prefix guarantees the Chinese literal is always stored as UTF-8: without
// the /utf-8 option MSVC converts literals to GBK, producing garbled console output
// and invalid UTF-8 in the request sent to the server
const std::string audio_file_path = "../TestFiles/TestAudio.mp3";
const std::string tts_output_path = "../TestFiles/TtsOutput.mp3";
const std::string audio_reference_text = u8"八百标兵奔北坡，炮兵并排北边跑，炮兵怕把标兵碰，标兵怕碰炮兵炮。";

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

	// ==================== Part 1: Speech to Text (STT) ====================
	// Speech-to-text is a typical "multipart form in, json out" endpoint; call the
	// mechanism layer SendMultipartRequest directly: the URL and every form field
	// (field names, model name) are given explicitly by the caller - the library
	// presumes no field names
	std::cout << "\n========== Speech To Text ==========" << std::endl;
	nlohmann::json stt_result = ai.SendMultipartRequest(STT_URL,
		audio_file_path,
		"file",
		{ {"model", stt_model} });
	std::cout << "STT Result: " << std::endl << stt_result.dump(2) << std::endl;

	// Use the stateless free function JsonGet to read a value safely by path: when the
	// path does not exist or the type does not match, the error is handled according to
	// the configured error mode and an empty value is returned, instead of throwing a
	// type_error like direct subscript access would
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

	// ==================== Part 2: Text to Speech (TTS, binary stream response) ====================
	// TTS returns raw audio bytes instead of json: set a data callback via
	// SetDataCallback so response chunks are delivered to the callback one by one
	// (here they are written straight to a file); the library never inspects the
	// delivered data. Always call ClearDataCallback afterwards to restore the
	// default collect-and-parse behavior
	std::cout << "\n========== Text To Speech ==========" << std::endl;
	std::ofstream tts_file(tts_output_path, std::ios::binary);
	if (!tts_file.good())
	{
		std::cout << "Cannot open output file: " << tts_output_path << std::endl;
		return 1;
	}

	ai.SetDataCallback([&tts_file](const char* data, size_t size) -> size_t {
		tts_file.write(data, static_cast<std::streamsize>(size));
		return size;	// return the number of bytes consumed; a mismatch aborts the request
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

	// Simple check that the output file was written (exists and is not empty)
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

	// ==================== Part 3: Audio Understanding (chat completions) ====================
	// Build a multimodal user message fluently with the library's ContentPartBuilder
	// (Builder pattern): AddAudioBase64 reads the audio file and encodes it as an
	// audio_url content part (a data URI), following the same OpenAI convention as
	// AddImageBase64 for images.
	// Verified site difference (2026-10, isolated curl tests): the SiliconFlow gateway
	// accepts the OpenAI-style audio_url audio part but rejects the upstream
	// DashScope-style input_audio part (400, code 20029 "Only text and image_url are
	// supported"), and does not require stream+modalities. If you switch to an upstream
	// site such as Alibaba Model Studio (DashScope), use its input_audio part with
	// stream=true instead — callers adapt to site differences as shown here
	std::cout << "\n========== Audio Understanding Chat ==========" << std::endl;
	nlohmann::json user_message = ALL_AI::FileOperator::ContentPartBuilder()
		.AddAudioBase64(audio_file_path)
		.AddText(u8"这段音频里说了什么？请直接转写原文，不要输出多余内容。")
		.BuildUserMessage();

	// AddAudioBase64 adds no part when the file cannot be read; validate the part count
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
