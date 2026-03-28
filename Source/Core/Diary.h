#pragma once
#include <string>

// Program start: clear diary_writing.txt
void OnProgramStart();
// Program exit: write state.json and append to diary.txt
void OnProgramExit();
// Append one line to diary_writing.txt
void DiaryAppendWritingLine(const std::wstring& line);
