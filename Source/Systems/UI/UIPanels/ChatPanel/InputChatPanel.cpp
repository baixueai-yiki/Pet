#include "InputChatPanel.h"
#include "ChatPanelInternal.h"
#include "../../../Pet/PetActor.h"
#include "../../../Pet/PetComponents/ChatComponent.h"
#include "../../../../Runtime/StateManager.h"
#include <imm.h>

namespace
{
    static HWND s_hInputWnd = nullptr;
    static std::wstring s_inputText;
    static std::wstring s_imeText;
    static POINT s_dragOffset = {};
    static bool s_dragging = false;

    LRESULT CALLBACK TextInputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
            if (g_inputFont)
                SelectObject(hdc, g_inputFont);
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
            s_dragOffset.x = GET_X_LPARAM(lParam);
            s_dragOffset.y = GET_Y_LPARAM(lParam);
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
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ClientToScreen(hwnd, &pt);
                SetWindowPos(hwnd, HWND_TOPMOST, pt.x - s_dragOffset.x, pt.y - s_dragOffset.y,
                    0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
            }
            break;
        case WM_CHAR:
            if (wParam == VK_RETURN)
            {
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
                s_inputText.push_back(static_cast<wchar_t>(wParam));
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            break;
        case WM_IME_COMPOSITION:
        {
            HIMC hImc = ImmGetContext(hwnd);
            if (hImc)
            {
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
            s_inputText.clear();
            s_imeText.clear();
            s_hInputWnd = nullptr;
            StateSet(L"ui.panel", L"idle");
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        return 0;
    }
}

void InputChatPanel::Show(HWND hwndParent)
{
    StateSet(L"ui.panel", L"input");
    EnsureFonts();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = TextInputProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"ChatInputWnd";
    static bool registered = false;
    if (!registered)
    {
        RegisterClassW(&wc);
        registered = true;
    }

    if (s_hInputWnd)
    {
        DestroyWindow(s_hInputWnd);
        s_hInputWnd = nullptr;
    }

    s_inputText.clear();
    s_imeText.clear();

    int wndW = (g_pet.w > 0) ? g_pet.w : kInputWidth;
    int posX = g_pet.x;
    int posY = g_pet.y - kInputHeight - 10;
    if (posY < 0) posY = g_pet.y + g_pet.h + 10;

    s_hInputWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        wc.lpszClassName,
        nullptr,
        WS_POPUP,
        posX, posY,
        wndW,
        kInputHeight,
        hwndParent,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr);

    if (!s_hInputWnd)
        return;

    SetLayeredWindowAttributes(s_hInputWnd, 0, 220, LWA_ALPHA);
    if (g_inputFont)
        SendMessageW(s_hInputWnd, WM_SETFONT, (WPARAM)g_inputFont, TRUE);
    ShowWindow(s_hInputWnd, SW_SHOW);
    SetFocus(s_hInputWnd);
}

bool InputChatPanel::IsVisible()
{
    return s_hInputWnd != nullptr && IsWindow(s_hInputWnd);
}

void InputChatPanel::Hide()
{
    if (s_hInputWnd)
    {
        DestroyWindow(s_hInputWnd);
        s_hInputWnd = nullptr;
    }
    s_inputText.clear();
    s_imeText.clear();
    StateSet(L"ui.panel", L"idle");
}

void InputChatPanel::UpdatePosition()
{
    if (!s_hInputWnd || !IsWindow(s_hInputWnd))
        return;

    int wndW = (g_pet.w > 0) ? g_pet.w : kInputWidth;
    int posX = g_pet.x;
    int posY = g_pet.y - kInputHeight - 10;
    if (posY < 0) posY = g_pet.y + g_pet.h + 10;

    SetWindowPos(s_hInputWnd, HWND_TOPMOST, posX, posY, wndW, kInputHeight, SWP_NOACTIVATE);
}
