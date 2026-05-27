#include "Engine/Render/Texture.h"

#include "Core/FileSystem.h"

namespace pet::engine::render {

bool Texture::LoadFromFile(const std::wstring& path) {
    if (!core::FileSystem::Exists(path)) {
        return false;
    }
    path_ = path;
    return true;
}

const std::wstring& Texture::GetPath() const {
    return path_;
}

} // namespace pet::engine::render
