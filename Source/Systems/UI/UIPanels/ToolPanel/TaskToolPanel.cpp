#include "TaskToolPanel.h"
#include "../../../Pet/PetActor.h"
#include <tlhelp32.h>
#include <windowsx.h>
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")
#include <string>
#include <vector>

namespace
{
    static HWND s_hTaskWnd = nullptr;
    static HWND s_hParentWnd = nullptr;
    static HFONT s_font = nullptr;
    static bool s_dragging = false;
    static POINT s_dragOffset = {};

    constexpr int kTaskWidth = 220;
    constexpr int kTaskHeight = 300;
    constexpr int kItemHeight = 22;
    constexpr int kTitleBarHeight = 24;

    struct TaskItem
    {
        std::wstring title;
        std::wstring processName;
        HICON icon;
        DWORD pid;
    };
    static std::vector<TaskItem> s_tasks;

    void EnsureFont()
    {
        if (!s_font)
        {
            s_font = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                L"Microsoft YaHei UI");
        }
    }

    bool IsAppWindow(HWND hwnd)
    {
        if (!IsWindowVisible(hwnd))
            return false;
        LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        if (!(style & WS_VISIBLE))
            return false;
        if (style & WS_CHILD)
            return false;
        if (GetWindow(hwnd, GW_OWNER) != nullptr)
            return false;
        if ((style & WS_EX_TOOLWINDOW) != 0)
            return false;
        wchar_t title[256] = {};
        GetWindowTextW(hwnd, title, 256);
        if (title[0] == L'\0')
            return false;
        return true;
    }

    HICON GetWindowIcon(HWND hwnd)
    {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == 0)
            return nullptr;

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProcess)
            return nullptr;

        wchar_t path[MAX_PATH] = {};
        DWORD size = MAX_PATH;
        BOOL ok = QueryFullProcessImageNameW(hProcess, 0, path, &size);
        CloseHandle(hProcess);
        if (!ok)
            return nullptr;

        SHFILEINFOW sfi = {};
        if (SHGetFileInfoW(path, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON))
            return sfi.hIcon;

        return nullptr;
    }

    void BuildTaskList()
    {
        for (auto& t : s_tasks)
        {
            if (t.icon)
                DestroyIcon(t.icon);
        }
        s_tasks.clear();

        DWORD selfPid = GetCurrentProcessId();

        struct Ctx { std::vector<TaskItem>* list; DWORD selfPid; };
        Ctx ctx = { &s_tasks, GetCurrentProcessId() };
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            auto* ctx = reinterpret_cast<Ctx*>(lParam);
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid == ctx->selfPid) return TRUE;
            if (IsAppWindow(hwnd))
            {
                TaskItem item;
                wchar_t title[256] = {};
                GetWindowTextW(hwnd, title, 256);
                item.title = title;
                item.pid = pid;
                item.icon = GetWindowIcon(hwnd);

                HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                if (hSnapshot != INVALID_HANDLE_VALUE)
                {
                    PROCESSENTRY32W pe = { sizeof(pe) };
                    if (Process32FirstW(hSnapshot, &pe))
                    {
                        do
                        {
                            if (pe.th32ProcessID == pid)
                            {
                                item.processName = pe.szExeFile;
                                break;
                            }
                        } while (Process32NextW(hSnapshot, &pe));
                    }
                    CloseHandle(hSnapshot);
                }
                ctx->list->push_back(item);
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&ctx));
    }

    void PositionWindow(HWND hwnd)
    {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int x = g_pet.x - kTaskWidth - 12;
        if (x < 0) x = g_pet.x + g_pet.w + 12;
        int y = g_pet.y;
        if (y + kTaskHeight > screenH)
            y = screenH - kTaskHeight;
        if (y < 0) y = 0;
        SetWindowPos(hwnd, HWND_TOPMOST, x, y, kTaskWidth, kTaskHeight, SWP_NOACTIVATE);
    }

    LRESULT CALLBACK TaskWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CREATE:
            BuildTaskList();
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            HBRUSH bg = CreateSolidBrush(RGB(255, 245, 250));
            FillRect(hdc, &rc, bg);
            DeleteObject(bg);

            RECT titleRc = { 0, 0, rc.right, kTitleBarHeight };
            HBRUSH titleBg = CreateSolidBrush(RGB(255, 235, 242));
            FillRect(hdc, &titleRc, titleBg);
            DeleteObject(titleBg);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(180, 120, 140));
            if (s_font)
                SelectObject(hdc, s_font);

            std::wstring title = L"Tasks (" + std::to_wstring(s_tasks.size()) + L")";
            TextOutW(hdc, 8, 4, title.c_str(), static_cast<int>(title.size()));

            int y = kTitleBarHeight + 4;
            for (const auto& task : s_tasks)
            {
                if (y + kItemHeight > rc.bottom)
                    break;

                if (task.icon)
                    DrawIconEx(hdc, 6, y, task.icon, 16, 16, 0, nullptr, DI_NORMAL);

                std::wstring text = task.title;
                if (text.empty())
                    text = task.processName;
                SetTextColor(hdc, RGB(140, 100, 120));
                TextOutW(hdc, 28, y + 2, text.c_str(), static_cast<int>(text.size()));
                y += kItemHeight;
            }

            if (s_tasks.empty())
            {
                SetTextColor(hdc, RGB(180, 150, 160));
                TextOutW(hdc, 8, y + 2, L"No visible windows", 18);
            }

            EndPaint(hwnd, &ps);
            break;
        }

        case WM_LBUTTONDOWN:
            s_dragging = true;
            s_dragOffset.x = GET_X_LPARAM(lParam);
            s_dragOffset.y = GET_Y_LPARAM(lParam);
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
                SetWindowPos(hwnd, HWND_TOPMOST,
                    pt.x - s_dragOffset.x,
                    pt.y - s_dragOffset.y,
                    0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            }
            break;

        case WM_DESTROY:
            s_hTaskWnd = nullptr;
            for (auto& t : s_tasks)
            {
                if (t.icon)
                    DestroyIcon(t.icon);
            }
            s_tasks.clear();
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }
}

void TaskToolPanel::Setup(UIActor& actor)
{
    (void)actor;
}

void TaskToolPanel::Toggle(HWND hwndParent)
{
    if (s_hTaskWnd && IsWindow(s_hTaskWnd))
    {
        Hide();
        return;
    }

    EnsureFont();
    s_hParentWnd = hwndParent;

    WNDCLASSW wc = {};
    wc.lpfnWndProc = TaskWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"PetTaskWnd";
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    static bool registered = false;
    if (!registered)
    {
        RegisterClassW(&wc);
        registered = true;
    }

    s_hTaskWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"Task Manager",
        WS_POPUP | WS_CAPTION | WS_THICKFRAME,
        100, 100,
        kTaskWidth, kTaskHeight,
        hwndParent,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr);

    if (!s_hTaskWnd)
        return;

    if (s_font)
        SendMessageW(s_hTaskWnd, WM_SETFONT, reinterpret_cast<WPARAM>(s_font), TRUE);

    PositionWindow(s_hTaskWnd);
    ShowWindow(s_hTaskWnd, SW_SHOW);
    UpdateWindow(s_hTaskWnd);
}

bool TaskToolPanel::IsVisible()
{
    return s_hTaskWnd != nullptr && IsWindow(s_hTaskWnd);
}

void TaskToolPanel::Hide()
{
    if (s_hTaskWnd)
    {
        DestroyWindow(s_hTaskWnd);
        s_hTaskWnd = nullptr;
    }
    for (auto& t : s_tasks)
    {
        if (t.icon)
            DestroyIcon(t.icon);
    }
    s_tasks.clear();
}

int TaskToolPanel::GetTaskCount()
{
    if (s_tasks.empty())
        BuildTaskList();
    return static_cast<int>(s_tasks.size());
}

std::wstring TaskToolPanel::GetTaskTitle(int index)
{
    if (s_tasks.empty())
        BuildTaskList();
    if (index < 0 || index >= static_cast<int>(s_tasks.size()))
        return L"";
    const auto& t = s_tasks[index];
    return t.title.empty() ? t.processName : t.title;
}

HICON TaskToolPanel::GetTaskIcon(int index)
{
    if (s_tasks.empty())
        BuildTaskList();
    if (index < 0 || index >= static_cast<int>(s_tasks.size()))
        return nullptr;
    return s_tasks[index].icon;
}

DWORD TaskToolPanel::GetTaskPID(int index)
{
    if (s_tasks.empty())
        BuildTaskList();
    if (index < 0 || index >= static_cast<int>(s_tasks.size()))
        return 0;
    return s_tasks[index].pid;
}

void TaskToolPanel::KillTask(int index)
{
    if (s_tasks.empty())
        BuildTaskList();
    if (index < 0 || index >= static_cast<int>(s_tasks.size()))
        return;
    DWORD pid = s_tasks[index].pid;
    if (pid == 0)
        return;
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess)
    {
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
    }
}

void TaskToolPanel::RefreshTaskList()
{
    for (auto& t : s_tasks)
    {
        if (t.icon)
            DestroyIcon(t.icon);
    }
    s_tasks.clear();
    BuildTaskList();
}
