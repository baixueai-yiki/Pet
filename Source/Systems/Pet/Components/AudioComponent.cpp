#include "Systems/Pet/Components/AudioComponent.h"

#include "Runtime/EventBus.h"

namespace pet::systems::pet::components {

void AudioComponent::Play(const std::wstring& clipId) {
    lastClipId_ = clipId;
    runtime::EventBus::Emit(L"pet.audio.play", clipId);
}

const std::wstring& AudioComponent::GetLastClipId() const {
    return lastClipId_;
}

} // namespace pet::systems::pet::components
