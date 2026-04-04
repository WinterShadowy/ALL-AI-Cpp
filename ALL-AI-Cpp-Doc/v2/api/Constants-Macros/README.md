<a id="top"></a>
<style>
.right-align {
    text-align: right;
}
</style>

## 宏的含义

### AI角色宏

<table style="border: 2px rgb(0, 0, 0); border-collapse: collapse;">
    <thead>
        <tr>
            <th style="border: 2px solid rgb(0, 0, 0); padding: 8px;">宏名</th>
            <th style="border: 2px solid rgb(0, 0, 0); padding: 8px;">值</th>
            <th style="border: 2px solid rgb(0, 0, 0); padding: 8px;">释义</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">ROLE_USER</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">"user"</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">参数中使用此宏时，表明此内容是用户向AI询问的</td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">ROLE_SYSTEM</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">"system"</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">参数中使用此宏时，此宏用于设置AI的角色信息</td>
        </tr>
         <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">ROLE_ASSISTANT</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">"assistant"</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">参数中使用此宏时，此宏用于标识内容，被标识的内容视为AI的回复</td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">ROLE_USER_W</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">L"user"</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">参数中使用此宏时，表明此内容是用户向AI询问的</td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">ROLE_SYSTEM_W</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">L"system"</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">参数中使用此宏时，此宏用于设置AI的角色信息</td>
        </tr>
         <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">ROLE_ASSISTANT_W</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">L"assistant"</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">参数中使用此宏时，此宏用于标识内容，被标识的内容视为AI的回复</td>
        </tr>
    </tbody>
</table>

**如果你不知道字符串前带有“L”是什么含义，你可以参考我的知识分享站的文章：**[C++ 中四种字符串字面量的区别](https://wang-sz.cn/1311.html)

### 功能宏

<table style="border: 2px rgb(0, 0, 0); border-collapse: collapse;">
    <thead>
        <tr>
            <th style="border: 2px solid rgb(0, 0, 0); padding: 8px;">宏名</th>
            <th style="border: 2px solid rgb(0, 0, 0); padding: 8px;">值</th>
            <th style="border: 2px solid rgb(0, 0, 0); padding: 8px;">释义</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_COMPLETIONS</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">0U</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用<strong>补全</strong></td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_EDITS</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">1U</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用<strong>编辑</strong></td>
        </tr>
         <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_MODEL_LIST</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">2U</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用<strong>模型列表</strong>，可以列出中转站支持的模型列表</td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_CHAT</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">3U</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用<strong>聊天</strong></td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_IMAGES</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">4U</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用<strong>绘图</strong></td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_FILE</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">5U</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用<strong>文件</strong></td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_EMBEDDINGS</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">6U</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用<strong>嵌入</strong></td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_FINETUNES</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">7U</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用<strong>微调</strong></td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_AUDIO</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">8U</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用<strong>音频</strong></td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_MODERATION</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">9U</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用<strong>视频</strong></td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_MODERATION</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">10U</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用<strong>审核</strong></td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_SELF_DEFINED</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">20U</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用<strong>自定义</strong>，你可以发送自定义的<strong>json</strong>对象</td>
        </tr>
    </tbody>
</table>

### 请求方式宏

<table style="border: 2px rgb(0, 0, 0); border-collapse: collapse;">
    <thead>
        <tr>
            <th style="border: 2px solid rgb(0, 0, 0); padding: 8px;">宏名</th>
            <th style="border: 2px solid rgb(0, 0, 0); padding: 8px;">值</th>
            <th style="border: 2px solid rgb(0, 0, 0); padding: 8px;">释义</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_CUSTOM_REQUEST_POST</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">"POST"</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用请求方式为<strong>POST</strong></td>
        </tr>
        <tr>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">AI_CUSTOM_REQUEST_GET</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">"GET"</td>
            <td style="border: 2px solid rgb(0, 0, 0); padding: 8px;">发送请求时使用此宏，表示使用请求方式为<strong>GET</strong></td>
        </tr>
    </tbody>
</table>

<hr>

**[⏫回到顶部](#top)**  |  **[下一页⏩](/v2/api/class/)**
