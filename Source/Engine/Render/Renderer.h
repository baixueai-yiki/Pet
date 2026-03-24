#pragma once
#include <windows.h>

// 初始化渲染器与资源（包括 GDI+ 和人物图像）
bool RendererInit();
// 确保退出时释放 GDI+ 资源并恢复状态
void RendererShutdown();
// 将宠物贴图绘制到传入的设备上下文中
void RendererRender(HDC hdc);
