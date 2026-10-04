# ALL-AI-Cpp V3 示例 Demo

本章节介绍 `Demo/` 目录下的示例代码。除特别说明外，每个示例都提供中英双版
（`Demo/DemoCN/` 与 `Demo/DemoEN/`），全部使用 `JsonGet` 无状态取值与 v3.2 接口。

> **注意**: 运行示例前，请确保您已拥有有效的 API Key 和对应的 API Endpoint URL。
> 示例中的相对路径（如 `../TestFiles/TestAudio.mp3`）相对于程序的工作目录——
> 命令行运行时请 `cd` 到 `Demo/DemoCN`（或 `DemoEN`）再执行；
> Visual Studio 调试时工作目录默认为项目目录，请检查路径是否匹配。

## 示例清单

| 文件 | 演示内容 | 关键接口 |
| --- | --- | --- |
| `MainTest-V3.cpp`（仅中文版） | **十项集成测试**：对话 / 模型列表 / 文件网关 / 文件转对话 / 视频识别 / STT / TTS / 音频理解 / 图片生成 / 视频生成 | 全部核心接口 |
| `ChatDemo-V3.cpp` | 最基础的对话请求 | `GetBuilder` / `GetTools` / `SendRequestFromBuilder_Post` / `JsonGet` |
| `ChatDemo-V3-Builder.cpp` | 构建器的多种用法对照 | `JsonRequestBuilder` |
| `ChatDemo-V3-Stream.cpp` | 流式（SSE）对话 | `stream=true`，传输层自动合并 |
| `ChatDemo-V3-Thread.cpp` | 多线程并发请求 | 线程安全（局部构建模式） |
| `Demo-Reload_AI.cpp` | 运行中切换 URL 配置 | `ReloadAI` |
| `ModelListsDemo-V3.cpp` | 获取模型列表 | `SendRequestRaw`(GET) |
| `FileDemo-V3.cpp` | 文件上传 / 内容抽取 / 列表 / 删除 / 文件对话 | `ai.Files` 文件网关 |
| `AudioDemo-V3.cpp` | 语音转文本 + 文本转语音 + 音频理解对话 | `SendMultipartRequest` / `SetDataCallback` / `ContentPartBuilder` |
| `ImageDemo-V3.cpp` | 文生图 | `SendRequestRaw`(POST) |
| `VideoDemo-V3.cpp` | 视频生成（任务创建 + 轮询） | `SendRequestRaw`(POST/GET) |
| `ArrayDemo-V3.cpp` | 构建器数组操作（增删查） | `JsonRequestBuilder` 数组 API |

测试资源文件位于 `Demo/TestFiles/`（小说开头文本、galgame 角色图片、中文绕口令音频；
`TestVideo.mp4` 为空占位文件，运行视频相关示例前须替换为真实视频文件）。

## 1. 集成测试：MainTest-V3.cpp

十项测试覆盖库的全部核心能力，按站点分组（KIMI 官方站 / 硅基流动 / 视频生成站点），
未配置对应站点的 Key 时自动 SKIP，有失败项时返回非零退出码（可接入 CI）：

```text
[1/10]  基础对话（构建器 + 工具类 + JsonGet）
[2/10]  模型列表（SendRequestRaw · GET）
[3/10]  文件网关（上传 / 内容 / 列表 / 删除）
[4/10]  多类型文件转对话（Files.ToMessages）
[5/10]  视频识别（上传视频 + AddVideoFileId 对话）
[6/10]  语音转文本（SendMultipartRequest）
[7/10]  文本转语音（SetDataCallback 二进制流）
[8/10]  音频理解对话（ContentPartBuilder · audio_url）
[9/10]  图片生成（SendRequestRaw · images/generations）
[10/10] 视频生成（任务创建 + 轮询，最长300秒）
```

## 2. 聊天对话：ChatDemo-V3.cpp

最基础的对话流程：初始化 → 构建参数 → 组装消息 → 发送 → `JsonGet` 取值。

```cpp
ALL_AI::AI ai(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
	url, api_key, ALL_AI::ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW);
if (!ai.InitAI()) { return 1; }

ai.GetBuilder().SetValue("gpt-3.5-turbo", "model");
ai.GetBuilder().SetValue(false, "stream");
ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM, "You are helpful assistant.");
ai.GetTools().PushBackArray(ALL_AI_TOOL_MESSAGE_ROLE_USER, "Introduce Github to me");
ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

nlohmann::json response = ai.SendRequestFromBuilder_Post();
std::string content = ALL_AI::JsonGet<std::string>(response, "choices", 0, "message", "content");
```

## 3. 文件网关：FileDemo-V3.cpp

文件全生命周期：上传 → 抽取内容 → 列表 → 删除，以及将文件内容注入对话。

```cpp
// 上传（KIMI 文件接口，purpose 为 file-extract）
nlohmann::json up = ai.Files.Upload("../TestFiles/TestDoc.txt", "file-extract", files_url);
std::string file_id = ALL_AI::JsonGet<std::string>(up, "id");

// 抽取文本（返回原始字符串）
std::string content = ai.Files.Content(file_id, files_url);

// 清理
ai.Files.Delete(file_id, files_url);
```

## 4. 语音三件套：AudioDemo-V3.cpp

同一示例演示三种请求形状完全不同的语音接口，体现"沉淀结构，不沉淀字段名"的设计原则：

```cpp
// STT：multipart 表单进，json 出
nlohmann::json stt = ai.SendMultipartRequest(stt_url, audio_path, "file",
	{ {"model", stt_model} });

// TTS：json 进，二进制音频流出（数据回调逐块写文件）
ai.SetDataCallback([&out](const char* data, size_t size) -> size_t {
	out.write(data, static_cast<std::streamsize>(size)); return size; });
ai.SendRequestRaw(ALL_AI::HttpMethod::POST, tts_url, tts_body);
ai.ClearDataCallback();

// 音频理解：audio_url 多模态对话（ContentPartBuilder 链式构建）
nlohmann::json user_message = ALL_AI::FileOperator::ContentPartBuilder()
	.AddAudioBase64(audio_path)
	.AddText(u8"这段音频里说了什么？")
	.BuildUserMessage();
```

## 5. 视频生成：VideoDemo-V3.cpp

任务式接口：POST 创建任务拿 `task_id`，GET 轮询直到完成（最长等待 300 秒）。
演示了机制层 `SendRequestRaw` 如何适配非对话端点，URL 与字段名全部按站点文档显式给出。

## 编译运行

所有示例均依赖 `ALL-AI-V3.hpp` (以及内部引用的 `nlohmann/json.hpp`) 和 `libcurl`。

### Windows (MSVC)

#### 测试系统：Windows 11 专业工作站版

确保在项目属性中：
1.  **包含目录**: 添加 `include` 目录。
2.  **链接器 -> 输入**: 添加 `libcurl.lib`。
3.  **预处理器定义**: 确保定义了 `CURL_STATICLIB` (如果使用静态库)。

### Linux (g++)

#### 测试系统：Ubuntu 22.04

```bash
cd /ALL-AI-Cpp

mkdir build && cd build

cmake ..

make
```

### Windows (g++ / MSYS2 MinGW64)

```bash
g++ -std=c++17 -I include Demo/DemoCN/MainTest-V3.cpp -o MainTest.exe -lcurl
```
