#include <windows.h>
#include "Engine/Window/Window.h"
#include "Engine/Render/Render.h"
#include "../Game/Pet/Pet.h"

static HWND g_hwnd = nullptr;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    // 初始化宠物的状态数据，确保后续渲染与交互依赖的数据有效
    PetInit();
    

    // 启动渲染器并加载资源；失败时给出提示并退出
    if (!RendererInit())
    {
        MessageBoxW(nullptr, L"Failed to load Content\\Images\\character.png.", L"Pet", MB_OK | MB_ICONERROR);
        return -1;
    }

    // 创建透明的主窗口，作为宠物和输入消息的承载体
    g_hwnd = CreateMainWindow(hInstance);
    if (!g_hwnd)
    {
        RendererShutdown();
        return -1;
    }

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    // 进入消息循环，持续处理系统分发的事件（鼠标、窗口等）
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 退出前释放渲染器占用的资源
    RendererShutdown();
    return 0;
}
