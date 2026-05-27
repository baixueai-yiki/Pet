#include "Engine/Audio/AudioPlayer.h"

#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

namespace pet::engine::audio {

bool AudioPlayer::PlayOnce(const AudioResource& res) {
    if (res.Path().empty()) return false;
    return PlaySoundW(res.Path().c_str(), nullptr, SND_FILENAME | SND_ASYNC) == TRUE;
}

bool AudioPlayer::PlayLoop(const AudioResource& res) {
    if (res.Path().empty()) return false;
    return PlaySoundW(res.Path().c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_LOOP) == TRUE;
}

void AudioPlayer::StopAll() {
    PlaySoundW(nullptr, nullptr, 0);
}

} // namespace pet::engine::audio
