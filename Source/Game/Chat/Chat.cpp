#include <windows.h>
#include <imm.h>
#include "Chat.h"
#include "Core/Path.h"
#include "../../Game/Pet/Pet.h"
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <sstream>
#include <cstring>

// 全局静态状态（仅当前 cpp 使用）
static HWND s_hInputWnd = nullptr;// 当前输入窗口句柄
static HWND s_hButtonWnd = nullptr;// 当前按钮窗口句柄
static std::wstring s_inputText;// 当前输入框中的文本内容
static POINT s_dragOffset;// 鼠标拖拽偏移
static bool s_dragging = false;// 拖拽状态
static std::map<std::wstring, std::wstring> s_dialogMap;// 聊天配置表
static std::map<std::wstring, std::wstring> s_buttonMap;// 按钮选项配置
static std::map<std::wstring, std::wstring> s_buttonLabelMap;// 按钮显示文本配置
static HFONT s_inputFont = nullptr; // 统一输入窗口字体，避免中文显示乱码
static std::wstring s_imeText; // 输入法组合字符串
static std::wstring s_buttonKey1; // 当前按钮1逻辑键
static std::wstring s_buttonKey2; // 当前按钮2逻辑键
static const int kInputWidth = 300;
static const int kInputHeight = 40;
static const int kButtonHeight = 80;

static int GetInputWidth()
{
    return (g_pet.w > 0) ? g_pet.w : kInputWidth;
}

static void GetInputRect(int& x, int& y, int& w, int& h)
{
    w = GetInputWidth();
    h = kInputHeight;
    x = g_pet.x;
    y = g_pet.y - h - 10;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
}

static void GetButtonRect(int& x, int& y, int& w, int& h)
{
    w = GetInputWidth();
    h = kButtonHeight;
    x = g_pet.x;
    y = g_pet.y - h - 10;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
}
static std::wstring s_dialogPath;
static FILETIME s_dialogWriteTime = {};
static bool s_hasDialogTime = false;
static HWND s_hTalkWnd = nullptr;
static std::wstring s_talkText;
static HFONT s_talkFont = nullptr;
static const int kTalkMaxWidth = 240;
static const int kTalkPadding = 8;
static const int kTalkCorner = 10;
static const int kTalkTail = 10;
static bool s_talkOnRight = true;
static const UINT_PTR kTalkAutoHideTimer = 1;
static const UINT kTalkAutoHideMs = 3000;

static std::wstring Trim(const std::wstring& text)
{
    const wchar_t* ws = L" \t\r\n";
    size_t start = text.find_first_not_of(ws);
    size_t end = text.find_last_not_of(ws);
    if (start == std::wstring::npos)
        return L"";
    return text.substr(start, end - start + 1);
}

#pragma comment(lib, "imm32.lib")


static bool ReadFileLines(const std::wstring& path, std::vector<std::wstring>& lines)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        return false;

    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (data.empty())
        return true;

    bool utf8 = (data.size() >= 3 &&
                 static_cast<unsigned char>(data[0]) == 0xEF &&
                 static_cast<unsigned char>(data[1]) == 0xBB &&
                 static_cast<unsigned char>(data[2]) == 0xBF);
    if (utf8)
        data.erase(0, 3);

    UINT codePage = utf8 ? CP_UTF8 : CP_ACP;
    int wlen = MultiByteToWideChar(codePage, 0, data.data(), static_cast<int>(data.size()), nullptr, 0);
    if (wlen <= 0)
        return false;

    std::wstring wdata(static_cast<size_t>(wlen), L'\0');
    MultiByteToWideChar(codePage, 0, data.data(), static_cast<int>(data.size()), &wdata[0], wlen);

    std::wistringstream iss(wdata);
    std::wstring line;
    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == L'\r')
            line.pop_back();
        lines.push_back(line);
    }
    return true;
}

static bool UpdateDialogMetadata(const std::wstring& path)
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data))
        return false;

    s_dialogWriteTime = data.ftLastWriteTime;
    s_dialogPath = path;
    s_hasDialogTime = true;
    return true;
}

static bool DialogFileChanged()
{
    if (!s_hasDialogTime || s_dialogPath.empty())
        return false;

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExW(s_dialogPath.c_str(), GetFileExInfoStandard, &data))
        return false;

    return memcmp(&data.ftLastWriteTime, &s_dialogWriteTime, sizeof(FILETIME)) != 0;
}

// 配置文件读取
static void LoadDialogConfig()
{
    s_dialogMap.clear();
    s_buttonMap.clear();
    s_buttonLabelMap.clear();

    std::vector<std::wstring> lines;
    const std::wstring path = GetAssetPath(L"chat\\chat_safeword.txt");
    if (!ReadFileLines(path, lines))
        return;

    for (const auto& lineRaw : lines)
    {
        std::wstring line = Trim(lineRaw);
        if (line.empty())
            continue;

        // 三段式：key=label=reply
        size_t firstEq = line.find(L'=');
        if (firstEq == std::wstring::npos)
            continue;
        size_t secondEq = line.find(L'=', firstEq + 1);

        std::wstring key = Trim(line.substr(0, firstEq));
        // 移除 BOM
        if (!key.empty() && key.front() == L'\ufeff')
            key.erase(0, 1);

        std::wstring value;
        std::wstring label;

        if (secondEq != std::wstring::npos)
        {
            label = Trim(line.substr(firstEq + 1, secondEq - firstEq - 1));
            value = Trim(line.substr(secondEq + 1));
        }
        else
        {
            value = Trim(line.substr(firstEq + 1));
        }

        // 判断是否为按钮选项 / 按钮显示文本
        if (!key.empty() && key[0] == L'#')
        {
            // 约定：#按钮1=对应点击后的回复
            s_buttonMap[key.substr(1)] = value;
            continue;
        }

        // 若是三段式，认为这是按钮配置：key=按钮显示文本=点击回复
        if (!label.empty())
        {
            s_buttonLabelMap[key] = label;
            s_buttonMap[key] = value;
            continue;
        }

        const std::wstring labelPrefixEn = L"button_text:";
        const std::wstring labelPrefixCn = L"按钮文本:";
        if (key.rfind(labelPrefixEn, 0) == 0)
        {
            s_buttonLabelMap[key.substr(labelPrefixEn.size())] = value;
            continue;
        }
        if (key.rfind(labelPrefixCn, 0) == 0)
        {
            s_buttonLabelMap[key.substr(labelPrefixCn.size())] = value;
            continue;
        }

        s_dialogMap[key] = value;
    }

    UpdateDialogMetadata(path);
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
    if (outW < 80) outW = 80;
    if (outH < 32) outH = 32;
}

static void PositionTalkWindow(int w, int h)
{
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    int x = g_pet.x + g_pet.w + 10;
    int y = g_pet.y + (g_pet.h - h) / 2;
    if (x + w > screenW)
    {
        x = g_pet.x - w - 10;
        s_talkOnRight = false;
    }
    else
    {
        s_talkOnRight = true;
    }
    if (x < 0) x = 0;
    if (y + h > screenH) y = screenH - h;
    if (y < 0) y = 0;

    SetWindowPos(s_hTalkWnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

static LRESULT CALLBACK TalkProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TIMER:
        if (wParam == kTalkAutoHideTimer)
        {
            KillTimer(hwnd, kTalkAutoHideTimer);
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
        DestroyWindow(hwnd);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH brush = CreateSolidBrush(RGB(255, 250, 230));
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(220, 200, 170));
        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        HGDIOBJ oldPen = SelectObject(hdc, pen);

        // 圆角气泡
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, kTalkCorner, kTalkCorner);

        // 尖角（朝向桌宠）
        POINT tail[3];
        if (s_talkOnRight)
        {
            tail[0] = { rc.left, rc.top + (rc.bottom - rc.top) / 2 - kTalkTail };
            tail[1] = { rc.left - kTalkTail, rc.top + (rc.bottom - rc.top) / 2 };
            tail[2] = { rc.left, rc.top + (rc.bottom - rc.top) / 2 + kTalkTail };
        }
        else
        {
            tail[0] = { rc.right, rc.top + (rc.bottom - rc.top) / 2 - kTalkTail };
            tail[1] = { rc.right + kTalkTail, rc.top + (rc.bottom - rc.top) / 2 };
            tail[2] = { rc.right, rc.top + (rc.bottom - rc.top) / 2 + kTalkTail };
        }
        Polygon(hdc, tail, 3);

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
    case WM_DESTROY:
        KillTimer(hwnd, kTalkAutoHideTimer);
        s_hTalkWnd = nullptr;
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 对话输出
void ChatTalk(HWND hwnd, const wchar_t* text)
{
    s_talkText = text ? text : L"";

    if (!s_talkFont)
    {
        s_talkFont = CreateFontW(
            18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Microsoft YaHei UI");
    }

    WNDCLASSW wc = {};
    wc.lpfnWndProc = TalkProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ChatTalkWnd";
    static bool registered = false;
    if (!registered)
    {
        RegisterClassW(&wc);
        registered = true;
    }

    if (!s_hTalkWnd)
    {
        s_hTalkWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            wc.lpszClassName,
            nullptr,
            WS_POPUP,
            0, 0, 10, 10,
            hwnd,
            nullptr,
            GetModuleHandle(NULL),
            nullptr
        );
        SetLayeredWindowAttributes(s_hTalkWnd, 0, 235, LWA_ALPHA);
    }

    int w = 0, h = 0;
    MeasureTalkText(kTalkMaxWidth, w, h);
    PositionTalkWindow(w, h);
    ShowWindow(s_hTalkWnd, SW_SHOW);
    InvalidateRect(s_hTalkWnd, nullptr, TRUE);
    SetTimer(s_hTalkWnd, kTalkAutoHideTimer, kTalkAutoHideMs, nullptr);
}


// 文字输入处理逻辑
void ChatHandleInput(HWND hwnd, const std::wstring& input)
{
    if (s_dialogMap.empty() || DialogFileChanged())
        LoadDialogConfig();

    const std::wstring normalized = Trim(input);
    auto it = s_dialogMap.find(normalized);

    // 特殊关键词：我爱你
    if (normalized == L"我爱你")
    {
        if (it != s_dialogMap.end())
            ChatTalk(hwnd, it->second.c_str());
        else
            ChatTalk(hwnd, L"要结束病娇游戏吗？"); //未配置输出语句时的默认输出
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
static void ChatHandleButtonInput(HWND buttonWnd, const std::wstring& key)
{
    if (s_buttonMap.empty())
        LoadDialogConfig();

    auto it = s_buttonMap.find(key);
    if (it != s_buttonMap.end())
        ChatTalk(GetParent(buttonWnd), it->second.c_str());

    DestroyWindow(buttonWnd);
    s_hButtonWnd = nullptr;

    // 点击按钮后：先显示文本，确认后再退出进程
    if (key == L"我爱你_2")
        PostQuitMessage(0);
}

static std::wstring GetButtonLabel(const std::wstring& key, const std::wstring& fallback)
{
    auto it = s_buttonLabelMap.find(key);
    return (it != s_buttonLabelMap.end()) ? it->second : fallback;
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
            // 回车提交前，把输入法组合串并入正式文本
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
            s_inputText.push_back((wchar_t)wParam);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        break;
    case WM_IME_COMPOSITION:
    {
        HIMC hImc = ImmGetContext(hwnd);
        if (hImc)
        {
            // 处理输入法的临时组合字符串
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

            // 处理已提交的结果字符串
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
            int id = LOWORD(wParam);
            std::wstring key = (id == 1) ? s_buttonKey1 : s_buttonKey2;
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


// 显示文字输入窗口
void ChatShowInput(HWND hwndParent)
{
    // 已展开则收起（右键切换展开/收起）
    if (s_hInputWnd)
    {
        DestroyWindow(s_hInputWnd);
        s_hInputWnd = nullptr;
        return;
    }

    const int width = GetInputWidth();
    const int height = kInputHeight;
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
    if (!s_inputFont)
    {
        s_inputFont = CreateFontW(
            20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Microsoft YaHei UI");
    }
    if (s_inputFont)
        SendMessageW(s_hInputWnd, WM_SETFONT, (WPARAM)s_inputFont, TRUE);
    SetLayeredWindowAttributes(s_hInputWnd, 0, 220, LWA_ALPHA);
    ShowWindow(s_hInputWnd, SW_SHOW);
    SetFocus(s_hInputWnd);

    ChatUpdateInputPosition();
}

void ChatUpdateInputPosition()
{
    if (!s_hInputWnd)
        return;

    int x = 0, y = 0, w = 0, h = 0;
    GetInputRect(x, y, w, h);
    SetWindowPos(s_hInputWnd, HWND_TOPMOST, x, y, w, h,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void ChatUpdateButtonPosition()
{
    if (!s_hButtonWnd)
        return;

    int x = 0, y = 0, w = 0, h = 0;
    GetButtonRect(x, y, w, h);
    SetWindowPos(s_hButtonWnd, HWND_TOPMOST, x, y, w, h,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void ChatUpdateTalkPosition()
{
    if (!s_hTalkWnd)
        return;

    int w = 0, h = 0;
    MeasureTalkText(kTalkMaxWidth, w, h);
    PositionTalkWindow(w, h);
}

// 显示按钮输入窗口
void ChatShowButtonInput(HWND hwndParent, const std::wstring& key1, const std::wstring& key2)
{
    const int width = GetInputWidth();
    const int height = kButtonHeight;

    s_buttonKey1 = key1;
    s_buttonKey2 = key2;

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
        0, 0,
        width, height,
        hwndParent,
        nullptr,
        GetModuleHandle(NULL),
        nullptr
    );

    if (!s_hButtonWnd) return;
    SetLayeredWindowAttributes(s_hButtonWnd, 0, 230, LWA_ALPHA);

    const std::wstring label1 = GetButtonLabel(key1, key1);
    const std::wstring label2 = GetButtonLabel(key2, key2);

    const int btnW = width - 20 * 2;
    const int btnH = 28;
    HWND btn1 = CreateWindowW(L"BUTTON", label1.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 10, btnW, btnH, s_hButtonWnd, (HMENU)1, GetModuleHandle(NULL), nullptr);
    HWND btn2 = CreateWindowW(L"BUTTON", label2.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 10 + btnH + 12, btnW, btnH, s_hButtonWnd, (HMENU)2, GetModuleHandle(NULL), nullptr);

    if (s_inputFont)
    {
        SendMessageW(btn1, WM_SETFONT, (WPARAM)s_inputFont, TRUE);
        SendMessageW(btn2, WM_SETFONT, (WPARAM)s_inputFont, TRUE);
    }

    ShowWindow(s_hButtonWnd, SW_SHOW);
    ChatUpdateButtonPosition();
}
