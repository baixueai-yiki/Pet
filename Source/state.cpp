#include "state.h"
#include "common.h"

static PetState g_state = STATE_IDLE;

void SetState(PetState state)
{
    g_state = state;
}

PetState GetState()
{
    return g_state;
}