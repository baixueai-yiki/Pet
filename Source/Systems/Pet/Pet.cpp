#include "Pet.h"

// 全局宠物状态实例，在渲染与输入逻辑中共享
PetState g_pet = { 120, 120, 0, 0, false, 0, 0 };

void PetInit()
{
    g_pet.x = 120;
    g_pet.y = 120;
    g_pet.w = 0;
    g_pet.h = 0;
    g_pet.isDragging = false;
    g_pet.dragOffsetX = 0;
    g_pet.dragOffsetY = 0;
}

void PetBehaviorTick()
{
    // 占位符，未来可在此添加宠物自动行为、状态切换等逻辑
}
