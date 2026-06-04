#pragma once
#include <windows.h>

class BubbleChatPanel
{
public:
    static void Show(HWND hwndParent, const wchar_t* text, int durationMs = 0);
    static void UpdatePosition();
};
