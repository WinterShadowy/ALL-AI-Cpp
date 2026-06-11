# V3 更新日志

用于记录 ALL-AI-Cpp V3 文档与代码的关键更新，便于追踪功能演进与兼容性变化。

## 2026-06-11
```txt
Added/新增:
1. 优化了错误抛出逻辑
2. 优化了部分代码，提升了代码性能

Changed/修改:
1. 修改了大量的错误抛出与不必要的拷贝语句

Fixed/修正:
无

Deleted/删除:
无

```

## 2026-06-09
```txt
Added/新增:
1. 新增构建器对数组的元素的操作（增，删，查）
2. 新增了DoErrorThrow函数执行错误抛出（暂无法使用，等待后续优化）
3. 新增了数组操作的Demo

Changed/修改:
1. 修改了ThrowError类的错误抛出方式

Fixed/修正:
无

Deleted/删除:
1. 删除了AppendToArray函数，删除原因：函数名表述不清楚

```

## 2026-06-06
```txt
Added/新增:
1. 新增构建器清空json数组的函数
2. 新增构建器类的获取json函数(BuilderToJson()函数)

Changed/修改:
无

Fixed/修正:
1. 弃用了构建器类中GetBuilder函数，此函数名在使用中可能存在表述不准确的问题

Deleted/删除:
无

```

## 2026-05-26
```txt
Added/新增:
无

Changed/修改:
修改了nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json)函数中的部分逻辑

Fixed/修正:
无

Deleted/删除:
1. _setValue函数删除

```

## 2026-05-09
```txt
Added/新增:
1. 新增构建器对json数组的操作
2. 新增构建器对复杂对象的操作逻辑
3. 新增对旧版构建器的兼容

Changed/修改:
1. 更新了Dmeo，支持简体中文、英语demo（无繁体中文是因为开发者不太懂繁中！）
2. 修改了旧版构建器对复杂对象的操作逻辑
3. 修改了 SetValue 函数对复杂对象构建的逻辑

Fixed/修正:
1. 修正 SetValue 函数直接操作数组时的错误

Deleted/删除:
1. _setValue函数已弃用（未删除，下次更新将删除部分冗余代码）

```

<hr>

## 2026-04-03

```txt
Added/新增:
1. 新增设计架构页面：[design-architecture.md](/v3/design-architecture.md)。
2. 新增更新日志页面：[changelog.md](/v3/changelog.md)。
3. 设计架构页面补充 ASCII 架构图与 SSE 处理链路图。
4. 侧边栏新增“设计架构 / 更新日志”入口。
5. V3 首页目录新增“更新日志”入口。
6. 新增流式示例文件：Demo/ChatDemo-V3-Stream.cpp。

Changed/修改:
1. include/ALL-AI-V3.hpp 中 CurlHttpTransport::SendRequest(...) 已支持根据 stream 字段切换 Accept 头。
2. include/ALL-AI-V3.hpp 中新增 SSE 回退解析逻辑（TryParseSseResponse(...)）。
3. SSE 解析结果会保留 sse_chunks，并合并 choices[].message.content 供现有读取方式使用。
4. 设计架构文档已补充 SSE 相关函数替换建议与迁移顺序。

Fixed/修正:
1. 修复 stream=true 时将 SSE 文本直接按 JSON 解析导致的报错问题。

Deleted/删除:
无

```
<hr>

## 2026-03-29

```txt
Added/新增:
无

Changed/修改:
1. 更新了Dmeo

Fixed/修正:
无

Deleted/删除:
无

```