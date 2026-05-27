#pragma once

namespace pet::systems::pet::components {

struct RenderState {
    int x = 0;
    int y = 0;
    int width = 256;
    int height = 256;
};

class PetRenderComponent {
public:
    void SetPosition(int x, int y);
    void SetSize(int width, int height);
    const RenderState& GetState() const;

private:
    RenderState state_;
};

} // namespace pet::systems::pet::components
