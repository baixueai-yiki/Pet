#pragma once
#include <windows.h>

void HandleDragMouseDown(HWND hwnd, LPARAM lParam);
void HandleDragMouseMove(HWND hwnd, LPARAM lParam);
void HandleDragMouseUp();

bool IsInsideCharacter(int x, int y);