<a id="top"></a>

## 视频(Video)

### 调用阐述：

- 调用AI视频功能，需要使用命名空间中的`AI`类的`Set_Video()`函数；

- 在`AI`类中的函数声明如下：
```cpp
Video* Set_Video();
```
该函数会返回一个`Video`类指针，通过该指针来设置json的结构；

- 使用`SendRequest`函数来发送请求，无论是否执行成功，都会返回一个json对象，执行失败会返回一个空对象;

- 发送请求后，若执行成功，可以通过`Video`类的`GetJson()`成员函数来获取解析器对象；

- 获取解析器对象后，可以通过`GetCount()`函数来获取`contant`类的实例，此实例可以进步获取你想要的回复信息。

- 对于中转站，生成视频可能会返回一个`task_id`,之后需要使用GET请求来获取任务状态。某些键：如`status`的值为"SUCCESS"时，标识任务完成，某些键的值可能会被修改一个可供下载视频的URL。

### 代码示例:

以下是一个简单的示例：

```cpp
#include "ALL-AI.hpp"

std::string url = "https://url.com/v2/videos/generations";
std::string api_key = "YOUR_API_KEY";
using namespace std;
using json = nlohmann::json;

int main()
{
	ALL_AI::AI ai(url, api_key);
	ai.Set_Video()->SetJson()->Model(std::string("veo2-fast-components"));
	ai.Set_Video()->SetJson()->Prompt(std::string("dancing."));
	std::string task_id = ai.SendRequest(ai.Set_Video()->GetRequestBuilderJson(), AI_VIDEO).at("task_id").get<std::string>();
	cout << task_id << endl;

	ai.Reinitialize(std::string(url + "/" + task_id), api_key);
	for (int i = 0; i < 15000; ++i)
	{
		cout << "Get " << i << "\n" << ai.SendRequest_GET().dump(2) << endl;
		if (ai.SendRequest_GET().at("status").get<std::string>() == "SUCCESS")
		{
			break;
		}
		Sleep(2500);
	}
	return 0;
}
```
前面都是一些请求设置，for循环用于询问视频生成进度。当status字段值为SUCCESS时，表明已经生成完成。
具体生成请参考你使用的中转站的文档。

<hr>

**[⏪上一页](/v2/api/Audio/)**  |  **[⏫回到顶部](#top)**  |  **[下一页⏩](/v2/api/Moderation/)**