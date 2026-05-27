#include "Systems/Pet/Components/InputComponent.h"

#include "Runtime/EventBus.h"

namespace pet::systems::pet::components {

void InputComponent::Submit(const std::wstring& inputText) {
    lastInput_ = inputText;
    runtime::EventBus::Emit(L"pet.input.submit", inputText);
}

const std::wstring& InputComponent::GetLastInput() const {
    return lastInput_;
}

} // namespace pet::systems::pet::components
