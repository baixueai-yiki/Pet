#include "Systems/UI/Panels/ToolPanel/TaskToolPanel.h"

#include "Runtime/EventBus.h"

namespace pet::systems::ui::panels::tool {
namespace {
bool g_open = false;
}

void TaskToolPanel::Setup() {
    runtime::EventBus::Emit(L"ui.panel.tool.task.setup");
}

void TaskToolPanel::Open() {
    g_open = true;
    runtime::EventBus::Emit(L"ui.panel.tool.task.open");
}

void TaskToolPanel::Close() {
    g_open = false;
    runtime::EventBus::Emit(L"ui.panel.tool.task.close");
}

bool TaskToolPanel::IsOpen() {
    return g_open;
}

} // namespace pet::systems::ui::panels::tool
