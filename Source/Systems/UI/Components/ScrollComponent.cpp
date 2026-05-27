#include "Systems/UI/Components/ScrollComponent.h"

#include "Runtime/EventBus.h"

namespace pet::systems::ui::components {

void ScrollComponent::AddDelta(int delta) {
    offset_ += delta;
    runtime::EventBus::Emit(L"ui.scroll.changed");
}

int ScrollComponent::GetOffset() const {
    return offset_;
}

void ScrollComponent::SetOffset(int offset) {
    offset_ = offset;
    runtime::EventBus::Emit(L"ui.scroll.changed");
}

} // namespace pet::systems::ui::components
