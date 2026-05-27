#pragma once

#include "Engine/Audio/AudioResource.h"

namespace pet::engine::audio {

class AudioPlayer {
public:
    static bool PlayOnce(const AudioResource& res);
    static bool PlayLoop(const AudioResource& res);
    static void StopAll();
};

} // namespace pet::engine::audio
