#pragma once

enum PetState
{
    STATE_IDLE,
    STATE_TALK,
    STATE_DRAG
};

void SetState(PetState state);
PetState GetState();