#pragma once

namespace pet::systems::ui::components {

struct DragState {
    bool dragging = false;
    int startX = 0;
    int startY = 0;
    int currentX = 0;
    int currentY = 0;
};

class DragComponent {
public:
    void Begin(int x, int y);
    void MoveTo(int x, int y);
    void End();
    const DragState& GetState() const;

private:
    DragState state_;
};

} // namespace pet::systems::ui::components
