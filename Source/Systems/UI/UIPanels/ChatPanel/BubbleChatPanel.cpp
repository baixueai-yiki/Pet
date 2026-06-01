#include "BubbleChatPanel.h"
#include "ChatPanelInternal.h"
#include "../../../Pet/PetActor.h"
#include <string>

namespace
{
    static HWND s_hTalkWnd = nullptr;
    static std::wstring s_talkText;

    void MeasureTalkText(int maxWidth, int& outW, int& outH)
    {
        HDC hdc = GetDC(nullptr);
        RECT rc = { 0, 0, maxWidth, 0 };
        if (g_talkFont)
            SelectObject(hdc, g_talkFont);
        DrawTextW(hdc, s_talkText.c_str(), -1, &rc, DT_CALCRECT | DT_WORDBREAK);
        ReleaseDC(nullptr, hdc);
        outW = (rc.right - rc.left) + kTalkPadding * 2;
        outH = (rc.bottom - rc.top) + kTalkPadding * 2;
        if (outW < 80)
            outW = 80;
        if (outH < 32)
            outH = 32;
    }

    void PositionTalkWindow(HWND hwnd, int w, int h)
    {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int x = g_pet.x + (g_pet.w - w) / 2;
        int y = g_pet.y - h - 10;
        if (x < 0) x = 0;
        if (x + w > screenW) x = screenW - w;
        if (y < 0) y = g_pet.y + g_pet.h + 10;
        SetWindowPos(hwnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE);
    }

    LRESULT CALLBACK TalkProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_TIMER && wParam == kTalkAutoHideTimer)
        {
            KillTimer(hwnd, kTalkAutoHideTimer);
            DestroyWindow(hwnd);
            return 0;
        }
        if (msg == WM_PAINT)
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH brush = CreateSolidBrush(RGB(255, 250, 230));
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(220, 200, 170));
            HGDIOBJ oldBrush = SelectObject(hdc, brush);
            HGDIOBJ oldPen = SelectObject(hdc, pen);
            RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 10, 10);
            SelectObject(hdc, oldBrush);
            SelectObject(hdc, oldPen);
            DeleteObject(brush);
            DeleteObject(pen);
            if (g_talkFont)
                SelectObject(hdc, g_talkFont);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(50, 50, 50));
            RECT textRc = rc;
            textRc.left += kTalkPadding;
            textRc.top += kTalkPadding;
            textRc.right -= kTalkPadding;
            textRc.bottom -= kTalkPadding;
            DrawTextW(hdc, s_talkText.c_str(), -1, &textRc, DT_WORDBREAK);
            EndPaint(hwnd, &ps);
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void BubbleChatPanel::Show(HWND hwndParent, const wchar_t* text)
{
    s_talkText = text ? text : L"";
    EnsureFonts();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = TalkProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"ChatTalkWnd";
    static bool registered = false;
    if (!registered)
    {
        RegisterClassW(&wc);
        registered = true;
    }

    if (s_hTalkWnd)
    {
        KillTimer(s_hTalkWnd, kTalkAutoHideTimer);
        DestroyWindow(s_hTalkWnd);
        s_hTalkWnd = nullptr;
    }

    s_hTalkWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        nullptr,
        WS_POPUP,
        0, 0,
        10, 10,
        hwndParent,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr);

    if (!s_hTalkWnd)
        return;

    SetLayeredWindowAttributes(s_hTalkWnd, 0, 235, LWA_ALPHA);

    int w = 0, h = 0;
    MeasureTalkText(kTalkMaxWidth, w, h);
    PositionTalkWindow(s_hTalkWnd, w, h);
    SetTimer(s_hTalkWnd, kTalkAutoHideTimer, kTalkAutoHideMs, nullptr);
    ShowWindow(s_hTalkWnd, SW_SHOW);
}

void BubbleChatPanel::UpdatePosition()
{
    if (!s_hTalkWnd || !IsWindow(s_hTalkWnd))
        return;

    RECT rc;
    GetWindowRect(s_hTalkWnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int x = g_pet.x + (g_pet.w - w) / 2;
    int y = g_pet.y - h - 10;
    if (x < 0) x = 0;
    if (x + w > screenW) x = screenW - w;
    if (y < 0) y = g_pet.y + g_pet.h + 10;

    SetWindowPos(s_hTalkWnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE);
}
