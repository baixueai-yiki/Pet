#include <windows.h>   // Windows 基础API（窗口、消息循环等）
#include <gdiplus.h>   // GDI+ 图形库（用来画图片）
#include "Func/func_chat.h" // 你自己的聊天窗口函数
#pragma comment(lib,"gdiplus.lib") // 告诉编译器链接 GDI+ 库

using namespace Gdiplus;


// ================= 全局数据 =================

// 窗口左上角初始位置（屏幕坐标）
int window_x = 100;
int window_y = 100;

// 窗口大小（后面会根据图片真实尺寸覆盖）
UINT window_width = 200;
UINT window_height = 300;

// 主窗口句柄（相当于“窗口身份证”，后面操作窗口都要用它）
HWND hwndMain = nullptr;

// GDI+ 启动句柄（用于初始化/关闭图形系统）
ULONG_PTR gdiplusToken;

// 记录最近6次点击时间（用于检测“特殊点击节奏”）
DWORD clickTimes[6] = {0};

// 桌宠图片（全局保存，避免每次绘制都重新加载）
Bitmap* g_character = nullptr;


// ================= 窗口过程函数 =================
// 👉 Windows发生任何事件（点击、绘制、关闭等）都会调用这个函数
// hwnd  = 当前窗口
// msg   = 发生了什么事件
// wParam/lParam = 事件附带的数据
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // 根据不同事件类型进行处理
    switch (msg)
    {
    case WM_CREATE:
    {
        // 👉 窗口刚创建时触发
        // 这里不再加载图片（因为我们在主函数里提前加载了）
        break;
    }

    case WM_PAINT:
    {
        // 👉 窗口需要重绘时触发（比如第一次显示 / 被遮挡后恢复）

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps); 
        // 开始绘制，获取“画布”（设备上下文 HDC）

        // 确保图片存在且加载成功
        if (g_character && g_character->GetLastStatus() == Ok)
        {
            Graphics graphics(hdc); // GDI+绘图对象（比HDC更高级）

            // 在窗口左上角(0,0)绘制整张图片
            graphics.DrawImage(g_character, 0, 0);
        }

        EndPaint(hwnd, &ps); // 结束绘制（必须调用）
        break;
    }

    case WM_RBUTTONDOWN:
    {
        // 👉 鼠标右键点击

        DWORD now = GetTickCount(); // 当前时间（毫秒）

        // 把旧点击时间往前挪（类似队列）
        for (int i = 0; i < 5; i++)
            clickTimes[i] = clickTimes[i + 1];

        // 记录最新点击
        clickTimes[5] = now;

        // 定义“节奏判定阈值”
        const int slowMax = 1000; // 慢：1秒内
        const int fastMax = 300;  // 快：0.3秒内

        // 判断前3次点击是否“慢节奏”
        bool slowTriple = (clickTimes[3] - clickTimes[0] <= slowMax);

        // 判断后3次点击是否“快节奏”
        bool fastTriple = (clickTimes[5] - clickTimes[3] <= fastMax);

        // 如果同时满足（慢三下 + 快三下）
        if (slowTriple && fastTriple)
        {
            // 清空记录（防止重复触发）
            for (int i = 0; i < 6; i++)
                clickTimes[i] = 0;

            // 打开聊天窗口（你自己实现的功能）
            ShowChatInput(hwnd);
        }
        break;
    }

    case WM_DESTROY:
    {
        // 👉 窗口关闭时触发

        // 释放图片内存（防止内存泄漏）
        if (g_character)
        {
            delete g_character;
            g_character = nullptr;
        }

        // 通知系统：程序可以退出了（结束主循环）
        PostQuitMessage(0);
        break;
    }

    default:
        // 👉 所有没处理的消息交给系统默认处理（必须保留）
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}


// ================= 程序入口 =================
// 👉 Windows程序的起点（不是 main）
// hInstance = 当前程序实例（Windows用来区分程序）
// 后面几个参数基本用不到（历史遗留）
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    // ================= 初始化图形系统 =================
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);


    // ================= 提前加载图片 =================
    // 👉 因为我们要用图片尺寸来决定窗口大小
    g_character = Bitmap::FromFile(L"character.png");

    // 检查是否加载成功
    if (!g_character || g_character->GetLastStatus() != Ok)
    {
        MessageBoxW(NULL, L"图片加载失败", L"错误", MB_OK);
        return 0; // 直接退出程序
    }

    // 获取图片真实尺寸
    window_width = g_character->GetWidth();
    window_height = g_character->GetHeight();


    // ================= 注册窗口类型 =================
    // 👉 告诉Windows“我要创建一种什么样的窗口”
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;   // 指定“窗口发生事件时调用谁”
    wc.hInstance = hInstance;   // 当前程序
    wc.lpszClassName = L"CharacterWindow"; // 窗口类型名字

    RegisterClassW(&wc);


    // ================= 创建窗口 =================
    hwndMain = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        // 扩展样式：
        // TOPMOST = 永远在最上层
        // LAYERED = 支持透明
        // TOOLWINDOW = 不显示在任务栏

        wc.lpszClassName, // 窗口类型
        L"桌宠",           // 窗口标题（一般看不到，因为无边框）
        WS_POPUP,         // 无边框窗口

        window_x, window_y,             // 位置
        window_width, window_height,    // 大小（来自图片）

        NULL, NULL, hInstance, NULL
    );


    // ================= 设置透明度 =================
    SetLayeredWindowAttributes(hwndMain, 0, 255, LWA_ALPHA);
    // 255 = 完全不透明（后面可以做渐变/透明桌宠）


    // ================= 显示窗口 =================
    ShowWindow(hwndMain, SW_SHOW);


    // ================= 主消息循环 =================
    // 👉 程序“活着”的核心
    // 👉 不断从系统取消息 → 分发给 WndProc
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg); // 处理键盘消息（可选）
        DispatchMessage(&msg);  // 交给 WndProc 处理
    }


    // ================= 程序结束清理 =================
    GdiplusShutdown(gdiplusToken);

    return 0;
}