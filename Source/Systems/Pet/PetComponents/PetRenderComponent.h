#pragma once

#include "../PetActor.h"

// 规划桌宠的绘制：决定贴图、尺寸、位置
// 实际 GDI+ 绘制由 Engine/Renderer 执行
class PetRenderComponent : public PetComponent
{
public:
    void OnInit(PetActor& actor) override;
    void OnShutdown(PetActor& actor) override;
};
