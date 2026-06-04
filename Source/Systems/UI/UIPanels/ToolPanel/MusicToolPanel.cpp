#include "MusicToolPanel.h"
#include "../../../Pet/PetActor.h"
#include "../../../../Core/Path.h"
#include <windowsx.h>
#include <commctrl.h>
#include <string>
#include <vector>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")

struct ComCtlInit { ComCtlInit() { InitCommonControls(); } } s_comCtl;

namespace
{
    struct MusicFile { std::wstring name; std::wstring path; };
    static std::vector<MusicFile> s_musicFiles;

    static HWND s_playerWnd   = nullptr;
    static HWND s_playBtn     = nullptr;
    static HWND s_timeLabel   = nullptr;
    static HWND s_parentWnd   = nullptr;
    static HFONT s_playerFont = nullptr;

    constexpr int kMinWidth     = 200;
    constexpr int kPlayerHeight = 34;
    constexpr int kBtnSize      = 26;
    constexpr int kPlayBtnId    = 101;
    constexpr int kPrevBtnId    = 102;
    constexpr int kNextBtnId    = 103;
    constexpr int kCloseBtnId   = 104;
    constexpr UINT_PTR kTimeTimer = 50;

    static int  s_curTrackMs  = 0;
    static int  s_trackLenMs  = 0;
    static int  s_curIndex    = -1;
    static bool s_isPlaying   = false;

    // --- MCI ---
    void MciSend(const std::wstring& cmd) { mciSendStringW(cmd.c_str(), nullptr, 0, nullptr); }

    int MciGetInt(const std::wstring& cmd)
    {
        wchar_t buf[32] = {};
        mciSendStringW(cmd.c_str(), buf, 32, nullptr);
        return _wtoi(buf);
    }

    void MciOpen(const std::wstring& path)
    {
        MciSend(L"close mp3");
        MciSend(L"open \"" + path + L"\" type mpegvideo alias mp3");
    }

    void MciPlay()
    {
        MciSend(L"play mp3");
        s_isPlaying = true;
        s_trackLenMs = MciGetInt(L"status mp3 length");
    }

    void MciPause()
    {
        MciSend(L"pause mp3");
        s_isPlaying = false;
    }

    void MciSeek(int posMs)
    {
        wchar_t buf[64];
        swprintf_s(buf, 64, L"seek mp3 to %d", posMs);
        MciSend(buf);
        s_curTrackMs = posMs;
    }

    void MciClose()
    {
        MciSend(L"close mp3");
        s_isPlaying = false;
        s_curTrackMs = 0;
    }

    // --- 文件列表 ---
    void RefreshMusicList()
    {
        s_musicFiles.clear();
        std::wstring pattern = GetAssetPath(L"music") + L"\\*.*";
        // 支持 wav / ogg / mp3
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                std::wstring name = fd.cFileName;
                if (name == L"." || name == L"..") continue;
                MusicFile mf;
                mf.name = name;
                mf.path = GetAssetPath(L"music") + L"\\" + name;
                s_musicFiles.push_back(mf);
            }
            while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
    }

    void EnsurePlayerFont()
    {
        if (!s_playerFont)
            s_playerFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                L"Microsoft YaHei UI");
    }

    void PositionPlayerWindow()
    {
        if (!s_playerWnd) return;
        int w = (g_pet.w > kMinWidth) ? g_pet.w : kMinWidth, h = kPlayerHeight;
        int x = g_pet.x + (g_pet.w - w) / 2;
        int y = g_pet.y - h - 10;
        if (x < 0) x = 0;
        if (y < 0) y = g_pet.y + g_pet.h + 10;
        SetWindowPos(s_playerWnd, HWND_TOPMOST, x, y, w, h, SWP_NOACTIVATE);
    }

    std::wstring FormatTime(int ms)
    {
        int sec = ms / 1000;
        int min = sec / 60; sec %= 60;
        wchar_t buf[16];
        swprintf_s(buf, 16, L"%02d:%02d", min, sec);
        return buf;
    }

    void UpdateTimeLabel()
    {
        if (!s_timeLabel) return;
        int cur = s_isPlaying ? MciGetInt(L"status mp3 position") : s_curTrackMs;
        std::wstring text = FormatTime(cur) + L" / " + FormatTime(s_trackLenMs);
        SetWindowTextW(s_timeLabel, text.c_str());
    }

    LRESULT CALLBACK PlayerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CREATE:
        {
            EnsurePlayerFont();
            HINSTANCE hi = GetModuleHandle(nullptr);
            int gap = 6, btnY = 4, leftX = 6;
            int petW = (g_pet.w > kMinWidth) ? g_pet.w : kMinWidth;

            // < 后退 10s
            CreateWindowW(L"BUTTON", L"<",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
                leftX, btnY, kBtnSize, kBtnSize, hwnd, (HMENU)(UINT_PTR)kPrevBtnId, hi, nullptr);

            // ▶/⏸
            s_playBtn = CreateWindowW(L"BUTTON", L"▶",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
                leftX + kBtnSize + gap, btnY, kBtnSize, kBtnSize, hwnd, (HMENU)(UINT_PTR)kPlayBtnId, hi, nullptr);
            SendMessageW(s_playBtn, WM_SETFONT, (WPARAM)s_playerFont, TRUE);

            // > 前进 10s
            CreateWindowW(L"BUTTON", L">",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
                leftX + (kBtnSize + gap) * 2, btnY, kBtnSize, kBtnSize, hwnd, (HMENU)(UINT_PTR)kNextBtnId, hi, nullptr);

            // 时间标签（按钮右侧 → ✕ 左侧）
            int timeLeft = leftX + (kBtnSize + gap) * 3;
            int timeW = petW - timeLeft - kBtnSize - 12;
            s_timeLabel = CreateWindowW(L"STATIC", L"00:00 / 00:00",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                timeLeft, btnY, timeW, kBtnSize, hwnd, nullptr, hi, nullptr);
            SendMessageW(s_timeLabel, WM_SETFONT, (WPARAM)s_playerFont, TRUE);

            // ✕
            CreateWindowW(L"BUTTON", L"X",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
                petW - kBtnSize - 4, btnY, kBtnSize, kBtnSize, hwnd, (HMENU)(UINT_PTR)kCloseBtnId, hi, nullptr);

            SetTimer(hwnd, kTimeTimer, 500, nullptr);
            break;
        }

        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(200, 140, 160));
            SetBkColor(hdc, RGB(255, 240, 245));
            return (LRESULT)CreateSolidBrush(RGB(255, 240, 245));
        }

        case WM_CTLCOLORBTN:
        {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkColor(hdc, RGB(255, 210, 220));
            return (LRESULT)CreateSolidBrush(RGB(255, 210, 220));
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case kPrevBtnId:
            {
                int pos = MciGetInt(L"status mp3 position");
                int seek = pos - 10000; if (seek < 0) seek = 0;
                MciSeek(seek); if (s_isPlaying) MciPlay();
                UpdateTimeLabel();
                break;
            }
            case kPlayBtnId:
                MusicToolPanel::TogglePlayPause();
                SetWindowTextW(s_playBtn, s_isPlaying ? L"⏸" : L"▶");
                break;
            case kNextBtnId:
            {
                int pos = MciGetInt(L"status mp3 position");
                int seek = pos + 10000;
                if (seek >= s_trackLenMs) seek = s_trackLenMs - 1000;
                MciSeek(seek); if (s_isPlaying) MciPlay();
                UpdateTimeLabel();
                break;
            }
            case kCloseBtnId:
                MusicToolPanel::HidePlayer();
                break;
            }
            break;
        }

        case WM_TIMER:
            if (wParam == kTimeTimer)
            {
                UpdateTimeLabel();
                // 播放完毕 → 自动重播
                if (s_trackLenMs > 0 && s_isPlaying)
                {
                    wchar_t mode[32] = {};
                    mciSendStringW(L"status mp3 mode", mode, 32, nullptr);
                    if (wcscmp(mode, L"stopped") == 0)
                    {
                        MciSeek(0);
                        MciPlay();
                    }
                }
            }
            break;

        case WM_DESTROY:
            KillTimer(hwnd, kTimeTimer);
            MciClose();
            s_playerWnd = nullptr;
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

    void EnsurePlayerWindow(HWND hwndParent)
    {
        if (s_playerWnd && IsWindow(s_playerWnd))
            return;

        s_parentWnd = hwndParent;
        EnsurePlayerFont();

        WNDCLASSW wc = {};
        wc.lpfnWndProc = PlayerWndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hbrBackground = CreateSolidBrush(RGB(255, 240, 245));
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"MusicPlayerWnd";
        static bool registered = false;
        if (!registered) { RegisterClassW(&wc); registered = true; }

        int w = (g_pet.w > kMinWidth) ? g_pet.w : kMinWidth;
        s_playerWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            wc.lpszClassName, nullptr, WS_POPUP,
            0, 0, w, kPlayerHeight,
            hwndParent, nullptr, GetModuleHandle(nullptr), nullptr);

        if (!s_playerWnd) return;

        PositionPlayerWindow();
        ShowWindow(s_playerWnd, SW_SHOW);
    }
}

// --- MusicToolPanel 实现 ---

void MusicToolPanel::Setup(UIActor&) {}

void MusicToolPanel::RefreshMusicList() { ::RefreshMusicList(); }

int MusicToolPanel::GetMusicCount()
{
    if (s_musicFiles.empty()) RefreshMusicList();
    return (int)s_musicFiles.size();
}

std::wstring MusicToolPanel::GetMusicName(int index)
{
    if (index < 0 || index >= (int)s_musicFiles.size()) return L"";
    return s_musicFiles[index].name;
}

void MusicToolPanel::PlayMusic(int index)
{
    if (index < 0 || index >= (int)s_musicFiles.size()) return;
    s_curIndex = index;
    MciOpen(s_musicFiles[index].path);
    MciPlay();
    EnsurePlayerWindow(s_parentWnd);
    SetWindowTextW(s_playBtn, L"⏸");
    UpdateTimeLabel();
}

bool MusicToolPanel::IsPlayerVisible()
{
    return s_playerWnd != nullptr && IsWindow(s_playerWnd);
}

void MusicToolPanel::HidePlayer()
{
    if (s_playerWnd) { DestroyWindow(s_playerWnd); s_playerWnd = nullptr; }
    MciClose();
}

void MusicToolPanel::TogglePlayPause()
{
    if (s_isPlaying) MciPause(); else MciPlay();
}

void MusicToolPanel::SeekTo(int posMs) { MciSeek(posMs); }
int  MusicToolPanel::GetCurrentPosition() { return s_curTrackMs; }
int  MusicToolPanel::GetTrackLength() { return s_trackLenMs; }

void MusicToolPanel::UpdatePosition()
{
    if (IsPlayerVisible()) PositionPlayerWindow();
}
