#pragma once
#include <windows.h>
#include <string>

// 弹出可输入文本的窗口
void ChatShowInput(HWND hwndParent);
void ChatShowButtonInput(HWND hwndParent, const std::wstring& key1, const std::wstring& key2);
// 根据输入内容执行相应的对话逻辑
void ChatHandleInput(HWND hwnd, const std::wstring& input);
