#pragma once
#include <string>

// 播放 assets 目录下的音频文件（不存在时静默忽略）
void PlayAudioAsset(const std::wstring& relative);

// 播放绝对路径音频文件（不存在时静默忽略）
void PlayAudioFile(const std::wstring& path);

// 注册音频相关事件订阅
void AudioInit();
