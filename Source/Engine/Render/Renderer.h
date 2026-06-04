#pragma once
#include <windows.h>

// GDI+ 启动（仅初始化，不加载图片）
bool RendererInit();
// 退出时释放 GDI+ 资源
void RendererShutdown();
// 加载宠物图片并缓存；成功返回 true
bool RendererLoadPetImage(const wchar_t* path);
// 获取已加载图片的原始宽高
int RendererGetImageWidth();
int RendererGetImageHeight();
// 使用窗口 HDC 进行常规绘制
void RendererRender(HDC hdc);
