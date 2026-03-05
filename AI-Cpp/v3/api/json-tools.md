# JSON 操作工具 (Json Operator Tools)

`ALL_AI::JsonOperator` 命名空间包含用于构建请求 (`JsonRequestBuilder`) 和解析响应 (`JsonResponceParser`) 的策略类，以及辅助工具 `JsonOperatorTools`。

## 命名空间
`ALL_AI::JsonOperator` 或 `ALL_AI`

## 包含头文件
```cpp
#include "ALL-AI-V3.hpp"
```

---

## 策略接口 (Interfaces)

### IRequestBuilderStrategy
请求构建策略接口。
```cpp
class IRequestBuilderStrategy : public ThrowError {
public:
    virtual nlohmann::json GetBuilder() = 0;
    virtual void ClearBuilder() = 0;
    virtual nlohmann::json GetEmptyBuilder() = 0;
};
```

### IResponseParserStrategy
响应解析策略接口。
```cpp
class IResponseParserStrategy : public ThrowError {
public:
    virtual void Parse(const nlohmann::json& response) = 0;
    virtual void Parse(const std::string& response) = 0;
    virtual nlohmann::json GetData() = 0;
};
```
所有策略类都继承自 `ThrowError`，支持统一的错误回调机制。

---

## JsonRequestBuilder

用于采用链式或分步方式构建 JSON 请求体。

### GetBuilder
```cpp
virtual nlohmann::json GetBuilder() override;
```
获取当前构建的 JSON 对象。

### ClearBuilder
```cpp
virtual void ClearBuilder() override;
```
清空当前构建器内容，重置为空 JSON 对象。

### SetValue (Deep Set)
```cpp
template <typename _T_Value, typename... Args>
bool SetValue(_T_Value _value, Args... _keys);
```
递归地设置 JSON 字段的值。支持深层嵌套路径，若路径上的中间层不存在会自动创建（如果类型允许）。
- **_value**: 要设置的值 (例如 `int`, `string`, `bool`, `nlohmann::json` 等)。
- **_keys**: 必须是字符串，表示 JSON 对象的键路径。
- **示例**:
  ```cpp
  builder.SetValue("gpt-3.5-turbo", "model");
  builder.SetValue(0.7, "temperature");
  // 仅支持对象递归构造，不支持自动数组扩容
  // 必须确保 intermediate path 是 Object 类型
  ```
> **注意**: 该实现内部使用了 `_json[key] = val` 的形式，对于 `nlohmann::json`，如果键不存在会自动创建。

---

## JsonResponceParser

用于解析 API 返回的 JSON 数据，并安全地获取字段值。

### Parse
```cpp
virtual void Parse(const nlohmann::json& response) override;
virtual void Parse(const std::string& response) override;
```
加载并解析 JSON 数据。如果是字符串版本，解析失败会触发错误处理（抛出异常或回调）。

### GetData
```cpp
virtual nlohmann::json GetData() override;
```
获取完整的解析后的 JSON 对象。

### GetValue (Deep Get)
```cpp
template <typename _T_Type, typename... Args>
_T_Type GetValue(Args... _keys);
```
递归地获取 JSON 字段的值。
- **_T_Type**: 期望返回的数据类型 (例如 `std::string`, `int`, `double`)。
- **_keys**: 路径参数，可以是 `std::string` (对象键) 或 `int` (数组索引)。
- **错误处理**: 
  - 如果路径不存在或类型不匹配。
  - 若配置为 `ALL_AI_EXCEPTION_THROWING`，抛出 `std::out_of_range`。
  - 若配置为 `ALL_AI_CALLBACK_FUNCTION`，调用回调并返回空/默认值。
- **示例**:
  ```cpp
  // 获取 choices[0].message.content
  std::string content = parser.GetValue<std::string>("choices", 0, "message", "content");
  ```

---

## JsonOperatorTools

用于辅助构建复杂 JSON 结构的工具类，主要用于管理消息历史。

### Role 枚举
```cpp
enum class Role {
    System,
    User,
    Assistant
};
```
映射到 JSON 字符串: `"system"`, `"user"`, `"assistant"`。

### GetMessagesArray
```cpp
nlohmann::json::array_t& GetMessagesArray();
```
获取当前管理的消息数组引用。

### PushBackArray
```cpp
void PushBackArray(const Role& _role, const std::string& _content);
```
向消息数组末尾添加一条消息。
- **格式**: `{"role": "...", "content": "..."}`。

### PopBackArray
```cpp
void PopBackArray();
```
移除消息数组中的最后一条消息。

---

## 错误处理基类 (ThrowError)

`JsonRequestBuilder` 和 `JsonResponceParser` 均继承自 `ThrowError`。
- **SetThrowErrorCallbackFunction**: 为这些组件单独设置错误回调（通常由 `AI` 类统一管理，但也可以单独设置）。
