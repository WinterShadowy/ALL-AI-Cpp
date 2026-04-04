<a id="top"></a>

# 库的导入

1. 首先您需要确保您的项目/系统可以正常使用CURL库。（参考[安装CURL](../../FAQ/FAQ-install-curl/)）
2. 您只需在项目中引入`#include "ALL-AI.hpp"`即可。

# 测试代码

以下是一个测试文档，成功的话您可以看到中转站的模型输出：
```cpp
#include "ALL-AI.hpp"

using namespace std;

std::string url = "Your_Url";
std::string api_key = "Your_API_Key";

void func(nlohmann::json json)
{
    cout << json.dump(2) << endl;
    if (json.contains("data"))
    {
        for (auto& buffer : json["data"])
        {
            if (buffer.contains("id"))
            {
                cout << (buffer.at("id").is_string() ? buffer.at("id").get<std::string>() : "") << endl;
            }
        }
    }
}

int main()
{
    ALL_AI::AI ai(url, api_key);
    ai.SendRequest(ai.Set_ModelList()->GetRequestBuilderJson(), AI_MODEL_LIST);
    ai.Set_ModelList()->GetModelInfo(func);
    return 0;
}
```


<hr>

**[⏪上一页](../)**  |  **[⏫返回顶部](#top)**  |  **[下一页⏩](../api/)**