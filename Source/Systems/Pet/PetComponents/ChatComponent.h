#pragma once
#include <windows.h>
#include <string>
#include <map>

// 输出（durationMs=0 使用默认 3 秒，-1 不自动消失）
void ChatTalk(HWND hwnd, const wchar_t* text, int durationMs = 0);
void ChatShowInput(HWND hwndParent);
void ChatShowButtonInput(HWND hwndParent, const std::wstring& key1, const std::wstring& key2);
std::wstring ChatGetButtonLabel(const std::wstring& key, const std::wstring& fallback);

// 对话数据查询（供 InputComponent 使用）
void ChatEnsureDialogLoaded();
const std::wstring* ChatLookupDialog(const std::wstring& input);
std::wstring ChatLastMatchedKey(); // 模糊匹配时返回匹配到的键名
const std::wstring* ChatLookupButton(const std::wstring& key);

// 状态 & 按时间触发语句
void ChatRecordInteraction();
bool ChatTryIdleLine(HWND hwnd);  // 根据当前小时，从 chat_idle.txt 随机选取并说出
bool ChatTrySleepLine(HWND hwnd); // 强制取 sleep 语句（睡觉前用）
bool GetIdleTextByKey(const std::wstring& key, std::wstring& out, std::wstring& keyUsed);
bool LoadIdleMap(std::map<std::wstring, std::wstring>& out);
void ChatGetStateSnapshot(long long& lastInteraction, int& valence, int& arousal);
void ChatInit(HWND hwnd);

// 位置更新（未实现，预留）
void ChatUpdateInputPosition();
void ChatUpdateTalkPosition();
void ChatUpdateButtonPosition();
void ChatUpdateTaskListPosition();

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
