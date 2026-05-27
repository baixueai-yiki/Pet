#include "Systems/UI/Panels/ToolPanel/SettingToolPanel.h"

#include "Runtime/EventBus.h"

namespace pet::systems::ui::panels::tool {
namespace {
bool g_open = false;
}

void SettingToolPanel::Setup() {
    runtime::EventBus::Emit(L"ui.panel.tool.setting.setup");
}

void SettingToolPanel::Open() {
    g_open = true;
    runtime::EventBus::Emit(L"ui.panel.tool.setting.open");
}

void SettingToolPanel::Close() {
    g_open = false;
    runtime::EventBus::Emit(L"ui.panel.tool.setting.close");
}

bool SettingToolPanel::IsOpen() {
    return g_open;
}

} // namespace pet::systems::ui::panels::tool
