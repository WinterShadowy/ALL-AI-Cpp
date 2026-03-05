# ALL-AI-Cpp

## 1. 项目简介

ALL-AI-Cpp 是一个轻量级的 C++ 库，完全由 C++ 编写，旨在简化 C++ 与 AI 模型的交互。
本项目致力于提供简单易用的接口，让开发者能够快速集成 AI 功能（如 ChatGPT、各类大模型 API 等）到自己的 C++ 项目中。

### V3 版本新特性 (New in V3)
V3 版本进行了架构重构，提供了更灵活的扩展能力：
- **依赖注入**：支持自定义 HTTP 传输层 (`IHttpTransport`)。
- **构建器模式**：提供 `JsonRequestBuilder`，构建 JSON 请求更便捷。
- **解析器策略**：通过 `JsonResponceParser` 灵活解析响应数据。
- **错误处理**：支持多种错误处理策略 (打印日志、回调函数、抛出异常)。
- **工具类**：新增 `JsonOperatorTools` 简化消息管理。

## 2. 环境要求

您的开发环境需要满足以下条件：
1. **编译器**：支持 C++17 或更高版本的编译器 (MSVC, G++, Clang 等)。
   - V3 建议使用 C++17。
   - V2 兼容 C++11/14。
2. **依赖库**：[libcurl](https://curl.se/libcurl/)
   - 请确保安装了 libcurl 开发包 (如 Linux 上的 `libcurl4-openssl-dev` 或 Windows 上的预编译包)。
   - 更多关于 Curl 的安装请参考：[Everything curl](https://everything.curl.dev/get)

## 3. 安装与集成

本项目为 **Header-Only** 库，无需复杂的构建过程。

1. **下载源码**：克隆或下载本项目。
2. **复制文件**：将仓库中的 `include/` 目录下的所有文件复制到您的项目中。
   - `include/ALL-AI-V3.hpp` (V3 核心头文件)
   - `include/ALL-AI.hpp` (V2 核心头文件)
   - `include/nlohmann/json.hpp` (JSON 依赖库)
3. **引入头文件**：
   - **使用 V3 (推荐)**：
     ```cpp
     #include "ALL-AI-V3.hpp"
     ```
   - **使用 V2 (旧版)**：
     ```cpp
     #include "ALL-AI.hpp"
     ```

## 4. 快速开始 (V3 示例)

以下是一个使用 V3 版本初始化 AI 对象的基础示例：

```cpp
#include "ALL-AI-V3.hpp"
#include <iostream>
#include <memory>

int main() {
    // 1. 设置 API 端点和密钥
    std::string url = "https://your-api-endpoint.com/v1/chat/completions"; 
    std::string api_key = "sk-xxxxxxxxxxxxxxxxxxxxxxxx";

    // 2. 初始化 AI 对象 (使用默认的 Curl 传输层)
    // 参数说明：传输层实现, API地址, API密钥, 错误处理策略
    ALL_AI::AI ai(std::make_shared<ALL_AI::HttpTransport::CurlHttpTransport>(),
                  url,
                  api_key,
                  ALL_AI::ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW);

    // 3. 执行初始化检查
    if (ai.InitAI()) {
        std::cout << "AI initialized successfully!" << std::endl;
        // 在此处编写对话或图像生成逻辑...
    } else {
        std::cerr << "Failed to initialize AI." << std::endl;
    }

    return 0;
}
```

更多完整示例请参考 `Demo/` 目录：
- [ChatDemo-V3.cpp](Demo/ChatDemo-V3.cpp) (对话示例)
- [ImageDemo-V3.cpp](Demo/ImageDemo-V3.cpp) (绘图示例)
- [VideoDemo-V3.cpp](Demo/VideoDemo-V3.cpp) (视频生成示例)

## 5. 文档与支持

- **本地文档**：
  - [V3 详细文档 (AI-Cpp/v3)](AI-Cpp/v3/README.md)
  - [V3 API 参考](AI-Cpp/v3/api/README.md)
  
 注：本地文档使用的项目为**docsify**
- **在线文档**：[ALL-AI-Cpp 使用手册](https://ai-cpp-docsify.cpluscottage.top/)

## 6. 支持与反馈

如果本项目对您有帮助，欢迎点击右上角的 **Star** ⭐ 支持我们！
您的支持是我们持续更新的动力。

---
**持续更新中...**
