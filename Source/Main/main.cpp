#include <windows.h>
#include "Engine/Window/Window.h"
#include "Engine/Render/Renderer.h"
#include "Game/Pet/Pet.h"

static HWND g_hwnd = nullptr;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    PetInit();

    if (!RendererInit())
    {
        MessageBoxW(nullptr, L"Failed to load Content\\Images\\character.png.", L"Pet", MB_OK | MB_ICONERROR);
        return -1;
    }

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