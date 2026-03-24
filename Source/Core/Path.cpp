#include "Path.h"
#include <windows.h>

std::wstring GetExeDir()
{
    wchar_t path[MAX_PATH];
    // 获取当前模块（exe）路径
    GetModuleFileNameW(nullptr, path, MAX_PATH);

    std::wstring fullPath(path);
    // 取最后的目录分隔符前的部分作为目录
    size_t pos = fullPath.find_last_of(L"\\/");
    return fullPath.substr(0, pos);
}

std::wstring GetContentPath(const std::wstring& relativePath)
{
    // 简单拼接目录和相对路径构成资源路径
    return GetExeDir() + L"\\" + relativePath;
}
