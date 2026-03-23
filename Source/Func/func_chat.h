#pragma once
#include <windows.h>
#include <string>

// 弹出输入气泡
void ShowChatInput(HWND hwnd);

// 处理输入文字
void HandleInput(HWND hwnd, const std::wstring& input);

// 对话逻辑
void Dialog_Love(HWND hwnd);
void Dialog_Unknown(HWND hwnd);