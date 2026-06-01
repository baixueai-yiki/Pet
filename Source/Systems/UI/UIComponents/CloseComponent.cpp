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

    // 通过 Runtime/StateManager 的 ui.panel 维度判断当前状态
    auto s = StateGet(L"ui.panel");

    if (s != L"idle" && s != L"input")
    {
        // 有 UI 页面打开（overlay_* 等）→ 全部关闭
        Setting::HideOverlay();
        InputChatPanel::Hide();
        OptionChatPanel::Hide();
        TaskToolPanel::Hide();
        InvalidateRect(actor.GetParent(), nullptr, TRUE);
    }
    else if (s == L"idle")
    {
        // 无 UI 页面 → 展开输入栏
        InputChatPanel::Show(actor.GetParent());
    }
    // s == "input" 时不做任何事（输入栏已打开，右键不重复打开）
}
