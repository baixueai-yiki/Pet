#pragma once

#include "../UIActor.h"

class CloseComponent : public UIComponent
{
public:
    void OnInit(UIActor& actor) override;
    void OnMouseClick(UIActor& actor, int x, int y) override;
};
