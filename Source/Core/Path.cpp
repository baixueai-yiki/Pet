#include "Path.h"
#include <windows.h>

std::wstring GetExeDir()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);

    std::wstring fullPath(path);
    size_t pos = fullPath.find_last_of(L"\\/");
    return fullPath.substr(0, pos);
}

std::wstring GetContentPath(const std::wstring& relativePath)
{
    return GetExeDir() + L"\\" + relativePath;
}
