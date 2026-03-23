#include "func_chat.h"
#include <windows.h>
#include <string>
#include <iostream>

// 病娇输入气泡内部状态
HWND g_hInputWnd = nullptr;
std::wstring g_inputText;
POINT g_dragOffset;
bool g_dragging = false;

// 窗口过程：气泡输入 + 闪烁 + 可拖动
LRESULT CALLBACK InputWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

        // 背景
        HBRUSH hBrush = CreateSolidBrush(flash ? RGB(255,200,220) : RGB(255,240,245));
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);
        flash = false;

        // 显示文字
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(50,50,50));
        DrawTextW(hdc, g_inputText.c_str(), -1, &rc, DT_LEFT|DT_VCENTER|DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_LBUTTONDOWN:
    {
        flash = true;
        InvalidateRect(hwnd,NULL,TRUE);
        g_dragging = true;
        g_dragOffset.x = LOWORD(lParam);
        g_dragOffset.y = HIWORD(lParam);
        SetCapture(hwnd);
        break;
    }
    case WM_LBUTTONUP:
        g_dragging = false;
        ReleaseCapture();
        break;
    case WM_MOUSEMOVE:
        if (g_dragging)
        {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            ClientToScreen(hwnd, &pt);
            int newX = pt.x - g_dragOffset.x;
            int newY = pt.y - g_dragOffset.y;
            SetWindowPos(hwnd, HWND_TOPMOST, newX, newY, 0,0, SWP_NOSIZE|SWP_NOACTIVATE);
        }
        break;
    case WM_CHAR:
        if (wParam==VK_RETURN) DestroyWindow(hwnd);
        else if (wParam==VK_BACK)
        {
            if(!g_inputText.empty()) g_inputText.pop_back();
            InvalidateRect(hwnd,NULL,TRUE);
        }
        else
        {
            g_inputText.push_back((wchar_t)wParam);
            InvalidateRect(hwnd,NULL,TRUE);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd,msg,wParam,lParam);
    }
    return 0;
}

// 弹出输入气泡
void ShowChatInput(HWND hwndParent)
{
    const int width = 300, height = 40;
    g_inputText.clear();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = InputWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ChatInputWnd";
    RegisterClassW(&wc);

    g_hInputWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName, nullptr,
        WS_POPUP,
        150,150,width,height,
        hwndParent, nullptr, GetModuleHandle(NULL), nullptr
    );
    if(!g_hInputWnd) return;

    SetLayeredWindowAttributes(g_hInputWnd,0,220,LWA_ALPHA);
    ShowWindow(g_hInputWnd,SW_SHOW);
    SetFocus(g_hInputWnd);

    MSG msg;
    while(IsWindow(g_hInputWnd))
    {
        if(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if(!g_inputText.empty())
        HandleInput(hwndParent,g_inputText);
}

// ------------------ 对话逻辑 ------------------
void Talk(HWND hwnd,const wchar_t* text)
{
    MessageBoxW(hwnd,text,L"晴小姐",MB_OK);
}

void Dialog_Love(HWND hwnd)
{
    Talk(hwnd,L"……真的？");
    int res = MessageBoxW(hwnd,L"你要让我留下，还是去休息？",L"晴小姐",MB_YESNO);
    if(res==IDYES) Talk(hwnd,L"那我就继续陪着你……");
    else
    {
        Talk(hwnd,L"……那我先睡一会儿，好吗？");
        PostQuitMessage(0);
    }
}

void Dialog_Unknown(HWND hwnd)
{
    Talk(hwnd,L"……我不太明白你在说什么。");
}

void HandleInput(HWND hwnd,const std::wstring& input)
{
    if(input==L"我爱你") Dialog_Love(hwnd);
    else Dialog_Unknown(hwnd);
}