#include <windows.h>
#include "Engine/Window/WindowLifecycle.h"
#include "Engine/Render/Renderer.h"
#include "Systems/Pet/PetActor.h"

static HWND g_hwnd = nullptr;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    // 初始化宠物默认状态（尝试从 state.json 恢复上次位置）
    PetInit();

    // 启动 GDI+ 渲染引擎
    if (!RendererInit())
    {
        MessageBoxW(nullptr, L"Failed to initialize renderer.", L"MissCarrot", MB_OK | MB_ICONERROR);
        return -1;
    }

    // 创建透明主窗口（WM_CREATE 时触发 PetInitSystems → PetRenderComponent 加载图片）
    g_hwnd = CreateMainWindow(hInstance);
    if (!g_hwnd)
    {
        RendererShutdown();
        return -1;
    }

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    RendererShutdown();
    return 0;
}

