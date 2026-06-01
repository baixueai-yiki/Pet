#include "AudioComponent.h"
#include "../../../Core/Path.h"
#include "../../../Runtime/EventBus.h"
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

namespace
{
    int s_playId = 0;
    int s_playAbsId = 0;
    bool s_initialized = false;

    void PlayAudioFileInternal(const std::wstring& path)
    {
        if (path.empty())
            return;
        if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
            return;

        PlaySoundW(path.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
    }

    void PlayAudioAssetInternal(const std::wstring& relative)
    {
        if (relative.empty())
            return;
        PlayAudioFileInternal(GetAssetPath(relative));
    }
}

void AudioComponent::EnsureInitialized()
{
    if (s_initialized)
        return;

    s_playId = EventSubscribe(L"audio.play", [](const Event& evt) {
        if (!evt.payload.empty())
            PlayAudioAssetInternal(evt.payload);
    });

    s_playAbsId = EventSubscribe(L"audio.play_abs", [](const Event& evt) {
        if (!evt.payload.empty())
            PlayAudioFileInternal(evt.payload);
    });

    s_initialized = true;
}

void AudioComponent::Shutdown()
{
    if (!s_initialized)
        return;

    if (s_playId != 0)
        EventUnsubscribe(L"audio.play", s_playId);
    if (s_playAbsId != 0)
        EventUnsubscribe(L"audio.play_abs", s_playAbsId);

    s_playId = 0;
    s_playAbsId = 0;
    s_initialized = false;
}

void AudioComponent::OnInit(PetActor&)
{
    EnsureInitialized();
}

void AudioComponent::OnShutdown(PetActor&)
{
    Shutdown();
}

void AudioComponent::PlayAudioAsset(const std::wstring& relative)
{
    PlayAudioAssetInternal(relative);
}

void AudioComponent::PlayAudioFile(const std::wstring& path)
{
    PlayAudioFileInternal(path);
}
