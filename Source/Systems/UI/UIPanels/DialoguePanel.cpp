#include "DialoguePanel.h"
#include "Core/Path.h"
#include "Systems/Pet/PetComponents/AudioComponent.h"
#include <gdiplus.h>
#include <string>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

namespace
{
    static HWND s_imgWnd = nullptr;
    static HWND s_textWnd = nullptr;
    static Gdiplus::Image* s_bgImage = nullptr;
    static std::wstring s_curText;
    static std::wstring s_curKey;
    static std::wstring s_curMood;
    static HFONT s_textFont = nullptr;
    static int s_runIndex = 0;
    static std::vector<DialoguePanel::Entry> s_entries;

    constexpr int kPanelHeight = 220;

    void EnsureImgWindow();
    Gdiplus::Image* LoadImage(const std::wstring& path)
    {
        Gdiplus::Image* img = Gdiplus::Image::FromFile(path.c_str());
        return (img && img->GetLastStatus() == Gdiplus::Ok) ? img : nullptr;
    }

    void ShowCurrent()
    {
        if (s_runIndex < 0 || s_runIndex >= (int)s_entries.size()) return;
        s_curText = std::get<1>(s_entries[s_runIndex]);
        s_curMood  = std::get<2>(s_entries[s_runIndex]);
        s_curKey   = std::get<0>(s_entries[s_runIndex]);

        delete s_bgImage; s_bgImage = nullptr;
        std::wstring imgPath = GetImagePath(s_curMood + L".png");
        s_bgImage = LoadImage(imgPath);

        if (s_bgImage) { if (!s_imgWnd) EnsureImgWindow(); if (s_imgWnd) { ShowWindow(s_imgWnd, SW_SHOW); InvalidateRect(s_imgWnd, nullptr, TRUE); } }
        else if (s_imgWnd) { ShowWindow(s_imgWnd, SW_HIDE); }

        if (s_textWnd) InvalidateRect(s_textWnd, nullptr, TRUE);
        AudioComponent::PlayAudioAuto(L"audio\\" + s_curKey);
    }

    LRESULT CALLBACK ImgWndProc(HWND,UINT,WPARAM,LPARAM);
    LRESULT CALLBACK TextWndProc(HWND,UINT,WPARAM,LPARAM);

    void EnsureImgWindow()
    {
        if (s_imgWnd) return;
        WNDCLASSW wc={}; wc.lpfnWndProc=ImgWndProc; wc.hInstance=GetModuleHandle(nullptr);
        wc.hbrBackground=CreateSolidBrush(RGB(255,0,255)); wc.lpszClassName=L"DIW";
        static bool reg=false; if(!reg){RegisterClassW(&wc);reg=true;}
        int sw=GetSystemMetrics(SM_CXSCREEN), sh=GetSystemMetrics(SM_CYSCREEN);
        int ww=sw*4/5, wh=kPanelHeight, wx=(sw-ww)/2, wy=sh-wh;
        s_imgWnd=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED|WS_EX_TRANSPARENT,wc.lpszClassName,nullptr,WS_POPUP,wx,wy,ww,wh,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);
        if(s_imgWnd){SetLayeredWindowAttributes(s_imgWnd,RGB(255,0,255),0,LWA_COLORKEY);ShowWindow(s_imgWnd,SW_SHOW);}
    }

    LRESULT CALLBACK ImgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_PAINT) {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps); RECT rc; GetClientRect(hwnd, &rc);
            HBRUSH bg = CreateSolidBrush(RGB(255,0,255)); FillRect(hdc, &rc, bg); DeleteObject(bg);
            if (s_bgImage) { Gdiplus::Graphics g(hdc); g.DrawImage(s_bgImage, 0, 0, rc.right, rc.bottom); }
            EndPaint(hwnd, &ps); return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT CALLBACK TextWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_CREATE:
            if (!s_textFont) s_textFont = CreateFontW(32,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"SimHei");
            break;
        case WM_ERASEBKGND: return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps); RECT rc; GetClientRect(hwnd, &rc);
            HBRUSH bg = CreateSolidBrush(RGB(255,0,255)); FillRect(hdc, &rc, bg); DeleteObject(bg);
            if (!s_curText.empty()) {
                SetBkMode(hdc, TRANSPARENT);
                if (s_textFont) SelectObject(hdc, s_textFont);
                RECT tr = {40, 20, rc.right-40, rc.bottom-20};
                SetTextColor(hdc, RGB(210,130,170));
                for (int dx=-1;dx<=1;dx+=2) for(int dy=-1;dy<=1;dy+=2)
                { RECT o={tr.left+dx,tr.top+dy,tr.right+dx,tr.bottom+dy}; DrawTextW(hdc,s_curText.c_str(),-1,&o,DT_CENTER|DT_WORDBREAK|DT_VCENTER); }
                SetTextColor(hdc, RGB(255,255,255));
                DrawTextW(hdc, s_curText.c_str(), -1, &tr, DT_CENTER|DT_WORDBREAK|DT_VCENTER);
            }
            EndPaint(hwnd, &ps); return 0;
        }
        case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN: DialoguePanel::AdvanceOrHide(); break;
        case WM_DESTROY: s_textWnd = nullptr; break;
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }
}

void DialoguePanel::Show(const std::vector<Entry>& entries)
{
    s_entries = entries;
    s_runIndex = 0;

    if (s_entries.empty()) return;

    if (!s_textWnd || !IsWindow(s_textWnd))
    {
        WNDCLASSW wc={}; wc.lpfnWndProc=TextWndProc; wc.hInstance=GetModuleHandle(nullptr);
        wc.hbrBackground=CreateSolidBrush(RGB(255,0,255)); wc.lpszClassName=L"DTW";
        static bool reg=false; if(!reg){RegisterClassW(&wc);reg=true;}
        int sw=GetSystemMetrics(SM_CXSCREEN), sh=GetSystemMetrics(SM_CYSCREEN);
        int ww=sw*4/5, wh=kPanelHeight, wx=(sw-ww)/2, wy=sh-wh;
        s_textWnd=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED,wc.lpszClassName,nullptr,WS_POPUP,wx,wy,ww,wh,nullptr,nullptr,GetModuleHandle(nullptr),nullptr);
        if(s_textWnd){SetLayeredWindowAttributes(s_textWnd,RGB(255,0,255),0,LWA_COLORKEY);ShowWindow(s_textWnd,SW_SHOW);}
    }

    ShowCurrent();
}

void DialoguePanel::AdvanceOrHide()
{
    if (!s_textWnd) return;
    ++s_runIndex;
    if (s_runIndex >= (int)s_entries.size()) { Hide(); return; }
    ShowCurrent();
}

bool DialoguePanel::IsVisible()
{
    return s_textWnd != nullptr && IsWindow(s_textWnd);
}

void DialoguePanel::Hide()
{
    if (s_imgWnd) { DestroyWindow(s_imgWnd); s_imgWnd = nullptr; }
    if (s_textWnd) { DestroyWindow(s_textWnd); s_textWnd = nullptr; }
    delete s_bgImage; s_bgImage = nullptr;
    s_entries.clear();
}
