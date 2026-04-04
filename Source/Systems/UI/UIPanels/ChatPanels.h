#pragma once

#include "../UIActor.h"
#include <windows.h>
#include <string>

class ChatPanels
{
public:
    static void Setup(UIActor& actor);
    static void ShowInput(HWND hwndParent);
    static void ShowButtonInput(HWND hwndParent, const std::wstring& key1, const std::wstring& key2);
    static void Talk(HWND hwnd, const wchar_t* text);
    static void ShowTaskList(HWND hwndParent);
    static void HandleInput(HWND hwnd, const std::wstring& input);
    static void HandleButtonInput(HWND buttonWnd, const std::wstring& key);
    static void RecordInteraction();
};
