#pragma once

#include "../UIActor.h"

class DragComponent : public UIComponent
{
public:
    void OnMouseClick(UIActor& actor, int x, int y) override;
};
