#include "ChatPanelInternal.h"

HFONT g_inputFont = nullptr;
HFONT g_talkFont = nullptr;

void EnsureFonts()
{
    if (!g_inputFont)
    {
        g_inputFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Microsoft YaHei UI");
    }
    if (!g_talkFont)
    {
        g_talkFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Microsoft YaHei UI");
    }
}

void ShowWindowEx(HWND& wnd, const wchar_t* cls, WNDPROC proc, HWND parent)
{
    if (wnd)
        return;

    WNDCLASSW wc = {};
    wc.lpfnWndProc = proc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = cls;
    RegisterClassW(&wc);

    wnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        cls, nullptr, WS_POPUP,
        0, 0, 10, 10,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);
    SetLayeredWindowAttributes(wnd, 0, 235, LWA_ALPHA);
}
