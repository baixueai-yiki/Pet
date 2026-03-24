#include "Renderer.h"
#include "../../Core/Path.h"
#include "../../Game/Pet/Pet.h"
#include <gdiplus.h>
#include <string>
#include <windows.h>

using namespace Gdiplus;

// 纹理句柄和 GDI+ 启动令牌，跨函数保存状态
static Image* s_image = nullptr;
static ULONG_PTR s_token = 0;

bool RendererInit()
{
    // 避免重复初始化
    if (s_image != nullptr)
        return true;

    GdiplusStartupInput input;
    if (GdiplusStartup(&s_token, &input, nullptr) != Ok)
        return false;

    // 根据相对于可执行文件的路径加载资源
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

    // 记录图像的尺寸，便于后续渲染与交互逻辑使用
    g_pet.w = s_image->GetWidth();
    g_pet.h = s_image->GetHeight();

    // 将宠物初始位置设置在屏幕中央，避免越界
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

    // 使用 GDI+ 图形上下文绘制图像，并保持透明通道
    Graphics graphics(hdc);
    graphics.SetCompositingMode(CompositingModeSourceOver);
    graphics.Clear(Color(0, 0, 0, 0));
    graphics.DrawImage(s_image, g_pet.x, g_pet.y, g_pet.w, g_pet.h);
}

void RendererShutdown()
{
    // 释放图像资源
    delete s_image;
    s_image = nullptr;

    if (s_token != 0)
    {
        GdiplusShutdown(s_token);
        s_token = 0;
    }

    // 重置宠物尺寸，防止后续逻辑以旧数据渲染
    g_pet.w = 0;
    g_pet.h = 0;
}
