<a id="top"></a>

## AI类：

<hr>

### 1. 构造函数：
#### 定义：
```cpp
AI(const std::string& url_base, const std::string& api_key, bool b_UseContext = false)
```
#### 参数：
  1. **const std::string&**：一个站点URL
  2. **const std::string&**：一个API密钥
  3. **bool**：是否使用上下文，早期开发者调试使用，现已弃用

#### 使用：
初始化时使用

<hr>

### 2. 析构函数：
#### 定义：
```cpp
~AI()
```
#### 参数：
无参数
#### 使用：
实例析构时释放资源

<hr>

### 3. 发送请求：
#### 定义：
```cpp
json SendRequest(const json json_request, UINT unFunction)
```
#### 参数：
  1. **const json**：一个json对象，发送向站点的请求
  2. **UINT**：请求识别用，识别是使用AI的什么功能（聊天、绘画等）

#### 使用：
实例直接调用即可。

<hr>

### 4. 以GET方式发送请求：
#### 定义：
```cpp
json SendRequest_GET()
```
#### 参数：
无参数
#### 使用
实例直接调用即可

<hr>

### 5. 以POST方式发送请求：
#### 定义：
```cpp
json SendRequest_POST(const json& json_request)
```
#### 参数：
  1. **const json&**：一个发送向服务器的json对象

#### 使用：
实例直接调用即可

<hr>

### 6. 重新初始化AI类：
#### 定义：
```cpp
void Reinitialize(const std::string& url_base, const std::string& api_key)
```
#### 参数：
  1. **const std::string&**：一个字符串，站点URL
  2. **const std::string&**：一个字符串，你的API-KEY

#### 使用：
实例直接调用即可

<hr>

### 7. 设置AI的功能
#### 定义：
```cpp
Completions* Set_Completions()
Edits* Set_Edits()
ModelList* Set_ModelList()
Chat* Set_Chat()
Images* Set_Images()
File* Set_File()
Embeddings* Set_Embeddings()
FineTunes* Set_FineTunes()
Audio* Set_Audio()
Video* Set_Video()
Moderation* Set_Moderation()
```
#### 参数：
均无参数
#### 使用：
实例直接调用即可，返回某个功能的对象

<hr>

**[⏫返回顶部](#top)**