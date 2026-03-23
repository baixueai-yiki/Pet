#include "Chat.h"
#include <string>
#include <windows.h>

static HWND s_hInputWnd = nullptr;
static std::wstring s_inputText;
static POINT s_dragOffset;
static bool s_dragging = false;

static LRESULT CALLBACK InputWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
        flash = false;

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(50, 50, 50));
        DrawTextW(hdc, s_inputText.c_str(), -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_LBUTTONDOWN:
    {
        flash = true;
        InvalidateRect(hwnd, nullptr, TRUE);
        s_dragging = true;
        s_dragOffset.x = LOWORD(lParam);
        s_dragOffset.y = HIWORD(lParam);
        SetCapture(hwnd);
        break;
    }
    case WM_LBUTTONUP:
        s_dragging = false;
        ReleaseCapture();
        break;
    case WM_MOUSEMOVE:
        if (s_dragging)
        {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ClientToScreen(hwnd, &pt);
            int newX = pt.x - s_dragOffset.x;
            int newY = pt.y - s_dragOffset.y;
            SetWindowPos(hwnd, HWND_TOPMOST, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        }
        break;
    case WM_CHAR:
        if (wParam == VK_RETURN)
            DestroyWindow(hwnd);
        else if (wParam == VK_BACK)
        {
            if (!s_inputText.empty())
                s_inputText.pop_back();
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        else
        {
            s_inputText.push_back((wchar_t)wParam);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

static void ChatTalk(HWND hwnd, const wchar_t* text)
{
    MessageBoxW(hwnd, text, L"晴小姐", MB_OK);
}

static void ChatDialogLove(HWND hwnd)
{
    ChatTalk(hwnd, L"……真的？");
    int res = MessageBoxW(hwnd, L"你要让我留下，还是去休息？", L"晴小姐", MB_YESNO);
    if (res == IDYES)
        ChatTalk(hwnd, L"那我就继续陪着你……");
    else
    {
        ChatTalk(hwnd, L"……那我先睡一会儿，好吗？");
        PostQuitMessage(0);
    }
}

static void ChatDialogUnknown(HWND hwnd)
{
    ChatTalk(hwnd, L"……我不太明白你在说什么。");
}

void ChatHandleInput(HWND hwnd, const std::wstring& input)
{
    if (input == L"我爱你")
        ChatDialogLove(hwnd);
    else
        ChatDialogUnknown(hwnd);
}

void ChatShowInput(HWND hwndParent)
{
    const int width = 300;
    const int height = 40;
    s_inputText.clear();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = InputWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ChatInputWnd";
    RegisterClassW(&wc);

    s_hInputWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName, nullptr,
        WS_POPUP,
        150, 150, width, height,
        hwndParent, nullptr, GetModuleHandle(NULL), nullptr
    );

    if (!s_hInputWnd)
        return;

    SetLayeredWindowAttributes(s_hInputWnd, 0, 220, LWA_ALPHA);
    ShowWindow(s_hInputWnd, SW_SHOW);
    SetFocus(s_hInputWnd);

    MSG msg;
    while (IsWindow(s_hInputWnd))
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (!s_inputText.empty())
        ChatHandleInput(hwndParent, s_inputText);
}
