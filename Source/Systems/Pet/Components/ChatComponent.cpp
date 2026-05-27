#include "Systems/Pet/Components/ChatComponent.h"

#include "Runtime/EventBus.h"

namespace pet::systems::pet::components {

void ChatComponent::Say(const std::wstring& text) {
    lastMessage_ = text;
    runtime::EventBus::Emit(L"pet.chat.message", text);
}

const std::wstring& ChatComponent::GetLastMessage() const {
    return lastMessage_;
}

} // namespace pet::systems::pet::components
