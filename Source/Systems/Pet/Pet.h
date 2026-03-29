#pragma once

// 记录宠物的位置、大小以及拖拽状态
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

// 宠物当前的情绪枚举
enum class PetMood
{
    Idle,
    Curious,
    Sleepy,
};

// 把情绪枚举转换为可显示的中文字符串
inline const wchar_t* PetMoodToString(PetMood mood)
{
    switch (mood)
    {
    case PetMood::Curious: return L"好奇";
    case PetMood::Sleepy: return L"想睡觉";
    default: return L"安静";
    }
}

// 初始化宠物位置与交互状态
void PetInit();
// 后续可以在此驱动宠物行为（如动画或状态切换）
void PetBehaviorTick();
