#include "Engine/Input/InputDispatcher.h"

#include "Engine/Input/Keyboard.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Input/TextInputHandler.h"
#include "Runtime/EventBus.h"

#include <windowsx.h>

namespace pet::engine::input {

bool InputDispatcher::Dispatch(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_KEYDOWN:
        Keyboard::OnKeyDown(wParam);
        runtime::EventBus::Emit(L"engine.input.key.down", std::wstring(1, static_cast<wchar_t>(wParam)));
        return true;
    case WM_KEYUP:
        Keyboard::OnKeyUp(wParam);
        runtime::EventBus::Emit(L"engine.input.key.up", std::wstring(1, static_cast<wchar_t>(wParam)));
        return true;
    case WM_MOUSEMOVE:
        Mouse::OnMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        runtime::EventBus::Emit(L"engine.input.mouse.move");
        return true;
    case WM_LBUTTONDOWN:
        Mouse::OnLeftDown();
        runtime::EventBus::Emit(L"engine.input.mouse.left.down");
        return true;
    case WM_LBUTTONUP:
        Mouse::OnLeftUp();
        runtime::EventBus::Emit(L"engine.input.mouse.left.up");
        return true;
    case WM_RBUTTONDOWN:
        Mouse::OnRightDown();
        runtime::EventBus::Emit(L"engine.input.mouse.right.down");
        return true;
    case WM_RBUTTONUP:
        Mouse::OnRightUp();
        runtime::EventBus::Emit(L"engine.input.mouse.right.up");
        return true;
    case WM_MOUSEWHEEL:
        Mouse::OnWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        runtime::EventBus::Emit(L"engine.input.mouse.wheel");
        return true;
    case WM_CHAR:
        TextInputHandler::OnChar(static_cast<wchar_t>(wParam));
        runtime::EventBus::Emit(L"engine.input.char", std::wstring(1, static_cast<wchar_t>(wParam)));
        return true;
    default:
        return false;
    }
}

} // namespace pet::engine::input
