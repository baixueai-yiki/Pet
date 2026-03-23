#include "Renderer.h"
#include "../../Core/Path.h"
#include "../../Game/Pet/Pet.h"
#include <gdiplus.h>
#include <string>
#include <windows.h>

using namespace Gdiplus;

static Image* s_image = nullptr;
static ULONG_PTR s_token = 0;

bool RendererInit()
{
    if (s_image != nullptr)
        return true;

    GdiplusStartupInput input;
    if (GdiplusStartup(&s_token, &input, nullptr) != Ok)
        return false;

    std::wstring path = GetContentPath(L"..\\Content\\Images\\character.png");
    s_image = Image::FromFile(path.c_str());
    if (!s_image || s_image->GetLastStatus() != Ok)
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
    if (g_pet.x < 0) g_pet.x = 0;
    g_pet.y = (screenH - g_pet.h) / 2;
    if (g_pet.y < 0) g_pet.y = 0;

    return true;
}

void RendererRender(HDC hdc)
{
    if (!hdc || !s_image || s_image->GetLastStatus() != Ok)
        return;

    Graphics graphics(hdc);
    graphics.SetCompositingMode(CompositingModeSourceOver);
    graphics.Clear(Color(0, 0, 0, 0));
    graphics.DrawImage(s_image, g_pet.x, g_pet.y, g_pet.w, g_pet.h);
}

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
