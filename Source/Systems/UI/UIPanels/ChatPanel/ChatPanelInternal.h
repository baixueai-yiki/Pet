#pragma once
#include <windows.h>
#include <windowsx.h>

constexpr int kInputWidth = 300;
constexpr int kInputHeight = 40;
constexpr int kButtonHeight = 80;
constexpr int kTalkPadding = 8;
constexpr int kTalkMaxWidth = 240;
constexpr UINT_PTR kTalkAutoHideTimer = 1;
constexpr UINT kTalkAutoHideMs = 3000;

extern HFONT g_inputFont;
extern HFONT g_talkFont;

void EnsureFonts();
void ShowWindowEx(HWND& wnd, const wchar_t* cls, WNDPROC proc, HWND parent);
