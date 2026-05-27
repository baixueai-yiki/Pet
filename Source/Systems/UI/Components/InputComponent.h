#pragma once

#include <string>

namespace pet::systems::ui::components {

class InputComponent {
public:
    void SetText(const std::wstring& text);
    const std::wstring& GetText() const;
    void Submit();

private:
    std::wstring text_;
};

} // namespace pet::systems::ui::components
