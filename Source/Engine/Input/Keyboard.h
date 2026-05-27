#pragma once

#include <windows.h>

namespace pet::engine::input {

class Keyboard {
public:
    static void OnKeyDown(WPARAM key);
    static void OnKeyUp(WPARAM key);
    static bool IsDown(int vk);
};

} // namespace pet::engine::input
