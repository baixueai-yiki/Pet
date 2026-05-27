#include "Systems/UI/Components/CloseComponent.h"

#include "Runtime/EventBus.h"

namespace pet::systems::ui::components {

void CloseComponent::RequestCloseAll() {
    requested_ = true;
    runtime::EventBus::Emit(L"ui.close_all");
}

bool CloseComponent::IsRequested() const {
    return requested_;
}

void CloseComponent::Reset() {
    requested_ = false;
}

} // namespace pet::systems::ui::components
