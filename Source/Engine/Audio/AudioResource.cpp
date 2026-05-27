#include "Engine/Audio/AudioResource.h"

#include "Core/FileSystem.h"

namespace pet::engine::audio {

bool AudioResource::Load(const std::wstring& path) {
    if (!core::FileSystem::Exists(path)) {
        return false;
    }
    path_ = path;
    return true;
}

const std::wstring& AudioResource::Path() const {
    return path_;
}

} // namespace pet::engine::audio
