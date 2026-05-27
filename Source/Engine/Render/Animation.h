#pragma once

#include <cstdint>

namespace pet::engine::render {

class Animation {
public:
    void Start(int frameCount, std::int64_t frameMs);
    void Stop();
    void Tick(std::int64_t nowMs);

    int CurrentFrame() const;
    bool IsRunning() const;

private:
    int frameCount_ = 0;
    std::int64_t frameMs_ = 0;
    std::int64_t lastTickMs_ = 0;
    int currentFrame_ = 0;
    bool running_ = false;
};

} // namespace pet::engine::render
