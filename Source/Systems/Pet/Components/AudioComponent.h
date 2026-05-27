#pragma once

#include <string>

namespace pet::systems::pet::components {

class AudioComponent {
public:
    void Play(const std::wstring& clipId);
    const std::wstring& GetLastClipId() const;

private:
    std::wstring lastClipId_;
};

} // namespace pet::systems::pet::components
