# 常见用法 (Common Usage)

ALL-AI-Cpp V3 的设计理念是一个通用的 HTTP JSON 客户端。这意味着你可以通过构建任何 JSON 结构并发往任何 API 端点来实现各种功能。

以下是如何构建常见 OpenAI 风格 API 请求的指南。

Please refer to the official API documentation of your provider (e.g., OpenAI, Anthropic, etc.) for the exact JSON structure required.
请参考你使用的 API 提供商的官方文档（如 OpenAI, Anthropic 等）以获取准确的 JSON 结构要求。

---

## 聊天补全 (Chat Completion)

**目标端点 (Endpoint)**: `https://api.openai.com/v1/chat/completions` (或兼容地址)

**代码示例**:

```cpp
// 初始化 AI 时传入完整 URL
std::string url = "https://api.openai.com/v1/chat/completions";
ALL_AI::AI ai(transport, url, api_key);
ai.InitAI();

// 构建 JSON
ai.GetBuilder().SetValue("gpt-3.5-turbo", "model");
ai.GetBuilder().SetValue(0.7, "temperature");

//设置消息
ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM, "You are a helpful assistant.");
ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER, "Hello, who are you?");
ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

// 发送请求
auto response = ai.SendRequestFromBuilder_Post();
```

---

## 图像生成 (Image Generation)

**目标端点 (Endpoint)**: `https://api.openai.com/v1/images/generations`

**代码示例**:

```cpp
std::string url = "https://api.openai.com/v1/images/generations";
ai.ReloadAI(url); // 如果复用 AI 对象，可以 Reload URL

// 清空旧的 builder 数据
ai.GetBuilder().ClearBuilder();

// 构建 JSON (参考 OpenAI Image API)
ai.GetBuilder().SetValue("dall-e-3", "model");
ai.GetBuilder().SetValue("A cute baby sea otter", "prompt");
ai.GetBuilder().SetValue("1024x1024", "size");
ai.GetBuilder().SetValue(1, "n");

// 发送请求
auto response = ai.SendRequestFromBuilder_Post();
```

---

## 模型列表 (Model List)

**目标端点 (Endpoint)**: `https://api.openai.com/v1/models`

**代码示例**:

```cpp
std::string url = "https://api.openai.com/v1/models";
ai.ReloadAI(url);

// 发送 GET 请求 (通常不需要 Body)
auto response = ai.SendRequestFromBuilder_Get();

// 输出结果
std::cout << response.dump(2) << std::endl;
```

---

## 自定义请求 (Custom Request)

你可以构建任何 JSON 结构。

```cpp
// 假设某个 API 需要如下 JSON:
// {
//   "task": "translation",
//   "input": {
//     "text": "Hello",
//     "target_lang": "zh"
//   }
// }

ai.GetBuilder().ClearBuilder();
ai.GetBuilder().SetValue("translation", "task");
ai.GetBuilder().SetValue("Hello", "input", "text");
ai.GetBuilder().SetValue("zh", "input", "target_lang");

ai.SendRequestFromBuilder_Post();
```
