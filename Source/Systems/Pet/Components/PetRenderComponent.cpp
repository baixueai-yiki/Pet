#include "Systems/Pet/Components/PetRenderComponent.h"

#include "Runtime/EventBus.h"

namespace pet::systems::pet::components {

void PetRenderComponent::SetPosition(int x, int y) {
    state_.x = x;
    state_.y = y;
    runtime::EventBus::Emit(L"pet.render.changed", L"position");
}

void PetRenderComponent::SetSize(int width, int height) {
    state_.width = width;
    state_.height = height;
    runtime::EventBus::Emit(L"pet.render.changed", L"size");
}

const RenderState& PetRenderComponent::GetState() const {
    return state_;
}

} // namespace pet::systems::pet::components
