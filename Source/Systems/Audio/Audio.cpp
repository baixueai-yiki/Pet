#include "Audio.h"
#include "Runtime/EventBus.h"
#include "../../Core/Path.h"
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

void PlayAudioFile(const std::wstring& path)
{
    if (path.empty())
        return;
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
        return;
    PlaySoundW(path.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
}

void PlayAudioAsset(const std::wstring& relative)
{
    if (relative.empty())
        return;
    const std::wstring path = GetAssetPath(relative);
    PlayAudioFile(path);
}

void AudioInit()
{
    EventSubscribe(L"audio.play", [](const Event& evt) {
        if (!evt.payload.empty())
            PlayAudioAsset(evt.payload);
    });
    EventSubscribe(L"audio.play_abs", [](const Event& evt) {
        if (!evt.payload.empty())
            PlayAudioFile(evt.payload);
    });
}
