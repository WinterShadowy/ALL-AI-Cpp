# 2025/08/14
## 新增
1. 新增了Linux系统的使用（测试环境：Ubuntu 22），引入方式和Windows环境相同;
2. 新增了错误代码抛出功能:
```cpp
// Check HTTP response code
long http_code = 0;
curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
if (http_code < 200 || http_code >= 300)
{
    std::string error_message = "HTTP error: " + std::to_string(http_code) + ", Response: " + str_Buffer;
    curl_slist_free_all(headers);
    throw std::runtime_error(error_message);
}
```
代码段位于`Session`类中的`MakeRequest`函数内;
3. 新增了空返回的错误抛出:
```cpp
// Check if response is empty
if (str_Buffer.empty())
{
    curl_slist_free_all(headers);
    throw std::runtime_error("Error: Session: Empty response from server");
}
```
代码段位于`Session`类中的`MakeRequest`函数内。

## 删除
无

## 修改
1. 修改了字符集处理函数的处理逻辑;
2. 修改了Linux环境下某些功能的bug;
3. 修改了字符集处理函数;
4. 修改了用户文档的相关内容。

<hr>

# 2025/07/03
## 新增
1. 创建了仓库并上传了相关文件；
2. 部署了相关的用户文档。

## 删除
无

## 修改
无