#include  "CloseComponent.h"

void CloseComponent::OnInit(UIActor& actor)
{
    (void)actor;
}

void CloseComponent::OnMouseClick(UIActor& actor, int x, int y)
{
    (void)x;
    (void)y;
    actor.Hide();
}
