#include "ALL-AI-V3.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>

using namespace std;

// Global Configuration
std::string url = "YOUR_URL";
std::string api_key = "YOUR_API_KEY";

// Used for synchronizing console output
std::mutex print_mutex;

void Worker(std::shared_ptr<ALL_AI::AI> ai, int thread_id)
{
    {
        std::lock_guard<std::mutex> lock(print_mutex);
        std::cout << "[Thread " << thread_id << "] Starting..." << std::endl;
    }

    // Each thread constructs its own request data to avoid logical conflicts caused by using a shared Builder
    // Although the AI class itself is thread-safe, 
    // modifying the state of the same Builder in different threads can lead to logical race conditions (i.e., requests overwriting each other)
    // Therefore, this demonstration uses manual JSON construction and calls SendRequest_POST, 
    // which is the most recommended high-concurrency mode
    nlohmann::json request_json;
    request_json["model"] = "gpt-3.5-turbo";
    request_json["stream"] = false;
    
    nlohmann::json messages = nlohmann::json::array();
    messages.push_back({{"role", "system"}, {"content", "You are a helpful assistant."}});
    messages.push_back({{"role", "user"}, {"content", "Hello from thread " + std::to_string(thread_id) + ". Who are you?"}});
    request_json["messages"] = messages;

    // Send request
    // Even if multiple threads simultaneously call SendRequest_POST of the same AI instance, 
    // the internal locking mechanism ensures the serial and safe execution of Transport
    try
    {
        nlohmann::json response = ai->SendRequest_POST(request_json);
        
        // Parse result (the result obtained here is the return result of this call, 
        // not the Parser state that may be overwritten by other threads within the AI)
        // Note: SendRequest returns a local JSON object, which is completely thread-safe
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
    // Create a shared AI instance
    auto ai = std::make_shared<ALL_AI::AI>(
        std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
        url,
        api_key,
        ALL_AI::ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR // Print errors for debugging
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

    // Start 5 threads to make concurrent requests
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i)
    {
        threads.emplace_back(Worker, ai, i + 1);
        // Slightly stagger the start time to simulate a more realistic concurrency scenario
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for all threads to complete
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
