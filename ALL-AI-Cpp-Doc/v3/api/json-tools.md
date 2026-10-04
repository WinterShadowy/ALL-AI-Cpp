# JSON 操作工具 (Json Operator Tools)

`ALL_AI::JsonOperator` 命名空间包含用于构建请求 (`JsonRequestBuilder`) 和解析响应
(`JsonResponseParser`) 的策略类；`ALL_AI` 命名空间提供无状态取值自由函数 `JsonGet`、
辅助工具 `JsonOperatorTools`；`ALL_AI::FileOperator` 命名空间提供多模态构建器
`ContentPartBuilder`。

## 命名空间
`ALL_AI::JsonOperator` / `ALL_AI` / `ALL_AI::FileOperator`

## 包含头文件
```cpp
#include "ALL-AI-V3.hpp"
```

---

## 无状态取值：JsonGet（推荐）

```cpp
template <typename _T_Type, typename... Args>
_T_Type ALL_AI::JsonGet(const nlohmann::json& data, Args&&... keys);
```

对传入的 json 按路径**一行安全取值**，无需像 `JsonResponseParser` 那样先 `Parse` 再 `GetValue`。

- **路径键**: 字符串（对象键）与整数（数组下标）可混用；
- **失败语义**: 路径不存在或类型不匹配时按错误抛出方式处理并返回 `T{}`（空值），
  **不会像直接下标取值那样抛出 `type_error` 异常**。

```cpp
std::string id      = ALL_AI::JsonGet<std::string>(resp, "id");
std::string content = ALL_AI::JsonGet<std::string>(resp, "choices", 0, "message", "content");
```

> 这是读取响应的首选方式。下文的 `JsonResponseParser` 是 `JsonGet` 的底层引擎，
> 需要连续读取同一响应的大量字段时也可以直接使用它。

---

## 策略接口 (Interfaces)

### IRequestBuilderStrategy
请求构建策略接口。
```cpp
class IRequestBuilderStrategy : public ThrowError {
public:
    virtual nlohmann::json BuilderToJson() = 0;	// 导出当前构建的 JSON（副本）
    virtual void ClearBuilder() = 0;			// 清空构建器
    virtual nlohmann::json GetEmptyBuilder() = 0;	// 获取空 JSON 对象
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

### BuilderToJson
```cpp
virtual nlohmann::json BuilderToJson() override;
```
导出当前构建的 JSON 对象（持锁返回副本）。

### ClearBuilder
```cpp
virtual void ClearBuilder() override;
```
清空当前构建器内容，重置为空 JSON 对象。

### SetValue (Deep Set)
```cpp
template <typename _T_Value, typename... Args>
bool SetValue(_T_Value value, Args... keys);
```
递归地设置 JSON 字段的值。支持深层嵌套路径，若路径上的中间层不存在会自动创建（如果类型允许）。
- **value**: 要设置的值 (例如 `int`, `string`, `bool`, `nlohmann::json` 等)。
- **keys**: 必须是字符串，表示 JSON 对象的键路径。
- **示例**:
  ```cpp
  builder.SetValue("gpt-3.5-turbo", "model");
  builder.SetValue(0.7, "temperature");
  // 仅支持对象递归构造，不支持自动数组扩容
  // 必须确保 intermediate path 是 Object 类型
  ```

构建器还提供对 JSON 数组元素的操作（增、删、查），参见 `Demo/DemoCN(ArrayDemo-V3.cpp)`。

---

## JsonResponseParser

`JsonGet` 的底层引擎：有状态解析器（Parse + GetValue 两步用法）。
需要连续读取同一响应的多字段时，一次 Parse 后多次 GetValue 比逐次 JsonGet 略省一次解析。

### Parse
```cpp
virtual void Parse(const nlohmann::json& response) override;
virtual void Parse(const std::string& response) override;
```
加载并解析 JSON 数据。字符串版本解析失败会触发错误处理。

### GetData
```cpp
virtual nlohmann::json GetData() override;
```
获取完整的解析后的 JSON 对象。

### GetValue (Deep Get)
```cpp
template <typename _T_Type, typename... Args>
_T_Type GetValue(Args... keys);
```
递归地获取 JSON 字段的值，路径与失败语义和 `JsonGet` 完全一致。

---

## JsonOperatorTools

用于辅助构建复杂 JSON 结构的工具类（通过 `ai.GetTools()` 获取），主要用于管理消息历史。

### Role 枚举
```cpp
enum class Role {
    System,
    User,
    Assistant
};
```
映射到 JSON 字符串: `"system"`, `"user"`, `"assistant"`。
也可使用宏 `ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM / _USER / _ASSISTANT`。

### PushBackArray / PopBackArray / GetMessagesArray
```cpp
void PushBackArray(const Role& role, const std::string& content);	// 追加 {"role","content"} 消息
void PopBackArray();												// 移除最后一条
nlohmann::json::array_t& GetMessagesArray();						// 获取消息数组引用
```

### FileToBase64（静态）
```cpp
static std::string JsonOperatorTools::FileToBase64(const std::string& file_path);
```
读取本地文件并 Base64 编码，失败返回空字符串。组装多模态 data URI 的基础工具。

---

## ContentPartBuilder（多模态内容构建）

`ALL_AI::FileOperator::ContentPartBuilder` 是 Builder 模式的链式构建器，
用于手工组装多模态 user 消息：

```cpp
nlohmann::json user_message = ALL_AI::FileOperator::ContentPartBuilder()
	.AddText(u8"描述这张图片，并转写这段音频")
	.AddImageBase64("girl.png")		// 本地图片 -> data:image/png;base64,... 的 image_url part
	.AddAudioBase64("voice.mp3")	// 本地音频 -> data:audio/mpeg;base64,... 的 audio_url part
	.BuildUserMessage();			// {"role":"user","content":[...parts]}
```

| 方法 | 输入 | 生成的 part | 读文件失败时 |
| --- | --- | --- | --- |
| `AddText(text)` | 字符串 | `{"type":"text", ...}` | —— |
| `AddImageBase64(path)` | 本地图片 | `image_url`（data URL） | 不添加该 part |
| `AddImageFileId(file_id)` | 已上传 ID | `image_url`（`ms://` 引用） | —— |
| `AddVideoFileId(file_id)` | 已上传 ID | `video_url`（`ms://` 引用） | —— |
| `AddAudioBase64(path)` | 本地音频 | `audio_url`（data URL） | 不添加该 part |
| `BuildParts()` | —— | parts 数组 | —— |
| `BuildUserMessage()` | —— | 完整 user 消息 json | —— |

> 站点差异提示（实测）：硅基流动网关接受 OpenAI 约定的 `audio_url` 音频 part，
> 拒绝上游 DashScope 风格的 `input_audio` part。切换站点时请按站点文档选择 part 格式，
> 差异部分可用 `JsonOperatorTools::FileToBase64` 手工组装，详见
> [站点适配指南](/v3/endpoint-abstraction.md)。

---

## 错误处理基类 (ThrowError)

`JsonRequestBuilder` 和 `JsonResponseParser` 均继承自 `ThrowError`。
- **SetThrowErrorCallbackFunction**: 为这些组件单独设置错误回调（通常由 `AI` 类统一管理）。
