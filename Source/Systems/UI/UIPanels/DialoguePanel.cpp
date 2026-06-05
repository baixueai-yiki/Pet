#include "DialoguePanel.h"
#include "Core/Path.h"
#include "Systems/Pet/PetComponents/AudioComponent.h"
#include "Systems/Pet/PetComponents/ChatComponent.h"
#include <windowsx.h>
#include <gdiplus.h>
#include <map>
#include <string>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

namespace
{
    static HWND s_imgWnd = nullptr, s_textWnd = nullptr;
    static Gdiplus::Image* s_bgImage = nullptr;
    static std::wstring s_curText, s_curKey, s_curMood;
    static std::vector<std::wstring> s_labels;
    static std::vector<RECT> s_btnRects;
    static std::map<std::wstring, DialoguePanel::Entry> s_labelMap;
    static HFONT s_textFont = nullptr;
    static int s_runIndex = 0;
    static std::vector<DialoguePanel::Entry> s_entries;

    constexpr int kPanelHeight = 220;

    Gdiplus::Image* LoadImage(const std::wstring& p) {
        auto* img = Gdiplus::Image::FromFile(p.c_str());
        return (img && img->GetLastStatus() == Gdiplus::Ok) ? img : nullptr;
    }

    void EnsureImgWindow();
    LRESULT CALLBACK ImgWndProc(HWND,UINT,WPARAM,LPARAM);
    LRESULT CALLBACK TextWndProc(HWND,UINT,WPARAM,LPARAM);

    void ShowCurrent()
    {
        if (s_runIndex < 0 || s_runIndex >= (int)s_entries.size()) return;
        s_curText = std::get<1>(s_entries[s_runIndex]);
        s_curMood = std::get<2>(s_entries[s_runIndex]);
        s_curKey  = std::get<0>(s_entries[s_runIndex]);
        s_labels  = std::get<4>(s_entries[s_runIndex]);
        s_btnRects.resize(s_labels.size());

        delete s_bgImage; s_bgImage = nullptr;
        std::wstring imgPath = GetImagePath(s_curMood + L".png");
        s_bgImage = LoadImage(imgPath);
        if (s_bgImage) { if (!s_imgWnd) EnsureImgWindow(); if (s_imgWnd) { ShowWindow(s_imgWnd, SW_SHOW); InvalidateRect(s_imgWnd, nullptr, TRUE); } }
        else if (s_imgWnd) { ShowWindow(s_imgWnd, SW_HIDE); }

        std::wstring act = std::get<3>(s_entries[s_runIndex]);
        if (!act.empty()) AudioComponent::PlayAudioAsset(L"audio\\" + act);
        else AudioComponent::PlayAudioAuto(L"audio\\" + s_curKey);

        if (s_textWnd) InvalidateRect(s_textWnd, nullptr, TRUE);
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
        case WM_NCHITTEST: return HTCLIENT;
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

                int n = (int)s_labels.size();
                if (n) {
                    int bh = 36, bw = 140, bx = (rc.right - bw)/2;
                    int totalH = bh*n + 4*(n-1);
                    int startY = (rc.bottom - totalH)/2;
                    SetTextColor(hdc, RGB(255,255,255));
                    for (int i=0;i<n;i++) {
                        s_btnRects[i] = {bx, startY + i*(bh+4), bx+bw, startY + i*(bh+4)+bh};
                        RECT& br = s_btnRects[i];
                        HBRUSH bb = CreateSolidBrush(RGB(240,180,200)); FillRect(hdc,&br,bb); DeleteObject(bb);
                        HPEN bp = CreatePen(PS_SOLID,1,RGB(255,255,255));
                        SelectObject(hdc,bp); SelectObject(hdc,GetStockObject(NULL_BRUSH));
                        Rectangle(hdc,br.left,br.top,br.right,br.bottom); DeleteObject(bp);
                        RECT lr = {br.left+8,br.top+4,br.right-8,br.bottom-4};
                        DrawTextW(hdc,s_labels[i].c_str(),-1,&lr,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
                    }
                }
            }
            EndPaint(hwnd, &ps); return 0;
        }
        case WM_LBUTTONDOWN: {
            POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
            bool hitBtn = false;
            for (int i=0;i<(int)s_labels.size();i++) {
                if (PtInRect(&s_btnRects[i], pt)) {
                    auto it = s_labelMap.find(s_labels[i]);
                    if (it != s_labelMap.end()) {
                        s_curText = std::get<1>(it->second);
                        std::wstring act = std::get<3>(it->second);
                        if (!act.empty()) AudioComponent::PlayAudioAsset(L"audio\\" + act);
                        s_labels.clear(); s_btnRects.clear();
                        InvalidateRect(hwnd, nullptr, TRUE);
                        hitBtn = true;
                        break;
                    }
                }
            }
            if (!hitBtn) DialoguePanel::AdvanceOrHide();
            break;
        }
        case WM_RBUTTONDOWN: case WM_MBUTTONDOWN: DialoguePanel::AdvanceOrHide(); break;
        case WM_DESTROY: s_textWnd = nullptr; break;
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }

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
}

void DialoguePanel::Show(const std::vector<Entry>& entries)
{
    s_entries = entries;
    s_labelMap.clear();
    {
        std::map<std::wstring, std::wstring> allMap;
        if (LoadIdleMap(allMap)) {
            for (const auto& kv : allMap) {
                std::wstring raw = kv.second, text = raw, mood = L"happy", action, l1, l2;
                size_t comma = text.rfind(L'，'); if (comma==std::wstring::npos) comma=text.rfind(L',');
                if (comma!=std::wstring::npos) { mood=text.substr(comma+1); text=text.substr(0,comma); }
                std::wstring parts[4]; int pi=0; size_t pp=0;
                while(pi<4){size_t nx=mood.find(L'|',pp); parts[pi]=mood.substr(pp,nx-pp);pi++;if(nx==std::wstring::npos)break;pp=nx+1;}
                mood=parts[0]; action=parts[1]; l1=parts[2]; l2=parts[3];
                std::vector<std::wstring> labels;
                if (!l1.empty()) labels.push_back(l1);
                if (!l2.empty()) labels.push_back(l2);
                s_labelMap[kv.first] = {kv.first, text, mood, action, labels};
            }
        }
    }
    s_runIndex = 0;

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
    if (s_textWnd) { DestroyWindow(s_textWnd); s_textWnd = nullptr; }
    if (s_imgWnd) { DestroyWindow(s_imgWnd); s_imgWnd = nullptr; }
    delete s_bgImage; s_bgImage = nullptr;
    s_entries.clear();
    s_labels.clear();
}
