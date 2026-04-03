#pragma once

#include "../UIActor.h"

class ScrollComponent : public UIComponent
{
public:
    void OnMouseWheel(UIActor& actor, int delta) override;
};
