#pragma once

// 宠物的位置、大小以及拖拽状态
// 放在 Core 层，供 Engine 和 Systems 共同访问
struct PetState
{
    int x;
    int y;
    int w;
    int h;
    bool isDragging;
    int dragOffsetX;
    int dragOffsetY;
};

extern PetState g_pet;
