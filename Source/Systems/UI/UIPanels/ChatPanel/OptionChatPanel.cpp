#include "OptionChatPanel.h"
#include "../../../Pet/PetActor.h"
#include "../../../Pet/PetComponents/InputComponent.h"
#include "../../../Pet/PetComponents/ChatComponent.h"
#include "../../../../Runtime/StateManager.h"

constexpr int kInputWidth = 300;
constexpr int kInputHeight = 40;
constexpr int kButtonHeight = 80;

namespace
{
    static HFONT s_inputFont = nullptr;
    void EnsureInputFont() { if (!s_inputFont) s_inputFont = CreateFontW(20,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Microsoft YaHei UI"); }
    static HWND s_hButtonWnd = nullptr;
    static std::wstring s_buttonKey1;
    static std::wstring s_buttonKey2;

    LRESULT CALLBACK ButtonInputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
                InputComponent::HandleButtonInput(hwnd, key);
            }
            break;
        case WM_DESTROY:
            s_hButtonWnd = nullptr;
            StateSet(L"ui.panel", L"idle");
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }
}

void OptionChatPanel::Show(HWND hwndParent, const std::wstring& key1, const std::wstring& key2)
{
    s_buttonKey1 = key1;
    s_buttonKey2 = key2;
    StateSet(L"ui.panel", L"input");

    EnsureInputFont();

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

    int wndW = (g_pet.w > 0) ? g_pet.w : kInputWidth;
    int posX = g_pet.x;
    int posY = g_pet.y - kButtonHeight - 10;
    if (posY < 0) posY = g_pet.y + g_pet.h + 10;

    s_hButtonWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        nullptr,
        WS_POPUP,
        posX, posY,
        wndW,
        kButtonHeight,
        hwndParent,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr);

    if (!s_hButtonWnd)
        return;

    SetLayeredWindowAttributes(s_hButtonWnd, 0, 230, LWA_ALPHA);

    const int btnW = wndW - 40;
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

bool OptionChatPanel::IsVisible()
{
    return s_hButtonWnd != nullptr && IsWindow(s_hButtonWnd);
}

void OptionChatPanel::Hide()
{
    if (s_hButtonWnd)
    {
        DestroyWindow(s_hButtonWnd);
        s_hButtonWnd = nullptr;
    }
    StateSet(L"ui.panel", L"idle");
}

void OptionChatPanel::UpdatePosition()
{
    if (!s_hButtonWnd || !IsWindow(s_hButtonWnd))
        return;

    int wndW = (g_pet.w > 0) ? g_pet.w : kInputWidth;
    int posX = g_pet.x;
    int posY = g_pet.y - kButtonHeight - 10;
    if (posY < 0) posY = g_pet.y + g_pet.h + 10;

    SetWindowPos(s_hButtonWnd, HWND_TOPMOST, posX, posY, wndW, kButtonHeight, SWP_NOACTIVATE);
}
