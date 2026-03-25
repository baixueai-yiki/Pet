#include <windows.h>
#include "Chat.h"
#include "Core/Path.h"
#include <string>
#include <fstream>
#include <map>

// 全局静态状态（仅当前 cpp 使用）
static HWND s_hInputWnd = nullptr;// 当前输入窗口句柄
static HWND s_hButtonWnd = nullptr;// 当前按钮窗口句柄
static std::wstring s_inputText;// 当前输入框中的文本内容
static POINT s_dragOffset;// 鼠标拖拽偏移
static bool s_dragging = false;// 拖拽状态
static std::map<std::wstring, std::wstring> s_dialogMap;// 聊天配置表
static std::map<std::wstring, std::wstring> s_buttonMap;// 按钮选项配置


// 配置文件读取
static void LoadDialogConfig()
{
    s_dialogMap.clear();
    s_buttonMap.clear();

    std::wifstream file(GetConfigPath(L"chat_safeword.txt"));
    if (!file.is_open()) return;

    std::wstring line;
    while (std::getline(file, line))
    {
        size_t pos = line.find(L'=');
        if (pos == std::wstring::npos) continue;
        std::wstring key = line.substr(0, pos);
        std::wstring value = line.substr(pos + 1);

        // 判断是否为按钮选项
        if (!key.empty() && key[0] == L'#')
            s_buttonMap[key.substr(1)] = value;
        else
            s_dialogMap[key] = value;
    }
}


// 对话输出
void ChatTalk(HWND hwnd, const wchar_t* text)
{
    MessageBoxW(hwnd, text, L"晴小姐", MB_OK);
}


// 文字输入处理逻辑
void ChatHandleInput(HWND hwnd, const std::wstring& input)
{
    if (s_dialogMap.empty())
        LoadDialogConfig();

    auto it = s_dialogMap.find(input);

    // 特殊关键词：我爱你
    if (input == L"我爱你")
    {
        if (it != s_dialogMap.end())
            ChatTalk(hwnd, it->second.c_str());
        else
            ChatTalk(hwnd, L"……真的？");

        // 弹出按钮输入
        ChatShowButtonInput(hwnd, L"我爱你_1", L"我爱你_2");
    }
    else
    {
        // 普通输入
        if (it != s_dialogMap.end())
            ChatTalk(hwnd, it->second.c_str());
        else
        {
            auto def = s_dialogMap.find(L"默认");
            if (def != s_dialogMap.end())
                ChatTalk(hwnd, def->second.c_str());
            else
                ChatTalk(hwnd, L"……我不太明白你在说什么。");
        }
    }
}


// 按钮输入处理逻辑
static void ChatHandleButtonInput(HWND hwnd, const std::wstring& key)
{
    if (s_buttonMap.empty())
        LoadDialogConfig();

    auto it = s_buttonMap.find(key);
    if (it != s_buttonMap.end())
        ChatTalk(hwnd, it->second.c_str());

    DestroyWindow(hwnd);
    s_hButtonWnd = nullptr;
}


// 文字输入窗口过程
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

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(50, 50, 50));
        DrawTextW(hdc, s_inputText.c_str(), -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        EndPaint(hwnd, &ps);

        flash = false;
        break;
    }
    case WM_LBUTTONDOWN:
        flash = true;
        InvalidateRect(hwnd, nullptr, TRUE);
        s_dragOffset.x = LOWORD(lParam);
        s_dragOffset.y = HIWORD(lParam);
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
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ClientToScreen(hwnd, &pt);
            SetWindowPos(hwnd, HWND_TOPMOST, pt.x - s_dragOffset.x, pt.y - s_dragOffset.y,
                0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        }
        break;
    case WM_CHAR:
        if (wParam == VK_RETURN)
        {
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
            s_inputText.push_back((wchar_t)wParam);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        break;
    case WM_DESTROY:
        if (!s_inputText.empty())
        {
            ChatHandleInput(GetParent(hwnd), s_inputText);
        }
        s_hInputWnd = nullptr;
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


// 按钮输入窗口过程
static LRESULT CALLBACK ButtonInputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            int id = LOWORD(wParam);
            std::wstring key = L"按钮" + std::to_wstring(id);
            ChatHandleButtonInput(GetParent(hwnd), key);
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


// 显示文字输入窗口
void ChatShowInput(HWND hwndParent)
{
    const int width = 300;
    const int height = 40;
    s_inputText.clear();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = TextInputProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ChatInputWnd";
    static bool registered = false;
    if (!registered)
    {
        RegisterClassW(&wc);
        registered = true;
    }

    s_hInputWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        nullptr,
        WS_POPUP,
        150, 150,
        width, height,
        hwndParent,
        nullptr,
        GetModuleHandle(NULL),
        nullptr
    );
    if (!s_hInputWnd) return;
    SetLayeredWindowAttributes(s_hInputWnd, 0, 220, LWA_ALPHA);
    ShowWindow(s_hInputWnd, SW_SHOW);
    SetFocus(s_hInputWnd);
}

// 显示按钮输入窗口
void ChatShowButtonInput(HWND hwndParent, const std::wstring& key1, const std::wstring& key2)
{
    const int width = 200;
    const int height = 100;

    WNDCLASSW wc = {};
    wc.lpfnWndProc = ButtonInputProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ChatButtonWnd";
    static bool registered = false;
    if (!registered)
    {
        RegisterClassW(&wc);
        RegisterClassW(&wc);
        registered = true;
    }

    s_hButtonWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        nullptr,
        WS_POPUP,
        200, 200,
        width, height,
        hwndParent,
        nullptr,
        GetModuleHandle(NULL),
        nullptr
    );

    if (!s_hButtonWnd) return;
    SetLayeredWindowAttributes(s_hButtonWnd, 0, 230, LWA_ALPHA);

    HWND btn1 = CreateWindowW(L"BUTTON", key1.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 20, 150, 30, s_hButtonWnd, (HMENU)1, GetModuleHandle(NULL), nullptr);
    HWND btn2 = CreateWindowW(L"BUTTON", key2.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 60, 150, 30, s_hButtonWnd, (HMENU)2, GetModuleHandle(NULL), nullptr);

    ShowWindow(s_hButtonWnd, SW_SHOW);
}