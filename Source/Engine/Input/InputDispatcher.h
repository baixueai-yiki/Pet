#pragma once

#include <windows.h>

namespace pet::engine::input {

class InputDispatcher {
public:
    static bool Dispatch(UINT msg, WPARAM wParam, LPARAM lParam);
};

} // namespace pet::engine::input
