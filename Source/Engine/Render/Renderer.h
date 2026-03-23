#pragma once
#include <windows.h>

bool RendererInit();
void RendererShutdown();
void RendererRender(HDC hdc);
