#pragma once

#include <windows.h>

namespace pet::systems::pet::components {
struct RenderState;
}

namespace pet::engine::render {

class Renderer {
public:
    Renderer() = delete;

    static bool Initialize();
    static void Shutdown();
    static void Render(HDC hdc, const pet::systems::pet::components::RenderState& state);
    static bool HasImage();

private:
    static bool LoadImageFromCandidates();
};

} // namespace pet::engine::render
