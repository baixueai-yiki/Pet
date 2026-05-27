#pragma once

#include <windows.h>

namespace pet::engine::render {

class Renderer {
public:
    static void Begin(HDC hdc);
    static void End(HDC hdc);

    static void Clear(HDC hdc, COLORREF color, const RECT& rc);
    static void DrawRect(HDC hdc, const RECT& rc, COLORREF color);
    static void DrawTextLine(HDC hdc, int x, int y, const wchar_t* text, COLORREF color = RGB(255, 255, 255));
};

} // namespace pet::engine::render
