#include "Core/FileSystem.h"

#include <filesystem>
#include <fstream>
#include <iterator>

namespace pet::core {

bool FileSystem::Exists(const std::wstring& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

bool FileSystem::EnsureDirectory(const std::wstring& dirPath) {
    if (dirPath.empty()) return false;
    std::error_code ec;
    if (std::filesystem::exists(dirPath, ec)) return true;
    return std::filesystem::create_directories(dirPath, ec);
}

bool FileSystem::ReadBinary(const std::wstring& path, std::string& data) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return false;
    data.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return true;
}

bool FileSystem::WriteBinary(const std::wstring& path, const std::string& data, bool append) {
    const std::filesystem::path p(path);
    if (p.has_parent_path()) EnsureDirectory(p.parent_path().wstring());
    std::ofstream out(path, std::ios::binary | (append ? std::ios::app : std::ios::trunc));
    if (!out.is_open()) return false;
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    return true;
}

bool FileSystem::IsFileEmpty(const std::wstring& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return true;
    return in.tellg() <= 0;
}

} // namespace pet::core
