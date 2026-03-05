# ALL-AI-Cpp V3 示例 Demo

本章节将通过 `Demo/` 目录下的示例代码，详细讲解如何使用 V3 版本的 API 进行开发。这包括聊天对话、图像生成以及视频生成的完整流程。

> **注意**: 运行示例前，请确保您已拥有有效的 API Key 和对应的 API Endpoint URL。

## 1. 聊天对话 (Chat Completion)

文件: `Demo/ChatDemo-V3.cpp`

此示例展示了最基础的对话请求构建流程：
1. **初始化**: 设置 `IHttpTransport`、URL 和 Key。
2. **构建参数**: 设置模型 (`model`)、流式开关 (`stream`)。
3. **添加消息**: 使用 `JsonOperatorTools` 便捷地添加 System 和 User 消息。
4. **发送请求**: 使用 POST 方法发送并在控制台输出结果。

### 示例代码

```cpp
#include "ALL-AI-V3.hpp"
#include <iostream>

// 请替换为您实际的 API URL 和 Key
std::string url = "https://api.openai.com/v1/chat/completions";
std::string api_key = "sk-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

int main()
{
    // 1. 初始化 AI 对象
    // 使用默认的 CurlHttpTransport，并设置不抛出异常 (错误信息可能通过 std::cerr 或返回值体现)
    ALL_AI::AI ai(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
        url,
        api_key,
        ALL_AI::ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW);

    if (!ai.InitAI()) {
        std::cerr << "AI initialization failed." << std::endl;
        return 1;
    }
    std::cout << "AI initialized successfully." << std::endl;

    // 2. 设置请求参数
    ai.GetBuilder().SetValue("gpt-3.5-turbo", "model");
    ai.GetBuilder().SetValue(false, "stream"); // 关闭流式输出

    // 3. 构建消息列表
    // 使用 Tools 辅助类添加消息，简化 JSON 结构构造
    ai.GetTools().PushBackArray(ALL_AI::JsonOperatorTools::Role::System, "You are a helpful assistant.");
    ai.GetTools().PushBackArray(ALL_AI::JsonOperatorTools::Role::User, "Introduce Github to me");
    
    // 将构建好的消息数组设置到请求体的 "messages" 字段
    ai.GetBuilder().SetValue(ai.GetTools().GetMessagesArray(), "messages");

    // 4. 发送请求
    std::cout << "Sending request..." << std::endl;
    nlohmann::json response = ai.SendRequestFromBuilder_Post();

    // 5. 输出完整响应
    std::cout << "Response:\n" << response.dump(2) << std::endl;

    // 6. 提取回复内容
    // 安全地从深层 JSON 结构中获取 content 字段
    std::string content = ai.GetParser().GetValue<std::string>("choices", 0, "message", "content");
    std::cout << "--------------------------------------------------" << std::endl;
    std::cout << "Assistant Content:\n" << content << std::endl;

    return 0;
}
```

---

## 2. 视频生成 (Video Generation)

文件: `Demo/VideoDemo-V3.cpp`

此示例展示了一个更复杂的异步任务流程：
1. **提交任务**: 发送 POST 请求启动视频生成任务，服务器返回 `task_id`。
2. **状态轮询**: 修改 API URL 为查询接口 (通常是 `base_url/task_id`)。
3. **循环检查**: 定时发送 GET 请求，直到任务完成或超时。

这里演示了 `ReloadAI` 方法的用法，它允许在不重建对象的情况下更新 URL。

### 示例代码

```cpp
#include "ALL-AI-V3.hpp"
#include <iostream>
#include <thread>
#include <chrono>

// 视频生成 API URL (示例)
std::string base_url = "https://api.example.com/v1/video/generations";
std::string api_key = "sk-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

int main()
{
    // 1. 初始化
    ALL_AI::AI ai(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
        base_url, 
        api_key);

    if (!ai.InitAI()) return 1;

    // 2. 提交生成任务
    ai.GetBuilder().SetValue("veo3.1-fast", "model");
    ai.GetBuilder().SetValue("Helicopter takes off.", "prompt");

    std::cout << "Submitting video task..." << std::endl;
    nlohmann::json submit_resp = ai.SendRequestFromBuilder_Post();
    std::cout << "Task Submitted: " << submit_resp.dump(2) << std::endl;

    // 假设返回结构中包含 task_id
    if (!submit_resp.contains("task_id")) {
        std::cerr << "Error: No task_id returned." << std::endl;
        return 1;
    }
    std::string task_id = submit_resp["task_id"].get<std::string>();

    // 3. 切换 URL 以进行结果查询
    // 拼接新的 URL: base_url + "/" + task_id
    std::string query_url = base_url + "/" + task_id;
    
    // 使用 ReloadAI 更新 URL，保持 Key 和 Transport 不变
    // 注意：ReloadAI 的第三个参数如果也是默认值，可能会重置 Transport，需根据实际情况传参或仅修改 URL
    // 这里演示完整重新加载配置
    if (ai.ReloadAI(query_url, api_key)) {
        std::cout << "AI reloaded for polling: " << query_url << std::endl;
    }

    // 4. 轮询检查状态
    for (int i = 0; i < 60; ++i) // 尝试 60 次
    {
        // 发送 GET 请求查询状态 (Get 请求通常不需要 body，Builder 中的内容会被忽略或清空依赖具体实现，此处建议手动清空或新建请求)
        // SendRequestFromBuilder_Get 会发送一个 GET 请求
        nlohmann::json status_resp = ai.SendRequestFromBuilder_Get();
        
        std::cout << "[Poll " << i << "] Status: " << status_resp.dump() << std::endl;
        
        // 检查任务状态 (假设字段为 "status")
        std::string status = ai.GetParser().GetValue<std::string>("status");
        if (status == "succeeded") {
            std::cout << "Video generation succeeded!" << std::endl;
            // 获取视频链接...
            break;
        } else if (status == "failed") {
            std::cerr << "Video generation failed." << std::endl;
            break;
        }

        // 等待 2.5 秒再次查询
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    }

    return 0;
}
```

## 其他示例

*   **ImageDemo-V3.cpp**: 演示了文生图的请求构建，主要涉及 `prompt`, `size`, `n` 等参数的设置。
*   **ModelListsDemo-V3.cpp**: 演示了如何调用 GET 接口获取模型列表。

## 编译运行

所有示例均依赖 `ALL-AI-V3.hpp` (以及内部引用的 `nlohmann/json.hpp`) 和 `libcurl`。

### Windows (MSVC)

确保在项目属性中：
1.  **包含目录**: 添加 `include` 目录。
2.  **链接器 -> 输入**: 添加 `libcurl.lib`。
3.  **预处理器定义**: 确保定义了 `CURL_STATICLIB` (如果使用静态库)。

### Linux (g++)

```bash
g++ -o chat_demo Demo/ChatDemo-V3.cpp -I include -lcurl
./chat_demo
```
