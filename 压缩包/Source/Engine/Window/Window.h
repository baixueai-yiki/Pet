#pragma once
#include <windows.h>

// 主窗口的消息回调，负责将系统输入/绘制事件继续分发
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
// 创建透明顶层窗口供宠物渲染和交互使用
HWND CreateMainWindow(HINSTANCE hInstance);
