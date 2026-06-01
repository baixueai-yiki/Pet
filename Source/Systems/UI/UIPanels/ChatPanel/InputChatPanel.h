#pragma once
#include <windows.h>
#include <string>

class InputChatPanel
{
public:
    static void Show(HWND hwndParent);
    static bool IsVisible();
    static void Hide();
    static void UpdatePosition();
};
