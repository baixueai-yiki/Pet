#include "Systems/UI/Components/DragComponent.h"

#include "Runtime/EventBus.h"

namespace pet::systems::ui::components {

void DragComponent::Begin(int x, int y) {
    state_.dragging = true;
    state_.startX = x;
    state_.startY = y;
    state_.currentX = x;
    state_.currentY = y;
    runtime::EventBus::Emit(L"ui.drag.begin");
}

void DragComponent::MoveTo(int x, int y) {
    if (!state_.dragging) {
        return;
    }
    state_.currentX = x;
    state_.currentY = y;
    runtime::EventBus::Emit(L"ui.drag.move");
}

void DragComponent::End() {
    if (!state_.dragging) {
        return;
    }
    state_.dragging = false;
    runtime::EventBus::Emit(L"ui.drag.end");
}

const DragState& DragComponent::GetState() const {
    return state_;
}

} // namespace pet::systems::ui::components
