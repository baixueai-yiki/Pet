#include "CloseComponent.h"
#include "../UIPanels/ChatPanel/InputChatPanel.h"
#include "../UIPanels/ChatPanel/OptionChatPanel.h"
#include "../UIPanels/ToolPanel/SettingToolPanel.h"
#include "../UIPanels/ToolPanel/TaskToolPanel.h"
#include "../../../Runtime/StateManager.h"

void CloseComponent::OnInit(UIActor& actor)
{
    (void)actor;
}

void CloseComponent::OnMouseClick(UIActor& actor, int x, int y)
{
    (void)x;
    (void)y;

    auto s = StateGet(L"ui.panel");

    if (s != L"idle")
    {
        // 任何 UI 页面打开 → 全部关闭
        Setting::HideOverlay();
        InputChatPanel::Hide();
        OptionChatPanel::Hide();
        TaskToolPanel::Hide();
        InvalidateRect(actor.GetParent(), nullptr, TRUE);
    }
    else
    {
        // 无 UI 页面 → 展开输入栏
        InputChatPanel::Show(actor.GetParent());
    }
}
