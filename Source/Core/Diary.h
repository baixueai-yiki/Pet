#pragma once
#include <string>

// 程序启动时：清空 diary_writing.txt
void OnProgramStart();
// 程序退出时：写入 state.json 并追加到 diary.txt
void OnProgramExit();
// 向 diary_writing.txt 追加一行
void DiaryAppendWritingLine(const std::wstring& line);

// 由系统层提供最新互动/愉悦度/兴奋度的快照。
using StateSnapshotProvider = void (*)(long long&, int&, int&);
void DiarySetStateSnapshotProvider(StateSnapshotProvider provider);

// 注册日记相关事件订阅
void DiaryInit();
