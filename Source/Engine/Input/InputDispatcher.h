#pragma once
#include <windows.h>

// 把窗口消息中的鼠标输入分发到具体的交互逻辑中
void HandleInput(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void UpdatePetWindowRegion(HWND hwnd);
// 获取本次运行的戳桌宠次数
unsigned long long GetPokeCount();
