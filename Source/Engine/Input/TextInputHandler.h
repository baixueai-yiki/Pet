#pragma once

#include <string>

namespace pet::engine::input {

class TextInputHandler {
public:
    static void BeginFrame();
    static void OnChar(wchar_t ch);
    static const std::wstring& GetComposedText();
};

} // namespace pet::engine::input
