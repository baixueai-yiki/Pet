#include "Engine/Input/Keyboard.h"

#include <array>

namespace pet::engine::input {
namespace {
std::array<bool, 256> g_keyDown = {};
}

void Keyboard::OnKeyDown(WPARAM key) {
    if (key < g_keyDown.size()) {
        g_keyDown[static_cast<size_t>(key)] = true;
    }
}

void Keyboard::OnKeyUp(WPARAM key) {
    if (key < g_keyDown.size()) {
        g_keyDown[static_cast<size_t>(key)] = false;
    }
}

bool Keyboard::IsDown(int vk) {
    if (vk < 0 || vk >= static_cast<int>(g_keyDown.size())) {
        return false;
    }
    return g_keyDown[static_cast<size_t>(vk)];
}

} // namespace pet::engine::input
