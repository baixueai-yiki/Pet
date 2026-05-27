#pragma once

#include <string>

namespace pet::systems::pet::components {

class InputComponent {
public:
    void Submit(const std::wstring& inputText);
    const std::wstring& GetLastInput() const;

private:
    std::wstring lastInput_;
};

} // namespace pet::systems::pet::components
