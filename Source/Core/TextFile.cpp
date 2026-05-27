#include "Core/TextFile.h"

#include <sstream>
#include <windows.h>

namespace pet::core {
namespace {

constexpr unsigned char kUtf8Bom[] = {0xEF, 0xBB, 0xBF};

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string out(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &out[0], len, nullptr, nullptr);
    return out;
}

bool Utf8ToWide(const std::string& data, std::wstring& out) {
    if (data.empty()) { out.clear(); return true; }
    int len = MultiByteToWideChar(CP_UTF8, 0, data.data(), static_cast<int>(data.size()), nullptr, 0);
    if (len <= 0) return false;
    out.assign(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, data.data(), static_cast<int>(data.size()), &out[0], len);
    return true;
}

}

bool TextFile::ReadText(const std::wstring& path, std::wstring& out) {
    std::string data;
    if (!FileSystem::ReadBinary(path, data)) return false;
    if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == kUtf8Bom[0] && static_cast<unsigned char>(data[1]) == kUtf8Bom[1] && static_cast<unsigned char>(data[2]) == kUtf8Bom[2]) {
        data.erase(0, 3);
    }
    return Utf8ToWide(data, out);
}

bool TextFile::ReadLines(const std::wstring& path, std::vector<std::wstring>& lines) {
    std::wstring text;
    if (!ReadText(path, text)) return false;
    lines.clear();
    std::wistringstream iss(text);
    std::wstring line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        lines.push_back(line);
    }
    return true;
}

bool TextFile::WriteText(const std::wstring& path, const std::wstring& content, bool append, bool ensureBom) {
    std::string data;
    if (ensureBom && (!append || FileIsEmpty(path))) {
        data.append(reinterpret_cast<const char*>(kUtf8Bom), sizeof(kUtf8Bom));
    }
    data += WideToUtf8(content);
    return FileSystem::WriteBinary(path, data, append);
}

bool TextFile::FileIsEmpty(const std::wstring& path) {
    return FileSystem::IsFileEmpty(path);
}

} // namespace pet::core
