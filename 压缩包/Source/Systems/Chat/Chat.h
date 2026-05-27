#pragma once
#include <windows.h>
#include <string>

// 传统的函数式入口，便于保留现有调用点。
void ChatShowInput(HWND hwndParent);
void ChatShowButtonInput(HWND hwndParent, const std::wstring& key1, const std::wstring& key2);
void ChatUpdateInputPosition();
void ChatUpdateTalkPosition();
void ChatUpdateButtonPosition();
void ChatUpdateTaskListPosition();
void ChatRecordInteraction();
void ChatTickIdleCheck(HWND hwnd);
void ChatGetStateSnapshot(long long& lastInteraction, int& valence, int& arousal);
void ChatInit(HWND hwnd);
void ChatHandleInput(HWND hwnd, const std::wstring& input);

// 系统式对象入口，后续可以逐步把 Chat 做成真正的对象系统。
class ChatSystem
{
public:
    static ChatSystem& Get();

    void Init(HWND hwnd);
    void ShowInput(HWND hwndParent);
    void ShowButtonInput(HWND hwndParent, const std::wstring& key1, const std::wstring& key2);
    void UpdateInputPosition();
    void UpdateTalkPosition();
    void UpdateButtonPosition();
    void UpdateTaskListPosition();
    void RecordInteraction();
    void TickIdleCheck(HWND hwnd);
    void GetStateSnapshot(long long& lastInteraction, int& valence, int& arousal);
    void HandleInput(HWND hwnd, const std::wstring& input);

private:
    ChatSystem() = default;
};
