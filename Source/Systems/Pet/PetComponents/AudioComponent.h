#pragma once

#include "../PetActor.h"
#include <string>

class AudioComponent : public PetComponent
{
public:
    void OnInit(PetActor& actor) override;
    void OnShutdown(PetActor& actor) override;

    static void EnsureInitialized();
    static void Shutdown();
    static void PlayRandomStartupAudio();
    static void PlayAudioAsset(const std::wstring& relative);
    static void PlayAudioFile(const std::wstring& path);
    // baseName 不含扩展名，自动尝试 wav/ogg/mp3
    static void PlayAudioAuto(const std::wstring& baseName);
};
