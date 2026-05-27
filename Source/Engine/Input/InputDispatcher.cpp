#include "Engine/Input/InputDispatcher.h"

#include "Engine/Input/Keyboard.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Input/TextInputHandler.h"

#include <windowsx.h>

namespace pet::engine::input {

bool InputDispatcher::Dispatch(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_KEYDOWN:
        Keyboard::OnKeyDown(wParam);
        return true;
    case WM_KEYUP:
        Keyboard::OnKeyUp(wParam);
        return true;
    case WM_MOUSEMOVE:
        Mouse::OnMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return true;
    case WM_LBUTTONDOWN:
        Mouse::OnLeftDown();
        return true;
    case WM_LBUTTONUP:
        Mouse::OnLeftUp();
        return true;
    case WM_RBUTTONDOWN:
        Mouse::OnRightDown();
        return true;
    case WM_RBUTTONUP:
        Mouse::OnRightUp();
        return true;
    case WM_MOUSEWHEEL:
        Mouse::OnWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        return true;
    case WM_CHAR:
        TextInputHandler::OnChar(static_cast<wchar_t>(wParam));
        return true;
    default:
        return false;
    }
}

} // namespace pet::engine::input
