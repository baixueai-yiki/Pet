#pragma once

namespace pet::systems::ui::components {

class ScrollComponent {
public:
    void AddDelta(int delta);
    int GetOffset() const;
    void SetOffset(int offset);

private:
    int offset_ = 0;
};

} // namespace pet::systems::ui::components
