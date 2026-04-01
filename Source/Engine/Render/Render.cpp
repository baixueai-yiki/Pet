#include "Render.h"
#include "../../Core/Path.h"
#include "../../Systems/Pet/Pet.h"
#include "../../Systems/Setting/Setting.h"
#include <gdiplus.h>
#include <vector>
#include <windows.h>

using namespace Gdiplus;

// 全局持有的 GDI+ 图片对象，RendererInit 只加载一次
static Image* s_image = nullptr;
// GDI+ 启动标记，用于 RendererShutdown 时反注册
static ULONG_PTR s_token = 0;

// 尝试多个路径加载资源（优先 assets，再 fallback 到源文件夹）
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

    const int imgW = static_cast<int>(s_image->GetWidth());
    const int imgH = static_cast<int>(s_image->GetHeight());
    // 固定目标宽度为 200，等比缩放高度，方便不同设备下始终保持可见大小
    const int targetW = 200;
    if (imgW > 0 && imgH > 0)
    {
        double scale = static_cast<double>(targetW) / static_cast<double>(imgW);
        g_pet.w = targetW;
        g_pet.h = static_cast<int>(imgH * scale + 0.5);
    }
    else
    {
        g_pet.w = imgW;
        g_pet.h = imgH;
    }

    // 默认放到屏幕中央，如果缩放后出界则 clamp
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    g_pet.x = (screenW - g_pet.w) / 2;
    g_pet.y = (screenH - g_pet.h) / 2;
    if (g_pet.x < 0) g_pet.x = 0;
    if (g_pet.y < 0) g_pet.y = 0;

    return true;
}

// 渲染当前帧： 清空透明背景，然后把宠物贴图画在 g_pet 的区域
void RendererRender(HDC hdc)
{
    if (!hdc || !s_image || s_image->GetLastStatus() != Ok)
        return;

    Graphics graphics(hdc);
    graphics.Clear(Color(0, 0, 0, 0));
    graphics.DrawImage(s_image, g_pet.x, g_pet.y, g_pet.w, g_pet.h);
    Setting::RenderOverlay(hdc);
}

// 退出时清理图片与 GDI+（防止内存泄漏）
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
