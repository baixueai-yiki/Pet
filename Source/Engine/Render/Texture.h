#pragma once

#include <string>

namespace pet::engine::render {

class Texture {
public:
    bool LoadFromFile(const std::wstring& path);
    const std::wstring& GetPath() const;

private:
    std::wstring path_;
};

} // namespace pet::engine::render
