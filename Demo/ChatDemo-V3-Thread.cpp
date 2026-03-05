#include "ALL-AI-V3.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>

using namespace std;

// 全局配置
std::string url = "YourURL";
std::string api_key = "Your API_Key";

// 用于同步控制台输出
std::mutex print_mutex;

void Worker(std::shared_ptr<ALL_AI::AI> ai, int thread_id)
{
    {
        std::lock_guard<std::mutex> lock(print_mutex);
        std::cout << "[Thread " << thread_id << "] Starting..." << std::endl;
    }

    // 每个线程构建自己的请求数据，避免使用共享的 Builder 导致逻辑冲突
    // 虽然 AI 类本身是线程安全的，但在不同线程修改同一个 Builder 状态会产生逻辑竞争（即请求内容互相覆盖）
    // 因此这里演示使用手动构建 JSON 并调用 SendRequest_POST，这是最推荐的高并发模式
    nlohmann::json request_json;
    request_json["model"] = "gpt-3.5-turbo";
    request_json["stream"] = false;
    
    nlohmann::json messages = nlohmann::json::array();
    messages.push_back({{"role", "system"}, {"content", "You are a helpful assistant."}});
    messages.push_back({{"role", "user"}, {"content", "Hello from thread " + std::to_string(thread_id) + ". Who are you?"}});
    request_json["messages"] = messages;

    // 发送请求
    // 即使多个线程同时调用同一个 ai 实例的 SendRequest_POST，内部的锁机制也会保证 Transport 的串行安全执行
    try
    {
        nlohmann::json response = ai->SendRequest_POST(request_json);
        
        // 解析结果 (这里获取的是本次调用的返回结果，而不是 AI 内部可能被其他线程覆盖的 Parser 状态)
        // 注意：SendRequest 返回的是局部的 json 对象，是完全线程安全的
        std::string content = "No content";
        if (response.contains("choices") && !response["choices"].empty())
        {
            content = response["choices"][0]["message"]["content"].get<std::string>();
        }

        {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "[Thread " << thread_id << "] Response: " << content.substr(0, 50) << "..." << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::lock_guard<std::mutex> lock(print_mutex);
        std::cerr << "[Thread " << thread_id << "] Error: " << e.what() << std::endl;
    }
}

int main()
{
    // 创建一个共享的 AI 实例
    auto ai = std::make_shared<ALL_AI::AI>(
        std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
        url,
        api_key,
        ALL_AI::ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR // 打印错误以便调试
    );

    if (ai->InitAI())
    {
        std::cout << "AI initialized successfully. Starting multi-thread test..." << std::endl;
    }
    else
    {
        std::cout << "AI initialization failed." << std::endl;
        return 1;
    }

    // 启动 5 个线程并发请求
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i)
    {
        threads.emplace_back(Worker, ai, i + 1);
        // 稍微错开一点启动时间，模拟更真实的并发场景
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 等待所有线程完成
    for (auto& t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    std::cout << "All threads finished." << std::endl;
    return 0;
}
