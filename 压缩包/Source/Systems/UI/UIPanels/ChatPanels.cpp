#include "ChatPanels.h"

#include "../../Chat/Chat.h"

#include <imm.h>
#include <windows.h>
#include <windowsx.h>

namespace
{
    static HWND s_hInputWnd = nullptr;
    static HWND s_hButtonWnd = nullptr;
    static HWND s_hTalkWnd = nullptr;
    static HFONT s_inputFont = nullptr;
    static HFONT s_talkFont = nullptr;
    static std::wstring s_inputText;
    static std::wstring s_imeText;
    static POINT s_dragOffset = {};
    static bool s_dragging = false;
    static std::wstring s_buttonKey1;
    static std::wstring s_buttonKey2;
    static std::wstring s_talkText;

    const int kInputWidth = 300;
    const int kInputHeight = 40;
    const int kButtonHeight = 80;
    const int kTalkPadding = 8;
    const int kTalkMaxWidth = 240;
    const UINT_PTR kTalkAutoHideTimer = 1;
    const UINT kTalkAutoHideMs = 3000;

    static void EnsureFonts()
    {
        if (!s_inputFont)
        {
            s_inputFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                L"Microsoft YaHei UI");
        }
        if (!s_talkFont)
        {
            s_talkFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                L"Microsoft YaHei UI");
        }
    }

    static void MeasureTalkText(int maxWidth, int& outW, int& outH)
    {
        HDC hdc = GetDC(nullptr);
        RECT rc = { 0, 0, maxWidth, 0 };
        if (s_talkFont)
            SelectObject(hdc, s_talkFont);
        DrawTextW(hdc, s_talkText.c_str(), -1, &rc, DT_CALCRECT | DT_WORDBREAK);
        ReleaseDC(nullptr, hdc);
        outW = (rc.right - rc.left) + kTalkPadding * 2;
        outH = (rc.bottom - rc.top) + kTalkPadding * 2;
        if (outW < 80)
            outW = 80;
        if (outH < 32)
            outH = 32;
    }

    static void PositionTalkWindow(HWND hwnd, int w, int h)
    {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int x = (screenW - w) / 2;
        int y = 50;
        SetWindowPos(hwnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE);
    }

    static void ShowWindowEx(HWND& wnd, const wchar_t* cls, WNDPROC proc, HWND parent)
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

    static LRESULT CALLBACK TextInputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        static bool flash = false;
        switch (msg)
        {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH hBrush = CreateSolidBrush(flash ? RGB(255, 200, 220) : RGB(255, 240, 245));
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
            if (s_inputFont)
                SelectObject(hdc, s_inputFont);
            SetBkMode(hdc, TRANSPARENT);
            std::wstring display = s_inputText + s_imeText;
            DrawTextW(hdc, display.c_str(), -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            EndPaint(hwnd, &ps);
            flash = false;
            break;
        }
        case WM_LBUTTONDOWN:
            flash = true;
            InvalidateRect(hwnd, nullptr, TRUE);
            s_dragOffset.x = GET_X_LPARAM(lParam);
            s_dragOffset.y = GET_Y_LPARAM(lParam);
            s_dragging = true;
            SetCapture(hwnd);
            break;
        case WM_LBUTTONUP:
            s_dragging = false;
            ReleaseCapture();
            break;
        case WM_MOUSEMOVE:
            if (s_dragging)
            {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ClientToScreen(hwnd, &pt);
                SetWindowPos(hwnd, HWND_TOPMOST, pt.x - s_dragOffset.x, pt.y - s_dragOffset.y,
                    0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            }
            break;
        case WM_CHAR:
            if (wParam == VK_RETURN)
            {
                if (!s_imeText.empty())
                {
                    s_inputText += s_imeText;
                    s_imeText.clear();
                }
                DestroyWindow(hwnd);
            }
            else if (wParam == VK_BACK)
            {
                if (!s_inputText.empty())
                    s_inputText.pop_back();
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            else
            {
                s_inputText.push_back(static_cast<wchar_t>(wParam));
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            break;
        case WM_IME_COMPOSITION:
        {
            HIMC hImc = ImmGetContext(hwnd);
            if (hImc)
            {
                if (lParam & GCS_COMPSTR)
                {
                    LONG len = ImmGetCompositionStringW(hImc, GCS_COMPSTR, nullptr, 0);
                    if (len > 0)
                    {
                        std::wstring buf(len / sizeof(wchar_t), L'\0');
                        ImmGetCompositionStringW(hImc, GCS_COMPSTR, &buf[0], len);
                        s_imeText = buf;
                    }
                }
                if (lParam & GCS_RESULTSTR)
                {
                    LONG len = ImmGetCompositionStringW(hImc, GCS_RESULTSTR, nullptr, 0);
                    if (len > 0)
                    {
                        std::wstring buf(len / sizeof(wchar_t), L'\0');
                        ImmGetCompositionStringW(hImc, GCS_RESULTSTR, &buf[0], len);
                        s_inputText += buf;
                    }
                    s_imeText.clear();
                }
                ImmReleaseContext(hwnd, hImc);
            }
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        }
        case WM_DESTROY:
            if (!s_inputText.empty())
            {
                ChatHandleInput(GetParent(hwnd), s_inputText);
            }
            s_inputText.clear();
            s_imeText.clear();
            s_hInputWnd = nullptr;
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    static LRESULT CALLBACK ButtonInputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH hBrush = CreateSolidBrush(RGB(255, 240, 245));
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                const int id = LOWORD(wParam);
                const std::wstring key = (id == 1) ? s_buttonKey1 : s_buttonKey2;
                ChatHandleButtonInput(hwnd, key);
            }
            break;
        case WM_DESTROY:
            s_hButtonWnd = nullptr;
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    static LRESULT CALLBACK TalkProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
            if (s_talkFont)
                SelectObject(hdc, s_talkFont);
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

void ChatPanels::Setup(UIActor& actor)
{
    (void)actor;
    EnsureFonts();
}

void ChatPanels::ShowInput(HWND hwndParent)
{
    EnsureFonts();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = TextInputProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"ChatInputWnd";
    static bool registered = false;
    if (!registered)
    {
        RegisterClassW(&wc);
        registered = true;
    }

    if (s_hInputWnd)
    {
        DestroyWindow(s_hInputWnd);
        s_hInputWnd = nullptr;
    }

    s_inputText.clear();
    s_imeText.clear();

    s_hInputWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        nullptr,
        WS_POPUP,
        150, 150,
        kInputWidth,
        kInputHeight,
        hwndParent,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr);

    if (!s_hInputWnd)
        return;

    SetLayeredWindowAttributes(s_hInputWnd, 0, 220, LWA_ALPHA);
    if (s_inputFont)
        SendMessageW(s_hInputWnd, WM_SETFONT, (WPARAM)s_inputFont, TRUE);
    ShowWindow(s_hInputWnd, SW_SHOW);
    SetFocus(s_hInputWnd);
}

void ChatPanels::ShowButtonInput(HWND hwndParent, const std::wstring& key1, const std::wstring& key2)
{
    s_buttonKey1 = key1;
    s_buttonKey2 = key2;

    EnsureFonts();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = ButtonInputProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"ChatButtonWnd";
    static bool registered = false;
    if (!registered)
    {
        RegisterClassW(&wc);
        registered = true;
    }

    if (s_hButtonWnd)
    {
        DestroyWindow(s_hButtonWnd);
        s_hButtonWnd = nullptr;
    }

    s_hButtonWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        nullptr,
        WS_POPUP,
        0, 0,
        kInputWidth,
        kButtonHeight,
        hwndParent,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr);

    if (!s_hButtonWnd)
        return;

    SetLayeredWindowAttributes(s_hButtonWnd, 0, 230, LWA_ALPHA);

    const int btnW = kInputWidth - 40;
    const int btnH = 28;
    const std::wstring label1 = ChatGetButtonLabel(key1, key1);
    const std::wstring label2 = ChatGetButtonLabel(key2, key2);
    HWND btn1 = CreateWindowW(L"BUTTON", label1.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 10, btnW, btnH, s_hButtonWnd, reinterpret_cast<HMENU>(1), GetModuleHandle(nullptr), nullptr);
    HWND btn2 = CreateWindowW(L"BUTTON", label2.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 10 + btnH + 12, btnW, btnH, s_hButtonWnd, reinterpret_cast<HMENU>(2), GetModuleHandle(nullptr), nullptr);

    if (s_inputFont)
    {
        SendMessageW(btn1, WM_SETFONT, (WPARAM)s_inputFont, TRUE);
        SendMessageW(btn2, WM_SETFONT, (WPARAM)s_inputFont, TRUE);
    }

    ShowWindow(s_hButtonWnd, SW_SHOW);
}

void ChatPanels::Talk(HWND hwndParent, const wchar_t* text)
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

void ChatPanels::ShowTaskList(HWND hwndParent)
{
    (void)hwndParent;
}
