#pragma once
#include <windows.h>
#include <string>

void ChatShowInput(HWND hwndParent);
void ChatHandleInput(HWND hwnd, const std::wstring& input);
