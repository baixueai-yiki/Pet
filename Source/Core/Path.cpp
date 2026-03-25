#include "Path.h"
#include <windows.h>

// 返回当前 exe 所在目录，其他路径以此作为根
std::wstring GetExeDir()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring fullPath(path);
    size_t pos = fullPath.find_last_of(L"\\/");
    return (pos == std::wstring::npos) ? fullPath : fullPath.substr(0, pos);
}

// 拼接 assets 目录下的相对路径
std::wstring GetAssetPath(const std::wstring& relative)
{
    return GetExeDir() + L"\\assets\\" + relative;
}

// 专门获取 assets/images 中的资源
std::wstring GetImagePath(const std::wstring& file)
{
    return GetAssetPath(L"images\\" + file);
}

// 获取 config 目录下文件的绝对路径
std::wstring GetConfigPath(const std::wstring& file)
{
    return GetExeDir() + L"\\config\\" + file;
}
