#include "Systems/UI/Components/InputComponent.h"

#include "Runtime/EventBus.h"

namespace pet::systems::ui::components {

void InputComponent::SetText(const std::wstring& text) {
    text_ = text;
    runtime::EventBus::Emit(L"ui.input.changed", text_);
}

const std::wstring& InputComponent::GetText() const {
    return text_;
}

void InputComponent::Submit() {
    runtime::EventBus::Emit(L"ui.input.submit", text_);
}

} // namespace pet::systems::ui::components
