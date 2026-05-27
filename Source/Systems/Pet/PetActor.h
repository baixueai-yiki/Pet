#pragma once

#include "Systems/Pet/Components/AudioComponent.h"
#include "Systems/Pet/Components/ChatComponent.h"
#include "Systems/Pet/Components/DiaryComponent.h"
#include "Systems/Pet/Components/InputComponent.h"
#include "Systems/Pet/Components/PetRenderComponent.h"

#include <string>

namespace pet::systems::pet {

struct PetMindState {
    long long lastInteraction = 0;
    int valence = 8;
    int arousal = 8;
};

class PetActor {
public:
    static PetActor& Get();

    void Initialize();
    void Shutdown();
    void Update();

    void OnUserInput(const std::wstring& text);
    void Say(const std::wstring& text);
    void PlayAudio(const std::wstring& clipId);
    bool AppendDiary(const std::wstring& line);

    void SetPosition(int x, int y);
    void SetSize(int width, int height);

    const components::RenderState& GetRenderState() const;
    const PetMindState& GetMindState() const;
    bool IsInitialized() const;

private:
    PetActor() = default;

    void EnsureMindStateReady();
    void RegisterRuntimeEvents();
    void UnregisterRuntimeEvents();

    bool initialized_ = false;
    int onUiInputSubmit_ = 0;
    PetMindState mindState_;

    components::AudioComponent audio_;
    components::ChatComponent chat_;
    components::DiaryComponent diary_;
    components::InputComponent input_;
    components::PetRenderComponent render_;
};

} // namespace pet::systems::pet
