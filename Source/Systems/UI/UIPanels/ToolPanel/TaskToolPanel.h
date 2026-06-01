#pragma once

#include "../../UIActor.h"
#include <windows.h>
#include <string>

class TaskToolPanel
{
public:
    static void Setup(UIActor& actor);

    static void Toggle(HWND hwndParent);
    static bool IsVisible();
    static void Hide();

    // 供 SettingToolPanel 嵌入任务列表
    static int  GetTaskCount();
    static std::wstring GetTaskTitle(int index);
    static HICON GetTaskIcon(int index);
    static DWORD GetTaskPID(int index);
    static void KillTask(int index);
    static void RefreshTaskList();
};
