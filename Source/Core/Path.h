#pragma once
#include <string>

// 获取当前可执行文件所在目录
std::wstring GetExeDir();
// 将相对路径拼接成资源的绝对路径
std::wstring GetContentPath(const std::wstring& relativePath);
