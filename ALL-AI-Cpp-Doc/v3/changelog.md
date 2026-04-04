# V3 更新日志

用于记录 ALL-AI-Cpp V3 文档与代码的关键更新，便于追踪功能演进与兼容性变化。


## 2026-04-03

### Added/新增

1. 新增设计架构页面：[design-architecture.md](/v3/design-architecture.md)。
2. 新增更新日志页面：[changelog.md](/v3/changelog.md)。
3. 设计架构页面补充 ASCII 架构图与 SSE 处理链路图。
4. 侧边栏新增“设计架构 / 更新日志”入口。
5. V3 首页目录新增“更新日志”入口。
6. 新增流式示例文件：`Demo/ChatDemo-V3-Stream.cpp`。

### Changed/修改

1. `include/ALL-AI-V3.hpp` 中 `CurlHttpTransport::SendRequest(...)` 已支持根据 `stream` 字段切换 `Accept` 头。
2. `include/ALL-AI-V3.hpp` 中新增 SSE 回退解析逻辑（`TryParseSseResponse(...)`）。
3. SSE 解析结果会保留 `sse_chunks`，并合并 `choices[].message.content` 供现有读取方式使用。
4. 设计架构文档已补充 SSE 相关函数替换建议与迁移顺序。

### Fixed/修正

1. 修复 `stream=true` 时将 SSE 文本直接按 JSON 解析导致的报错问题。

### Deleted/删除

无

<hr>

## 2026-03-29

### Added/新增

无

### Changed/修改

1. 更新了Dmeo

### Fixed/修正

无

### Deleted/删除

无