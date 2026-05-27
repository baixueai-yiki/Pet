#include "UIManager.h"
#include "UIComponents/CloseComponent.h"
#include "UIComponents/ScrollComponent.h"
#include "UIComponents/DragComponent.h"
#include "UIComponents/InputComponent.h"
#include "UIPanels/ChatPanels.h"
#include "UIPanels/TaskPanels.h"
#include "UIPanels/SettingPanels.h"
#include "UIPanels/OverlayPanels.h"

#include <memory>

namespace UI
{
    static std::unique_ptr<UIActor> s_actor;

    void Initialize(HWND parent)
    {
        if (!s_actor)
            s_actor = std::make_unique<UIActor>(L"MainUI");

        s_actor->AddComponent(std::make_unique<CloseComponent>());
        s_actor->AddComponent(std::make_unique<ScrollComponent>());
        s_actor->AddComponent(std::make_unique<DragComponent>());
        s_actor->AddComponent(std::make_unique<InputComponent>());
        s_actor->Initialize(parent);
        s_actor->Show();

        ChatPanels::Setup(*s_actor);
        TaskPanels::Setup(*s_actor);
        SettingPanels::Setup(*s_actor);
        OverlayPanels::Setup(*s_actor);
    }

    void Shutdown()
    {
        if (s_actor)
        {
            s_actor->Shutdown();
            s_actor.reset();
        }
    }

    UIActor& GetActor()
    {
        if (!s_actor)
            s_actor = std::make_unique<UIActor>(L"MainUI");
        return *s_actor;
    }

    void DispatchMouseClick(int x, int y)
    {
        if (s_actor)
            s_actor->NotifyMouseClick(x, y);
    }

    void DispatchMouseWheel(int delta)
    {
        if (s_actor)
            s_actor->NotifyMouseWheel(delta);
    }
}
