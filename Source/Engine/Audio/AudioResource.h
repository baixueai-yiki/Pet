#pragma once

#include <string>

namespace pet::engine::audio {

class AudioResource {
public:
    bool Load(const std::wstring& path);
    const std::wstring& Path() const;

private:
    std::wstring path_;
};

} // namespace pet::engine::audio
