#pragma once

#include <string>

namespace pet::systems::pet::components {

class DiaryComponent {
public:
    bool AppendLine(const std::wstring& line);
};

} // namespace pet::systems::pet::components
