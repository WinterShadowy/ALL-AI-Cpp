<a id="top"></a>

## JsonResponseParser类：

<hr>

### 1. 构造函数：
#### 定义：
```cpp
JsonResponseParser(const json& json)
```
#### 参数：
  1. **const json& json**: 一个json对象
#### 使用：
创建JsonResonseParser类时使用，传入一个中转站返回的json对象

<hr>

### 2. 析构函数：
#### 定义：
```cpp
~JsonResponseParser()
```
#### 参数
无参数

<hr>

### 3. 设置Json对象：
#### 定义：
```cpp
void SetJson(const json& json_reply)
```
#### 参数：
  1. **const json&**: 一个json对象

#### 使用：
实例直接调用即可，无返回值。  

<hr>

### 4. 获取内容
#### 定义：
```cpp
content GetContent()
```
#### 参数:
无参数
#### 使用：
返回一个content实例

<hr>

### 5. 获取json对象某个字段的值:
#### 定义：
```cpp
template <typename _T>
_T GetJsonValue(std::string key) 
```
#### 参数：
  1. **std::string**: 一个字段字符串

#### 使用：
显式模板实例化时，可以获取某个字段的值，其类型为实例化时的类型，但不一定有效。函数执行失败返回空值

<hr>

### 6. 打印json到控制台中：
#### 定义：
```cpp
void PrintJson_CMD(unsigned int count = 0)
```
#### 参数：
  1. **unsigned int**: 输出json的格式

#### 使用：
实例直接打印

<hr>

### 7. 获取json对象：
#### 定义：
```cpp
json GetJson()
```
#### 参数：
无参数
#### 使用：
直接返回json对象

<hr>

### 8. json对象转换为字符串
#### 定义：
```cpp
std::string ToString(const json & json) 

std::wstring ToWString(const json& json) 
```
#### 参数:
  1. **const json&**: 一个json对象

#### 使用：
实例直接调用，返回一个json字符串

<hr>

### 9. 数字转换为字符串：
#### 定义：
```cpp
template <typename T>
std::string IntegerToString(T value)

template <typename T>
std::wstring IntegerToWString(T value)
```
#### 参数：
  1. **T**: 一个整型数据

#### 使用：
实例直接调用，函数执行成功返回一个string/wstring字符串。

<hr>

## JsonRequestBuilder类：

<hr>

### 1. 构造函数：
#### 定义：
```cpp
JsonRequestBuilder()
```
#### 参数：
无参数
#### 使用：
构造函数，初始化一些基本信息。

<hr>

### 2. 析构函数：
#### 定义：
```cpp
~JsonRequestBuilder()
```
#### 参数：
无参数
#### 使用：
无

<hr>

### 3. 向json对象中插入字段：
#### 定义：
```cpp
template <typename _T>
void Insert(const std::string& str_key, const _T& _t_value)
```
#### 参数：
  1. **const std::strinig&**: 一个字符串，作为json数据的键；
  2. **_T&**: 一个任意值，作为值；

#### 使用：
```cpp
instance->Insert("test1", "hello");
instance->Insert("test2", false);
...more...
```
无返回值，插入成功json对象中会出现你插入的键值对。失败则没有。

<hr>

### 4. 设置Model:
#### 定义：
```cpp
void Model(const std::string& str_model) 
```
#### 参数：
  1. **const std::string&**: 一个字符串，指定模型信息

#### 使用：
```cpp
instance->Model("gpt-3.5-turbo");
```
传入的参数是一个模型名称即可完成对json对象中模型信息的设置（使用前请参阅中转站是否支持传入的模型）。

<hr>

### 5. 设置N
#### 定义：
```cpp
void N(unsigned int n_n)
```
#### 参数：
  1. **unsigend int**: 一个无符号整型数据，用于设置中转站回复数

#### 使用
```cpp
instance->N(1);
```
传入的参数是一个无符号整形即可玩名称对json对象中模型回复数的设置（请勿填写负值或过大的值）

<hr>

### 6. 设置Prompt：
##### 定义：
```cpp
void Prompt(const std::string& str_prompt)
```
#### 参数：
  1. **const std::string&**: 一个字符串，用于设置AI人设（大概？）

#### 使用：
```cpp
instance->Prompt("you are a helpful assistent.");
```
传入一个字符串即可完成对AI人设的设置，某些模型可能不支持。是否支持请参阅相关文档。

<hr>

### 7. 设置Stop:
#### 定义：
```cpp
void Stop(const std::string& str_stop)
```
#### 参数：
  1. **const std::string&**: 一个字符串，用于设置停止标记

#### 使用：
实例直接调用即可。

### 8. 设置最大回复tokens
#### 定义：
```cpp
void MaxTokens(unsigned int un_max_tokens)
```
#### 参数：
  1. **unsigned int**: 一个无符号整形，为最大回复的Token数

#### 使用：
实例直接调用即可。

### 9. 设置回复随机度：
#### 定义：
```cpp
void Temperature(float f_temperature)
```
#### 参数：
  1. **float**: 一个单精度浮点数，控制生成文本的随机性，范围通常为0到2

#### 使用：
实例直接调用即可。

<hr>

### 10. 设置核采样
#### 定义：
```cpp
void TopP(unsigned int un_top_p)
```
#### 参数：
  1. **unsigend int**: 核采样参数，控制生成文本的多样性，值越低，选择越集中；值越高，选择越广泛

#### 使用：
实例直接调用即可。

<hr>

### 11. 设置流式方式
#### 定义：
```cpp
void Stream(bool b_stream)
```
#### 参数：
  1. **bool**: 值为true为流式返回，false为一次性返回完整结果

#### 使用：
实例直接调用即可。

<hr>

### 12. 添加询问消息
#### 定义：
```cpp
void Push_Back_Message(const std::string& str_role, const std::string& str_content)
```
#### 参数：
  1. **const std::string&**: 指定此消息是什么角色的
  2. **const std::string&**：具体的消息内容

#### 使用：
实例直接调用即可。

<hr>

### 13. 添加消息：
#### 定义：
```cpp
void Messages(const json::array_t& _arr_messages)
```
#### 参数：
  1. **const json::array_t&**：一个数组，数组中添加消息

#### 使用：
实例直接调用即可。

<hr>

### 14. 设置惩罚度：
#### 定义：
```cpp
void FrequencyPenalty(const float f_frequency_penalty)

void PresencePenalty(const float f_presence_penalty)
```
#### 参数：
  1. **const float**：设置重复惩罚，值为-2.0 ~ 2.0，数值越大越不重复

#### 使用：
实例直接调用即可，二者通常只使用其一。

<hr>

### 15. 用户使用信息统计：
#### 定义：
```cpp
void User(const std::string& str_user)
```
#### 参数：
  1. **const std::string&**：设置user，此键并不是某些AI厂商的标准，而是某些中转站的需要，它往往用于携带用户标识信息

#### 使用：
实例直接调用即可，AI是否支持请使用的站点的相关文档

<hr>

### 16. 向Json添加自定义字段
#### 定义：
```cpp
template <typename _T_Param>
void AddCustomFieldToJson(const std::string& str_key, const _T_Param& _t_value)
```
#### 参数：
  1. **const std::string&**：json字段的键
  2. **const _T_Param&**：一个任意类型

#### 使用：
```cpp
instance->AddCustomFieldToJson("test1", false);
instance->AddCustomFieldToJson("test2", "hello");
insatane->AddCustomFieldTOJson("test3", 2);
...more...
```
第二个参数类型不限，但是添加不一定有效。

<hr>

### 17. 向Json添加自定义字段
#### 定义：
```cpp
template <typename ... Args>
void AddCustomFieldToJson(int nKeyValuePairNumber, Args... args)
```
#### 参数：
  1. **int**：键值对的数量，数值为不超过16的正整数；
  2. **Args...**：键值对

#### 使用：
```cpp
instance->AddCustomFieldToJson(3,
    std::pair<std::string, int>("test1", 2),
    std::pair<std::string, bool>("test2", false),
    std::pair<std::string, std::string>("test3", "hello"));
```
pair的第一个参数实例化必须为std::string, 第二个参数不限制类型，但是参数不一定有效。

<hr>

### 18. 获取请求json对象
#### 定义：
```cpp
const json GetRequestJson()
```
#### 参数：
无参数
#### 使用：
实例直接调用即可

<hr>

### 19. 输出请求json对象
#### 定义：
```cpp
void PrintRequestJson_CMD(unsigned int un_Count = 0)
```
#### 参数：
  1. **unsigned int**：控制json对象输出格式的参数

#### 使用：
实例直接调用即可，请确保程序为控制台程序

<hr>

**[⏫返回顶部](#top)**