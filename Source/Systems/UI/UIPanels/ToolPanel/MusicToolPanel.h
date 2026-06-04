#pragma once

#include "../../UIActor.h"
#include <windows.h>
#include <string>
#include <vector>

class MusicToolPanel
{
public:
    static void Setup(UIActor& actor);

    // 音乐文件列表
    static int  GetMusicCount();
    static std::wstring GetMusicName(int index);
    static void PlayMusic(int index);   // 双击文件 → 开始播放
    static void RefreshMusicList();

    // 播放器条
    static bool IsPlayerVisible();
    static void HidePlayer();
    static void TogglePlayPause();
    static void SeekTo(int posMs);
    static int  GetCurrentPosition();
    static int  GetTrackLength();
    static void UpdatePosition();
};
