#pragma once
#include <windows.h>
#include <string>

// 弹出可输入文本的窗口
void ChatShowInput(HWND hwndParent);
void ChatShowButtonInput(HWND hwndParent, const std::wstring& key1, const std::wstring& key2);
// 宠物移动时调用，用来同步输入栏位置
void ChatUpdateInputPosition();
// 宠物移动时调用，用来同步回复气泡位置
void ChatUpdateTalkPosition();
// 宠物移动时调用，用来同步按钮栏位置
void ChatUpdateButtonPosition();
// 根据输入内容执行相应的对话逻辑
void ChatHandleInput(HWND hwnd, const std::wstring& input);
