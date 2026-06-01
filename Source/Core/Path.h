#pragma once
#include <string>

// 获取当前可执行文件所在目录
std::wstring GetExeDir();

// 获取 assets/images 下的资源路径
std::wstring GetImagePath(const std::wstring& file);

// 获取 assets 下任意文件的路径（可用于配置、对话等）
std::wstring GetAssetPath(const std::wstring& relative);

// 获取 config 下的资源路径
std::wstring GetConfigPath(const std::wstring& file);
