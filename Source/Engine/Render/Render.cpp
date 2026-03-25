#include "Render.h"
#include "../../Core/Path.h"
#include "../../Game/Pet/Pet.h"
#include <gdiplus.h>
#include <vector>
#include <windows.h>

using namespace Gdiplus;

static Image* s_image = nullptr;
static ULONG_PTR s_token = 0;

static Image* LoadImageFromPaths(const std::vector<std::wstring>& paths)
{
    for (const auto& path : paths)
    {
        Image* img = Image::FromFile(path.c_str());
        if (img && img->GetLastStatus() == Ok)
            return img;
        delete img;
    }
    return nullptr;
}

bool RendererInit()
{
    if (s_image)
        return true;

    GdiplusStartupInput input;
    if (GdiplusStartup(&s_token, &input, nullptr) != Ok)
        return false;

    std::vector<std::wstring> candidates = {
        GetImagePath(L"character.png"),
        GetExeDir() + L"\\..\\Content\\Images\\character.png"
    };
    s_image = LoadImageFromPaths(candidates);
    if (!s_image)
    {
        delete s_image;
        s_image = nullptr;
        GdiplusShutdown(s_token);
        s_token = 0;
        return false;
    }

    g_pet.w = s_image->GetWidth();
    g_pet.h = s_image->GetHeight();

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    g_pet.x = (screenW - g_pet.w) / 2;
    g_pet.y = (screenH - g_pet.h) / 2;
    if (g_pet.x < 0) g_pet.x = 0;
    if (g_pet.y < 0) g_pet.y = 0;

    return true;
}

// 使用窗口 HDC 绘制一帧（旧版：拖动时会闪烁）
void RendererRender(HDC hdc)
{
    if (!hdc || !s_image || s_image->GetLastStatus() != Ok)
        return;

    Graphics graphics(hdc);
    graphics.Clear(Color(0, 0, 0, 0));
    graphics.DrawImage(s_image, g_pet.x, g_pet.y, g_pet.w, g_pet.h);
}

// 释放 GDI+ 资源，重置状态
void RendererShutdown()
{
    delete s_image;
    s_image = nullptr;

    if (s_token != 0)
    {
        GdiplusShutdown(s_token);
        s_token = 0;
    }

    g_pet.w = 0;
    g_pet.h = 0;
}
