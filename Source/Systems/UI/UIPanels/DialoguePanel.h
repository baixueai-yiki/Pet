#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <tuple>

// 屏幕底部对话框——纯 UI，只接收并显示文本列表
class DialoguePanel
{
public:
    // key, text, mood
    using Entry = std::tuple<std::wstring, std::wstring, std::wstring>;

    static void Show(const std::vector<Entry>& entries);
    static void AdvanceOrHide();
    static bool IsVisible();
    static void Hide();
};
