#pragma once
#include <string>
#include <windows.h>

///////////////////////////////////////////////////////////////////////////////
// 聊天输入模块（文本 + UI 按钮共同使用的辅助库）
// 该模块可以处理 `WM_CHAR` 里的回车/退格/普通字符，并支持按键与按钮共用的配置映射。
///////////////////////////////////////////////////////////////////////////////

// 文本输入具体结果，用来指导输入框是否重绘或提交
enum class ChatInputAction
{
    None,        // 末发生变化
    TextChanged, // 内容变了，需要刷新显示
    Submit,      // 按下回车，应提交输入
};

// 处理聊天输入框中的字符事件（回车/退格/文字）
ChatInputAction ProcessChatInputChar(std::wstring& text, WPARAM key);

// 按钮输入事件，适合那些 UI 上的点击交互
enum class ButtonInputEvent
{
    None,
    Press,
    Release,
};

// 保存按钮状态的简单双值
struct ButtonInputState
{
    bool isDown = false;
    bool wasDown = false;
};

// 更新一个按钮的状态并返回事件，`key` 参数当前未使用，仅为未来扩展准备
ButtonInputEvent ProcessButtonInput(ButtonInputState& state, WPARAM key, bool isDown);

// chat 配置文件解析（类型为 text/button/default）
bool LoadChatConfig(const std::wstring& configPath);
const std::wstring* GetChatTextResponse(const std::wstring& key);
const std::wstring* GetChatButtonResponse(const std::wstring& buttonId);
const std::wstring* GetDefaultChatResponse();
// 每次处理前调用，可在运行时自动重新加载已选中的配置文件（基于文件写入时间）
void MaybeReloadChatConfig();
