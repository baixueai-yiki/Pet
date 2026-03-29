#pragma once
#include <windows.h>
#include <string>

// 弹出可输入文本的窗口
void ChatShowInput(HWND hwndParent);
void ChatShowButtonInput(HWND hwndParent, const std::wstring& key1, const std::wstring& key2);
// 宠物移动时调用，用来同步输入栏位置
void ChatUpdateInputPosition();
// 宠物移动时调用，用来同步回复气泡位置
void ChatUpdateTalkPosition();
// 宠物移动时调用，用来同步按钮栏位置
void ChatUpdateButtonPosition();
// 宠物移动时调用，用来同步任务列表窗口位置
void ChatUpdateTaskListPosition();
// 记录一次互动时间并写入 state.json
void ChatRecordInteraction();
// 每分钟检查是否需要触发闲聊
void ChatTickIdleCheck(HWND hwnd);
// 获取当前聊天状态快照（用于程序退出写入）
void ChatGetStateSnapshot(long long& lastInteraction, int& valence, int& arousal);
// 启动时注册聊天状态提供者与事件订阅
void ChatInit(HWND hwnd);
// 根据输入内容执行相应的对话逻辑
void ChatHandleInput(HWND hwnd, const std::wstring& input);
