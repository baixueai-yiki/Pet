#pragma once

#include <string>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

///////////////////////////////////////////////////////////////////////////////
// Setting 模块：统一读取 settings.json，将设置分组、分类，并提供内嵌面板与快捷修改接口。
// 按照虚幻引擎思路将配置与渲染输入等系统独立出来，避免其他模块重复实现。
///////////////////////////////////////////////////////////////////////////////
namespace Setting
{
    // Retrieve a named entry or return defaultValue if missing.
    std::wstring GetString(const std::wstring& key, const std::wstring& defaultValue);
    int GetInt(const std::wstring& key, int defaultValue);

    bool ShowOverlay();
    void HideOverlay();
    bool ToggleOverlay();
    bool IsOverlayVisible();
    bool TryApplyInlineValue(const std::wstring& command);
    void RenderOverlay(HDC hdc);
    bool HandleOverlayMouse(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool IsPointInsideOverlay(int x, int y);
}

