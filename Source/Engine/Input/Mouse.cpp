#include "Engine/Input/Mouse.h"

namespace pet::engine::input {
namespace {
MouseState g_mouse;
}

void Mouse::OnMove(int x, int y) { g_mouse.x = x; g_mouse.y = y; }
void Mouse::OnLeftDown() { g_mouse.leftDown = true; }
void Mouse::OnLeftUp() { g_mouse.leftDown = false; }
void Mouse::OnRightDown() { g_mouse.rightDown = true; }
void Mouse::OnRightUp() { g_mouse.rightDown = false; }
void Mouse::OnWheel(int delta) { g_mouse.wheelDelta += delta; }

MouseState Mouse::GetState() { return g_mouse; }

void Mouse::ClearFrameDelta() { g_mouse.wheelDelta = 0; }

} // namespace pet::engine::input
