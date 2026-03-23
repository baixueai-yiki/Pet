#include "Pet.h"

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
    // Placeholder for future behavior logic.
}
