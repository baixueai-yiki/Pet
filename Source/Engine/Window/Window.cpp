#include <windows.h>
#include <windowsx.h>
#include <fstream>
#include <sstream>
#include <string>
#include <cwctype>
#include <vector>
#include "../Input/InputDispatcher.h"
#include "../Render/Render.h"
#include "Window.h"
#include "../../Core/Path.h"
#include "../../Core/Diary.h"
#include "../../Game/Pet/Pet.h"
#include "../../Game/Chat/Chat.h"

static const UINT_PTR kIdleCheckTimer = 2;
static const UINT kIdleCheckMs = 60000;

static std::wstring Trim(const std::wstring& s)
{
    size_t b = 0;
    while (b < s.size() && iswspace(s[b]))
        ++b;
    size_t e = s.size();
    while (e > b && iswspace(s[e - 1]))
        --e;
    return s.substr(b, e - b);
}
static std::wstring ToLower(std::wstring s)
{
    for (auto& ch : s)
        ch = static_cast<wchar_t>(towlower(ch));
    return s;
}

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

static int ReadIntSetting(const std::wstring& key, int defaultValue)
{
    std::vector<std::wstring> lines;
    if (!ReadFileLines(GetConfigPath(L"settings.txt"), lines))
        return defaultValue;

    for (const auto& lineRaw : lines)
    {
        std::wstring line = lineRaw;
        if (line.empty() || line[0] == L'#')
            continue;
        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos)
            continue;
        std::wstring k = Trim(line.substr(0, eq));
        std::wstring kLower = ToLower(k);
        std::wstring keyLower = ToLower(key);
        std::wstring v = Trim(line.substr(eq + 1));
        if (kLower == keyLower ||
            (keyLower == L"fps" && (k == L"帧率" || k == L"刷新率")))
        {
            std::wistringstream iss(v);
            int out = defaultValue;
            iss >> out;
            return out;
        }
    }
    return defaultValue;
}
static UINT GetRefreshIntervalMs()
{
    int fps = ReadIntSetting(L"fps", 60);
    if (fps != 30 && fps != 60 && fps != 120)
        fps = 60;
    return static_cast<UINT>(1000 / fps);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool s_exitHandled = false;
    switch (msg)
    {
    case WM_CREATE:
        // Use a timer to keep drag redraw smooth
        SetTimer(hwnd, 1, GetRefreshIntervalMs(), nullptr);
        SetTimer(hwnd, kIdleCheckTimer, kIdleCheckMs, nullptr);
        OnProgramStart();
        break;


    case WM_NCHITTEST:
    {
        // 只有宠物区域可点击，其余区域点击穿透
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (x >= g_pet.x && x <= g_pet.x + g_pet.w &&
            y >= g_pet.y && y <= g_pet.y + g_pet.h)
        {
            return HTCLIENT;
        }
        return HTTRANSPARENT;
    }

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
        HandleInput(hwnd, msg, wParam, lParam);
        break;
    case WM_TIMER:
        // Redraw at a steady rate while dragging
        if (wParam == kIdleCheckTimer)
        {
            ChatTickIdleCheck(hwnd);
        }
        else
        {
            if (g_pet.isDragging)
                InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;


    case WM_PAINT:
    {
        // 进入/退出绘制流程，确保 WM_PAINT 被正确消费
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);

        // 使用双缓冲减少拖动时闪烁
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        HDC memDC = CreateCompatibleDC(ps.hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(ps.hdc, w, h);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        // 先清成黑色（与颜色键一致），再绘制宠物
        HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH);
        FillRect(memDC, &rc, black);
        RendererRender(memDC);

        BitBlt(ps.hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        if (!s_exitHandled)
        {
            s_exitHandled = true;
            OnProgramExit();
        }
        PostQuitMessage(0);
        return 0;

    case WM_QUERYENDSESSION:
        if (!s_exitHandled)
        {
            s_exitHandled = true;
            OnProgramExit();
        }
        return TRUE;

    case WM_ENDSESSION:
        if (wParam && !s_exitHandled)
        {
            s_exitHandled = true;
            OnProgramExit();
        }
        return 0;

    case WM_USER + 1:
        ChatShowInput(hwnd);
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

HWND CreateMainWindow(HINSTANCE hInstance)
{
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"PetWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH));

    ATOM atom = RegisterClassW(&wc);
    if (atom == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return nullptr;

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"Pet",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (hwnd)
        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    return hwnd;
}
