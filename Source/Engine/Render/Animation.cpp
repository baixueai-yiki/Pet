#include "Engine/Render/Animation.h"

namespace pet::engine::render {

void Animation::Start(int frameCount, std::int64_t frameMs) {
    frameCount_ = frameCount > 0 ? frameCount : 1;
    frameMs_ = frameMs > 0 ? frameMs : 100;
    currentFrame_ = 0;
    lastTickMs_ = 0;
    running_ = true;
}

void Animation::Stop() {
    running_ = false;
}

void Animation::Tick(std::int64_t nowMs) {
    if (!running_ || frameCount_ <= 1) return;
    if (lastTickMs_ == 0) {
        lastTickMs_ = nowMs;
        return;
    }
    if (nowMs - lastTickMs_ >= frameMs_) {
        currentFrame_ = (currentFrame_ + 1) % frameCount_;
        lastTickMs_ = nowMs;
    }
}

int Animation::CurrentFrame() const { return currentFrame_; }
bool Animation::IsRunning() const { return running_; }

} // namespace pet::engine::render
