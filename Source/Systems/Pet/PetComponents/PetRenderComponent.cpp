#include "PetRenderComponent.h"
#include "../../../Core/Path.h"
#include "../../../Core/PetState.h"
#include "../../../Engine/Render/Renderer.h"

void PetRenderComponent::OnInit(PetActor& actor)
{
    (void)actor;

    // 通过 Engine 加载图片
    if (!RendererLoadPetImage(GetImagePath(L"qing.png").c_str()))
        return;

    // 根据图片原始尺寸计算显示尺寸（固定宽度 200，等比缩放）
    int imgW = RendererGetImageWidth();
    int imgH = RendererGetImageHeight();
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

    // 位置：如果已有保存位置（非默认 120,120），保留；否则居中
    bool hasSavedPos = (g_pet.x != 120 || g_pet.y != 120);
    if (!hasSavedPos)
    {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        g_pet.x = (screenW - g_pet.w) / 2;
        g_pet.y = (screenH - g_pet.h) / 2;
        if (g_pet.x < 0) g_pet.x = 0;
        if (g_pet.y < 0) g_pet.y = 0;
    }
}

void PetRenderComponent::OnShutdown(PetActor& actor)
{
    (void)actor;
    // RendererShutdown 在 main.cpp 中统一调用，这里不需要处理
}
