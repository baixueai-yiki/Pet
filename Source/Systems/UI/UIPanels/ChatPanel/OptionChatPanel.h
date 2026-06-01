#pragma once
#include <windows.h>
#include <string>

class OptionChatPanel
{
public:
    static void Show(HWND hwndParent, const std::wstring& key1, const std::wstring& key2);
    static bool IsVisible();
    static void Hide();
    static void UpdatePosition();
};
