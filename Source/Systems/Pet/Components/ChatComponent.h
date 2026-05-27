#pragma once

#include <string>

namespace pet::systems::pet::components {

class ChatComponent {
public:
    void Say(const std::wstring& text);
    const std::wstring& GetLastMessage() const;

private:
    std::wstring lastMessage_;
};

} // namespace pet::systems::pet::components
