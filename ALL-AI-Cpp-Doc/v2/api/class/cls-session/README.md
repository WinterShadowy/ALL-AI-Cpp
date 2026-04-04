<a id="top"></a>

## Session类

<hr>

### 1. 构造函数：
#### 定义：
```cpp
Session(const std::string& url, const std::string& key)
```
#### 参数：
  1. **const std::string& url**: 中转站URL
  2. **const std::string& key**: 密钥字符串

#### 使用：
创建实例时，此构造函数会初始化一些信息。详细信息请参阅源码。

<hr>

### 2. 析构函数：
#### 定义：
```cpp
~Session()
```
#### 参数：
无参数
#### 使用：
无

<hr>

### 3. 发送请求-GET
#### 定义：
```cpp
json MakeRequest_GET()
```
#### 参数：
无参数
#### 使用：
实例直接调用即可，执行成功将以GET方式发送请求。

<hr>

### 4. 收发请求-POST
#### 定义：
```cpp
json MakeRequest_POST(const json& _json)
```

#### 参数：
  1. **const json&**: 一个发送向服务器的json对象

#### 使用：
实例直接调用，函数执行成功返回中转站返回的json对象，函数执行失败返回空json对象

<hr>

**[⏫返回顶部](#top)**