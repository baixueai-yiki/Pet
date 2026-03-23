#pragma once

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

enum class PetMood
{
    Idle,
    Curious,
    Sleepy,
};

inline const wchar_t* PetMoodToString(PetMood mood)
{
    switch (mood)
    {
    case PetMood::Curious: return L"好奇";
    case PetMood::Sleepy: return L"想睡觉";
    default: return L"安静";
    }
}

void PetInit();
void PetBehaviorTick();
