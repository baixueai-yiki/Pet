#include "Engine/Input/TextInputHandler.h"

namespace pet::engine::input {
namespace {
std::wstring g_text;
}

void TextInputHandler::BeginFrame() {
    g_text.clear();
}

void TextInputHandler::OnChar(wchar_t ch) {
    g_text.push_back(ch);
}

const std::wstring& TextInputHandler::GetComposedText() {
    return g_text;
}

} // namespace pet::engine::input
