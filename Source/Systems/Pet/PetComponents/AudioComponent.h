#pragma once

#include "../Pet.h"
#include <string>

class AudioComponent : public PetComponent
{
public:
    void OnInit(PetActor& actor) override;
    void OnShutdown(PetActor& actor) override;

    static void EnsureInitialized();
    static void Shutdown();
    static void PlayAudioAsset(const std::wstring& relative);
    static void PlayAudioFile(const std::wstring& path);
};
