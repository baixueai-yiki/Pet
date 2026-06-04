#include "AudioComponent.h"
#include "../../../Core/Path.h"
#include "../../../Runtime/EventBus.h"
#include <windows.h>
#include <mmsystem.h>
#include <string>
#include <vector>

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

        // wav 用 PlaySound，ogg/mp3 等走 MCI
        if (path.size() >= 4 && _wcsicmp(path.c_str() + path.size() - 4, L".wav") == 0)
        {
            PlaySoundW(path.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
        }
        else
        {
            static int s_mciIdx = 0;
            wchar_t alias[32];
            swprintf_s(alias, 32, L"pet%d", ++s_mciIdx);
            mciSendStringW((L"close " + std::wstring(alias)).c_str(), nullptr, 0, nullptr);
            std::wstring cmd = L"open \"" + path + L"\" alias " + alias;
            mciSendStringW(cmd.c_str(), nullptr, 0, nullptr);
            mciSendStringW((L"play " + std::wstring(alias)).c_str(), nullptr, 0, nullptr);
        }
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

void AudioComponent::PlayRandomStartupAudio()
{
    std::vector<std::wstring> runFiles;
    std::wstring pattern = GetAssetPath(L"audio") + L"\\run_*.*";
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                runFiles.push_back(L"audio\\" + std::wstring(fd.cFileName));
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
    if (!runFiles.empty())
        PlayAudioAssetInternal(runFiles[rand() % (int)runFiles.size()]);
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
    PlayRandomStartupAudio();
}

void AudioComponent::OnShutdown(PetActor&)
{
    Shutdown();
}

void AudioComponent::PlayAudioAuto(const std::wstring& baseName)
{
    static const wchar_t* kExts[] = { L".wav", L".mp3", L".ogg" };
    for (const auto* ext : kExts)
    {
        std::wstring path = baseName + ext;
        if (GetFileAttributesW(GetAssetPath(path).c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            PlayAudioAsset(path);
            return;
        }
    }
}

void AudioComponent::PlayAudioAsset(const std::wstring& relative)
{
    PlayAudioAssetInternal(relative);
}

void AudioComponent::PlayAudioFile(const std::wstring& path)
{
    PlayAudioFileInternal(path);
}
