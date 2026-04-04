<a id="top"></a>

## content类：

<hr>

### 1. 构造函数：
#### 定义：
```cpp
content(const json::array_t arr_Replies)
```
#### 参数：
  1. **const json::array_t**，它可以简单理解为被封装过的vector，详细的内容需要您参阅nlohmann/json.hpp的内容

<hr>

### 2. 析构函数：
#### 定义：
```cpp
~content()
```

<hr>

### 3. 获取回复函数：
#### 定义：
```cpp
template <typename _T>
_T Reply(unsigned int index = 0)
```
#### 参数：
  1. **unsigned int**，一个无符号整型，回复的json对象可能携带多个回复。本函数不进行错误检测，访问回复不存在则会返回空字符串。
#### 使用：
使用时需要显示模板实例化为std::string或std::wstring

<hr>

### 4. 获取可使用的下标的最大值：
#### 定义：
```cpp
size_t GetMaxIndex()
```
#### 参数：
无参数。
#### 使用：
实例直接调用即可。函数执行成功返回可使用的下标最大值，失败则返回(size_t)(-1)

<hr>

### 5. 判断回复是否存在
#### 定义：
```cpp
bool ContentEmpty()
```
#### 参数：
无参数。
#### 使用：
实例直接调用，若无回复则返回true，否则返回false

<hr>

### 6. 获取回复数量：
#### 定义：
```cpp
size_t GetContentSize()
```
#### 参数：
无参数
#### 使用：
实例直接调用即可。函数执行成功返回回复数，失败则返回(size_t)0

<hr>

### 7. 获取回复中的URL：
#### 定义：
```cpp
template <typename _T>
 _T GetURLFromReply(int index = 0)
```
#### 参数：
  1. **int**，指定获取回复中的第（index + 1）个URL
#### 使用：
显示模板实例化时，如果实例化为int，则参数无效，函数返回一个URL总数，如果实例化为std::string或者是std::wstring，函数返回第（index + 1）个URL

<hr>

### 8. 获取回复中的所有URL:
#### 定义：
```cpp
template<typename _OutputContainer>
_OutputContainer GetAllURL_FromReplies()
```
#### 参数：
无参数。
#### 使用：
显示模板实例化时，需要实例化为容器（vector、array、set），函数会将当前回复中的URL都加容器中并回复

<hr>

### 9. 获取角色
#### 定义：
```cpp
template <typename _T>
_T GetRole(unsigned int index = 0)
```

#### 参数：
  1. **unsigned int**： 获取第（index+1）个回复中的role信息。

#### 使用：
显示模板实例化时，需要实例化为std::string或是std::wstring，否会函数返回空内容。

<hr>

### 10. 获取完成原因
#### 定义：
```cpp
template <typename _T>
_T GetFinishReason(unsigned int index = 0)
```
#### 参数：
无参数
#### 使用：
显示模板实例化时，需要实例化为std::string或std::wstring，否则函数返回空内容。

<hr>

**[⏫返回顶部](#top)**