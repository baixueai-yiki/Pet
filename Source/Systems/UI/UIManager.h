#pragma once

#include "UIActor.h"
#include <windows.h>

namespace UI
{
    void Initialize(HWND parent);
    void Shutdown();

    UIActor& GetActor();
    void DispatchMouseClick(int x, int y);
    void DispatchMouseWheel(int delta);
}
