# 常见用法 (Common Usage)

ALL-AI-Cpp V3 的设计理念：库只拥有不变的机制（可靠的 curl 收发 + 数据加工工具），
端点 URL、表单字段、响应字段等易变语义完全交给调用方——**沉淀结构，不沉淀字段名**。

以下示例均取自 `Demo/` 目录下经过真实联调的代码（KIMI 官方站 + 硅基流动）。
准确的 URL 与字段要求以各 API 提供商的官方文档为准。

---

## 聊天补全 (Chat Completion)

**端点**: `https://api.moonshot.cn/v1/chat/completions` (或兼容地址，构造时注入)

```cpp
ALL_AI::AI ai(transport, "https://api.moonshot.cn/v1/chat/completions", api_key);
ai.InitAI();

ai.GetBuilder().SetValue("kimi-k2.6", "model");
ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM, "You are a helpful assistant.");
ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER, "Hello, who are you?");
ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

nlohmann::json response = ai.SendRequestFromBuilder_Post();
std::string content = ALL_AI::JsonGet<std::string>(response, "choices", 0, "message", "content");
```

---

## 模型列表 (Model List)

**端点**: `GET https://api.moonshot.cn/v1/models`（机制层，URL 显式传入）

```cpp
std::string raw = ai.SendRequestRaw(ALL_AI::HttpMethod::GET, "https://api.moonshot.cn/v1/models");
nlohmann::json list = nlohmann::json::parse(raw, nullptr, false);
```

---

## 文件上传与管理 (Files)

**端点**: `https://api.moonshot.cn/v1/files`（url 参数必填）

```cpp
// 上传（KIMI 的 purpose 为 file-extract）
nlohmann::json up = ai.Files.Upload("Book.txt", "file-extract", files_url);
std::string file_id = ALL_AI::JsonGet<std::string>(up, "id");

// 抽取文本（返回原始字符串，KIMI 通常返回含 content 字段的 JSON 字符串）
std::string raw_content = ai.Files.Content(file_id, files_url);

// 列表 / 信息 / 删除
nlohmann::json list = ai.Files.List(files_url);
nlohmann::json info = ai.Files.Info(file_id, files_url);
ai.Files.Delete(file_id, files_url);
```

批量上传（自动识别类型、推导 purpose，单个失败不影响其他文件）：

```cpp
auto results = ai.Files.UploadBatch({ "a.txt", "b.png", "c.mp3" }, files_url);
for (const auto& r : results)
{
	if (r.success) { /* r.file_id / r.file_type */ }
}
```

---

## 文件转对话 (Files To Messages)

一次调用把混合文件（文档 + 图片 + 视频…）转为 messages 数组：

```cpp
nlohmann::json messages = ai.Files.ToMessages({ "../TestFiles/TestDoc.txt",
	"../TestFiles/TestImage.jpg" }, files_url);
// 将用户问题追加到数组末尾后再发起对话
messages.push_back({ {"role", "user"}, {"content", u8"总结文档并描述图片"} });
ai.GetBuilder().ClearBuilder();
ai.GetBuilder().SetValue(chat_model, "model");
ai.GetBuilder().SetValue(messages, "messages");
nlohmann::json reply = ai.SendRequestFromBuilder_Post();
```

---

## 语音转文本 STT (Speech To Text)

**端点**: `POST https://api.siliconflow.cn/v1/audio/transcriptions`（multipart 表单）

```cpp
nlohmann::json stt = ai.SendMultipartRequest(stt_url, "TestAudio.mp3", "file",
	{ {"model", "FunAudioLLM/SenseVoiceSmall"} });
std::string text = ALL_AI::JsonGet<std::string>(stt, "text");
```

> 表单字段名（`file` / `model`）完全由调用方按站点文档传入，库不预设字段名。

---

## 文本转语音 TTS (Text To Speech)

**端点**: `POST https://api.siliconflow.cn/v1/audio/speech`（JSON 进，二进制音频流出）

```cpp
std::ofstream out("TtsOutput.mp3", std::ios::binary);
ai.SetDataCallback([&out](const char* data, size_t size) -> size_t {
	out.write(data, static_cast<std::streamsize>(size));
	return size;
});

nlohmann::json body = {
	{"model", "fnlp/MOSS-TTSD-v0.5"},
	{"input", u8"你站在桥上看风景。"},
	{"voice", "fnlp/MOSS-TTSD-v0.5:alex"},
	{"response_format", "mp3"}
};
ai.SendRequestRaw(ALL_AI::HttpMethod::POST, tts_url, body);
ai.ClearDataCallback();	// 用完务必恢复默认的收集行为
```

---

## 音频理解对话 (Audio Understanding Chat)

本地音频 base64 编码为 `audio_url` content part（OpenAI 约定）：

```cpp
nlohmann::json user_message = ALL_AI::FileOperator::ContentPartBuilder()
	.AddAudioBase64("TestAudio.mp3")
	.AddText(u8"这段音频里说了什么？")
	.BuildUserMessage();

nlohmann::json messages = nlohmann::json::array();
messages.push_back(user_message);
ai.GetBuilder().SetValue("Qwen/Qwen3-Omni-30B-A3B-Instruct", "model");
ai.GetBuilder().SetValue(messages, "messages");
nlohmann::json reply = ai.SendRequestFromBuilder_Post();
```

> **站点差异（实测）**：硅基流动接受 `audio_url` part，拒绝 DashScope 风格的
> `input_audio` part（400, code 20029）。详见 [站点适配指南](/v3/endpoint-abstraction.md)。

---

## 图片生成 (Image Generation)

**端点**: `POST https://api.siliconflow.cn/v1/images/generations`（JSON 进 JSON 出）

```cpp
nlohmann::json body = {
	{"model", "Tongyi-MAI/Z-Image-Turbo"},
	{"prompt", u8"一只在月光下的樱花道上奔跑的柴犬，吉卜力动画风格"},
	{"image_size", "1024x1024"}
};
std::string raw = ai.SendRequestRaw(ALL_AI::HttpMethod::POST, image_url, body);
nlohmann::json resp = nlohmann::json::parse(raw, nullptr, false);
// 返回结构因站点而异（data[].url 或 images[].url），按站点文档用 JsonGet 取值
std::string url_out = ALL_AI::JsonGet<std::string>(resp, "data", 0, "url");
```

---

## 视频生成 (Video Generation · 任务式接口)

创建任务返回 `task_id`，GET 轮询任务状态直到完成（参考 `Demo/DemoCN(MainTest-V3.cpp)` 第 10 项，最长等待 300 秒）：

```cpp
// 1. 创建任务
nlohmann::json body = { {"model", "Wan-AI/Wan2.2-T2V-A14B"}, {"prompt", u8"直升机起飞"} };
std::string raw = ai.SendRequestRaw(ALL_AI::HttpMethod::POST, submit_url, body);
std::string task_id = ALL_AI::JsonGet<std::string>(
	nlohmann::json::parse(raw, nullptr, false), "id");

// 2. 轮询状态（GET {status_url}?task_id=...，字段名以站点文档为准）
for (int i = 0; i < 60; ++i)
{
	std::string sraw = ai.SendRequestRaw(ALL_AI::HttpMethod::GET,
		status_url + "?task_id=" + task_id);
	std::string status = ALL_AI::JsonGet<std::string>(
		nlohmann::json::parse(sraw, nullptr, false), "status");
	if (status == "succeeded") { /* 取视频地址 */ break; }
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}
```

---

## 自定义请求 (Custom Request)

构建器支持构建任意 JSON 结构（深层嵌套使用多键路径）：

```cpp
// {
//   "task": "translation",
//   "input": { "text": "Hello", "target_lang": "zh" }
// }
ai.GetBuilder().ClearBuilder();
ai.GetBuilder().SetValue("translation", "task");
ai.GetBuilder().SetValue("Hello", "input", "text");
ai.GetBuilder().SetValue("zh", "input", "target_lang");

ai.SendRequestFromBuilder_Post();
```
