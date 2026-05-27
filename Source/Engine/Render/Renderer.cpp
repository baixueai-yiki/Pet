#include "Engine/Render/Renderer.h"

namespace pet::engine::render {

void Renderer::Begin(HDC hdc) {
    (void)hdc;
}

void Renderer::End(HDC hdc) {
    (void)hdc;
}

void Renderer::Clear(HDC hdc, COLORREF color, const RECT& rc) {
    HBRUSH b = CreateSolidBrush(color);
    FillRect(hdc, &rc, b);
    DeleteObject(b);
}

void Renderer::DrawRect(HDC hdc, const RECT& rc, COLORREF color) {
    HBRUSH b = CreateSolidBrush(color);
    FillRect(hdc, &rc, b);
    DeleteObject(b);
}

void Renderer::DrawTextLine(HDC hdc, int x, int y, const wchar_t* text, COLORREF color) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    TextOutW(hdc, x, y, text, lstrlenW(text));
}

} // namespace pet::engine::render
