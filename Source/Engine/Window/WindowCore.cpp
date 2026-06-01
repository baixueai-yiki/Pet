#include "Engine/Window/WindowCore.h"

namespace pet::engine::window {

void WindowCore::ApplyStyle(HWND hwnd, const WindowStyle& style) {
    if (!hwnd) return;

    SetWindowLongPtrW(hwnd, GWL_STYLE, static_cast<LONG_PTR>(style.style));
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, static_cast<LONG_PTR>(style.exStyle));

    if (style.useColorKey) {
        SetColorKey(hwnd, style.colorKey);
    } else {
        SetAlpha(hwnd, style.alpha);
    }
    SetTopMost(hwnd, style.topMost);
    SetClickThrough(hwnd, style.clickThrough);

    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void WindowCore::SetTopMost(HWND hwnd, bool topMost) {
    if (!hwnd) return;
    SetWindowPos(hwnd, topMost ? HWND_TOPMOST : HWND_NOTOPMOST,
        0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void WindowCore::SetClickThrough(HWND hwnd, bool clickThrough) {
    if (!hwnd) return;
    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (clickThrough) {
        ex |= WS_EX_TRANSPARENT;
    } else {
        ex &= ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT);
    }
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
}

void WindowCore::SetAlpha(HWND hwnd, BYTE alpha) {
    if (!hwnd) return;
    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if ((ex & WS_EX_LAYERED) == 0) {
        ex |= WS_EX_LAYERED;
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
    }
    SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
}

void WindowCore::SetColorKey(HWND hwnd, COLORREF colorKey) {
    if (!hwnd) return;
    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if ((ex & WS_EX_LAYERED) == 0) {
        ex |= WS_EX_LAYERED;
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
    }
    SetLayeredWindowAttributes(hwnd, colorKey, 0, LWA_COLORKEY);
}

} // namespace pet::engine::window
