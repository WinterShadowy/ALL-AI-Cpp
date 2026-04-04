<a id="top"></a>

## 聊天(Chat)

### 调用阐述：

- 调用聊天功能，需要使用命名空间中的`AI`类的`Set_Chat()`函数；

- 在`AI`类中的函数声明如下：
```cpp
Chat* Set_Chat();
```
该函数会返回一个`Chat`类指针，通过该指针来设置json的结构；

- 使用`SendRequest`函数来发送请求，无论是否执行成功，都会返回一个json对象，执行失败会返回一个空对象;

- 发送请求后，若执行成功，可以通过`Chat`类的`GetJson()`成员函数来获取解析器对象；

- 获取解析器对象后，可以通过`GetCount()`函数来获取`contant`类的实例，在此实例中可以进步获取你想要的回复信息。


### 示例代码：

1. 以下是一个聊天调用的代码示例，直接输出json信息：

```cpp
#include "ALL-AI.hpp"

std::string url = "https://url.com/v1/chat/completions";    // 使用：URL/v1/chat/completions格式，或者是：URL/v1/格式
std::string api_key = "YOUR_API_KEY";    // API-Key
using namespace std;

int main() 
{
	ALL_AI::AI ai(url, api_key);
	ai.Set_Chat()->SetJson()->Model(std::string("gpt-3.5-turbo"));   // 设置模型
	ai.Set_Chat()->SetJson()->Push_Back_Message("system", "You are a helpful assistant.");
	ai.Set_Chat()->SetJson()->Push_Back_Message("user", "Explain the development history of Github.");    // 设置询问内容
	ai.Set_Chat()->SetJson()->N(1);     // 设置回复数量
	ai.Set_Chat()->SetJson()->MaxTokens(200);
	ai.Set_Chat()->SetJson()->Temperature(0.9);
	nlohmann::json res = ai.SendRequest(ai.Set_Chat()->GetRequestBuilderJson(), AI_CHAT);    // 发送请求

	cout << res.dump(2) << endl;

	return 0;
}
```

2. 以下是一个使用`GetJson`函数来获取回复信息的示例：

```cpp
#include "ALL-AI.hpp"

std::string url = "https://url.com/v1/chat/completions";    // 使用：URL/v1/chat/completions格式，或者是：URL/v1/格式
std::string api_key = "YOUR_API_KEY";    // API-Key
using namespace std;

int main() 
{
	ALL_AI::AI ai(url, api_key);
	ai.Set_Chat()->SetJson()->Model(std::string("gpt-3.5-turbo"));
	ai.Set_Chat()->SetJson()->Push_Back_Message("system", "You are a helpful assistant.");
	ai.Set_Chat()->SetJson()->Push_Back_Message("user", "Explain the development history of Github.");
	ai.Set_Chat()->SetJson()->N(1);
	ai.Set_Chat()->SetJson()->MaxTokens(200);
	ai.Set_Chat()->SetJson()->Temperature(0.9);
	ai.SendRequest(ai.Set_Chat()->GetRequestBuilderJson(), AI_CHAT);    // 发送请求

	cout << ai.Set_Chat()->GetReply().GetContent();  // 直接获取json对象中content的值

	return 0;
}
```

<hr>

**[⏪上一页](/v2/api/Model-List/)**  |  **[⏫回到顶部](#top)**  |  **[下一页⏩](/v2/api/Images/)**