#include "Systems/Pet/PetActor.h"

#include "Core/Timer.h"
#include "Runtime/EventBus.h"
#include "Runtime/StateManager.h"

namespace pet::systems::pet {

PetActor& PetActor::Get() {
    static PetActor instance;
    return instance;
}

void PetActor::Initialize() {
    if (initialized_) {
        return;
    }

    EnsureMindStateReady();
    RegisterRuntimeEvents();

    render_.SetPosition(120, 120);
    render_.SetSize(256, 256);

    runtime::EventBus::Emit(L"pet.actor.initialized");
    initialized_ = true;
}

void PetActor::Shutdown() {
    if (!initialized_) {
        return;
    }

    UnregisterRuntimeEvents();
    runtime::EventBus::Emit(L"pet.actor.shutdown");
    initialized_ = false;
}

void PetActor::Update() {
    if (!initialized_) {
        return;
    }

    PetMindState& m = mindState_;
    const auto now = core::Timer::NowUnixSeconds();
    const auto elapsed = now - m.lastInteraction;

    if (elapsed > 60 * 10) {
        chat_.Say(L"我在这儿哦。");
        m.lastInteraction = now;
    }
}

void PetActor::OnUserInput(const std::wstring& text) {
    if (!initialized_) {
        return;
    }

    input_.Submit(text);
    chat_.Say(L"收到: " + text);
    audio_.Play(L"poke_nya");
    AppendDiary(L"[input] " + text);

    mindState_.lastInteraction = core::Timer::NowUnixSeconds();
    if (mindState_.valence < 15) {
        ++mindState_.valence;
    }

    runtime::StateManager::Set(L"pet.state", L"interactive");
}

void PetActor::Say(const std::wstring& text) {
    chat_.Say(text);
}

void PetActor::PlayAudio(const std::wstring& clipId) {
    audio_.Play(clipId);
}

bool PetActor::AppendDiary(const std::wstring& line) {
    return diary_.AppendLine(line);
}

void PetActor::SetPosition(int x, int y) {
    render_.SetPosition(x, y);
}

void PetActor::SetSize(int width, int height) {
    render_.SetSize(width, height);
}

const components::RenderState& PetActor::GetRenderState() const {
    return render_.GetState();
}

const PetMindState& PetActor::GetMindState() const {
    return mindState_;
}

bool PetActor::IsInitialized() const {
    return initialized_;
}

void PetActor::EnsureMindStateReady() {
    mindState_.lastInteraction = core::Timer::NowUnixSeconds();
    if (mindState_.valence < 1) {
        mindState_.valence = 8;
    }
    if (mindState_.arousal < 1) {
        mindState_.arousal = 8;
    }
}

void PetActor::RegisterRuntimeEvents() {
    if (onUiInputSubmit_ != 0) {
        return;
    }

    onUiInputSubmit_ = runtime::EventBus::Subscribe(L"ui.input.submit", [&](const runtime::Event& evt) {
        OnUserInput(evt.payload);
    });
}

void PetActor::UnregisterRuntimeEvents() {
    if (onUiInputSubmit_ != 0) {
        runtime::EventBus::Unsubscribe(L"ui.input.submit", onUiInputSubmit_);
        onUiInputSubmit_ = 0;
    }
}

} // namespace pet::systems::pet
