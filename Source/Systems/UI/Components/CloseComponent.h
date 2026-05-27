#pragma once

namespace pet::systems::ui::components {

class CloseComponent {
public:
    void RequestCloseAll();
    bool IsRequested() const;
    void Reset();

private:
    bool requested_ = false;
};

} // namespace pet::systems::ui::components
