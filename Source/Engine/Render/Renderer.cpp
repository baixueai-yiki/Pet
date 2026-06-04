#include "Renderer.h"
#include "../../Core/Path.h"
#include "../../Core/PetState.h"
#include "../../Systems/UI/UIPanels/ToolPanel/SettingToolPanel.h"
#include <gdiplus.h>
#include <windows.h>

using namespace Gdiplus;

static Image* s_image = nullptr;
static ULONG_PTR s_token = 0;

// GDI+ 启动（纯 Engine 职责）
bool RendererInit()
{
    if (s_token != 0)
        return true;

    GdiplusStartupInput input;
    if (GdiplusStartup(&s_token, &input, nullptr) != Ok)
        return false;
    return true;
}

// 加载宠物图片（Engine 提供能力，Systems 决定何时/加载什么）
bool RendererLoadPetImage(const wchar_t* path)
{
    if (s_image)
    {
        delete s_image;
        s_image = nullptr;
    }
    if (!path || !path[0])
        return false;

    Image* img = Image::FromFile(path);
    if (!img || img->GetLastStatus() != Ok)
    {
        delete img;
        return false;
    }
    s_image = img;
    return true;
}

int RendererGetImageWidth()
{
    if (!s_image) return 0;
    return static_cast<int>(s_image->GetWidth());
}

int RendererGetImageHeight()
{
    if (!s_image) return 0;
    return static_cast<int>(s_image->GetHeight());
}

// 渲染当前帧
void RendererRender(HDC hdc)
{
    if (!hdc || !s_image || s_image->GetLastStatus() != Ok)
        return;

    Graphics graphics(hdc);
    graphics.Clear(Color(0, 0, 0, 0));
    graphics.DrawImage(s_image, g_pet.x, g_pet.y, g_pet.w, g_pet.h);
    Setting::RenderOverlay(hdc);
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
