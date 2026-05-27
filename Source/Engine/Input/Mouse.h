#pragma once

#include <windows.h>

namespace pet::engine::input {

struct MouseState {
    int x = 0;
    int y = 0;
    bool leftDown = false;
    bool rightDown = false;
    int wheelDelta = 0;
};

class Mouse {
public:
    static void OnMove(int x, int y);
    static void OnLeftDown();
    static void OnLeftUp();
    static void OnRightDown();
    static void OnRightUp();
    static void OnWheel(int delta);

    static MouseState GetState();
    static void ClearFrameDelta();
};

} // namespace pet::engine::input
