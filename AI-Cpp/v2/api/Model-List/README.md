<a id="top"></a>

## 获取模型列表（Model-List）

### 调用阐述：

- 调用列举模型功能，直接使用`SendRequest`函数来发送请求，无论是否执行成功，都会返回一个json对象，执行失败会返回一个空对象，执行成功则返回一个json对象;

- 获取中转站的模型信息，您需要自己编写回调函数，回调函数必须满足以下格式：

```cpp
void Function(nlohmann::json JsonInfo);
```

### 代码示例:

以下是一个列出中转站模型信息的示例：

```cpp
#include "ALL-AI.hpp"

using namespace std;

std::string url = "https://URL.com/v1/models";
std::string api_key = "YOUR_API_KEY";

int main()
{
    ALL_AI::AI ai(url, api_key);
    cout << ai.SendRequest(ai.Set_ModelList()->GetRequestBuilderJson(), AI_MODEL_LIST).dump(2);
    return 0;
}
```

以下是一个使用回调函数输出模型信息的例子：

```cpp
#include "ALL-AI.hpp"

using namespace std;

std::string url = "https://URL.com/v1/models";
std::string api_key = "YOUR_API_KEY";

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

**[⏪上一页](/v2/api/Edits/)**  |  **[⏫回到顶部](#top)**  |  **[下一页⏩](/v2/api/Chat/)**