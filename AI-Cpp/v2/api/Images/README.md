<a id="top"></a>

## 图像(Images)

### 调用阐述：

- 调用绘图功能，需要使用命名空间中的`AI`类的`Set_Images()`函数；

- 在`AI`类中的函数声明如下：
```cpp
Images* Set_Images();
```
该函数会返回一个`Images`类指针，通过该指针来设置json的结构；

- 使用`SendRequest`函数来发送请求，无论是否执行成功，都会返回一个json对象，执行失败会返回一个空对象;

- 发送请求后，若执行成功，可以通过`Images`类的`GetJson()`成员函数来获取解析器对象；

- 获取解析器对象后，可以通过`GetCount()`函数来获取`contant`类的实例，此实例可以进步获取你想要的回复信息。


### 代码示例:

以下是一个调用绘图功能的例子：

```cpp
#include "ALL-AI.hpp"

std::string url = "https://URL.com/v1/chat/completions";
std::string api_key = "YOUR_API_KEY";
using namespace std;

int main()
{
	ALL_AI::AI ai(url, api_key);
	ai.Set_Images()->SetJson()->Model(std::string("gpt-4o-image"));
	ai.Set_Images()->SetJson()->Push_Back_Message("user", "A dog.");
	ai.Set_Images()->SetJson()->Temperature(0.9);    // 这并不是必须的

	cout << ai.SendRequest(ai.Set_Images()->GetRequestBuilderJson(), AI_IMAGES).dump(2) << endl;
	return 0;
}
```

以下是直接打印contant内容的例子：

```cpp
#include "ALL-AI.hpp"

std::string url = "https://URL.com/v1/chat/completions";
std::string api_key = "YOUR_API_KEY";
using namespace std;

int main()
{
	ALL_AI::AI ai(url, api_key);
	ai.Set_Images()->SetJson()->Model(std::string("gpt-4o-image"));
	ai.Set_Images()->SetJson()->Push_Back_Message("user", "A dog.");
	ai.Set_Images()->SetJson()->Temperature(0.9);    // 这并不是必须的

	ai.SendRequest(ai.Set_Images()->GetRequestBuilderJson(), AI_IMAGES);
    
    // contant对象重载了“<<”运算符，默认返回json对象中content的值，输出的结果是一个URL
	cout << ai.Set_Images()->GetReply().GetContent();  
	return 0;
}
```

<hr>

**[⏪上一页](/v2/api/Chat/)**  |  **[⏫回到顶部](#top)**  |  **[下一页⏩](/v2/api/File/)**