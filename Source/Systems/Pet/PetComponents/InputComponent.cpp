#include "InputComponent.h"
#include "AudioComponent.h"
#include "ChatComponent.h"
#include "../../../Core/Path.h"
#include "../../../Core/TextFile.h"
#include "../../UI/UIPanels/ToolPanel/SettingToolPanel.h"
#include "../../UI/UIPanels/ToolPanel/TaskToolPanel.h"
#include <windows.h>

static std::wstring Trim(const std::wstring& text)
{
    const wchar_t* ws = L" \t\r\n";
    size_t start = text.find_first_not_of(ws);
    size_t end = text.find_last_not_of(ws);
    if (start == std::wstring::npos)
        return L"";
    return text.substr(start, end - start + 1);
}

static bool IsTaskListTrigger(const std::wstring& text)
{
    return text == L"任务管理器";
}

void InputComponent::OnInit(PetActor& actor)
{
    (void)actor;
}

void InputComponent::OnShutdown(PetActor& actor)
{
    (void)actor;
}

void InputComponent::HandleInput(HWND hwnd, const std::wstring& input)
{
    ChatRecordInteraction();

    const std::wstring normalized = Trim(input);
    if (Setting::TryApplyInlineValue(normalized))
    {
        InvalidateRect(hwnd, nullptr, TRUE);
        return;
    }

    if (IsTaskListTrigger(normalized))
    {
        TaskToolPanel::Toggle(hwnd);
        return;
    }

    if (normalized == L"设置")
    {
        Setting::ToggleOverlay();
        InvalidateRect(hwnd, nullptr, TRUE);
        return;
    }

    // 按钮的 label 不作为文本输入匹配
    {
        auto* btnCheck = ChatLookupButton(normalized);
        if (btnCheck && *btnCheck == normalized)
            { ChatTalk(hwnd, L"……"); return; }
    }

    auto* reply = ChatLookupDialog(normalized);
    if (reply)
    {
        size_t actPos = reply->find(L'\x01');
        std::wstring text = (actPos == std::wstring::npos) ? *reply : reply->substr(0, actPos);
        std::wstring action = (actPos == std::wstring::npos) ? L"" : reply->substr(actPos + 1);

        if (!text.empty())
            ChatTalk(hwnd, text.c_str());

        if (action == L"quit")
            CreateThread(nullptr, 0, [](LPVOID c)->DWORD { Sleep(5000); PostMessageW((HWND)c, WM_CLOSE, 0, 0); return 0; },
                         (LPVOID)hwnd, 0, nullptr);
        else if (action == L"audio")
            AudioComponent::PlayAudioAuto(L"audio\\" + normalized);
        else if (!action.empty())
            AudioComponent::PlayAudioAsset(L"audio\\" + action);

        // 检查是否有 label1/label2 → 弹出选项按钮
        {
            std::wstring matchKey = ChatLastMatchedKey();
            if (matchKey.empty()) matchKey = normalized;
            std::wstring l1 = ChatGetButtonLabel(matchKey, L"");
            auto* l2 = ChatLookupButton(matchKey);
            if (!l1.empty() && l2 && l1.find(L"__btn__") == std::wstring::npos)
                ChatShowButtonInput(hwnd, l1, *l2);
        }
        return;
    }
    if (reply)
    {
        ChatTalk(hwnd, reply->c_str());
        // 检查是否有 label1/label2 → 弹出选项按钮
        std::wstring mk2 = ChatLastMatchedKey(); if (mk2.empty()) mk2 = normalized;
        std::wstring l1 = ChatGetButtonLabel(mk2, L"");
        auto* l2 = ChatLookupButton(mk2);
        if (!l1.empty() && l2 && l1.find(L"__btn__") == std::wstring::npos)
            ChatShowButtonInput(hwnd, l1, *l2);
        return;
    }

    auto* def = ChatLookupDialog(L"默认");
    ChatTalk(hwnd, def ? def->c_str() : L"……我不太明白你在说什么。");
}

void InputComponent::HandleButtonInput(HWND buttonWnd, const std::wstring& key)
{
    ChatRecordInteraction();

    auto* reply = ChatLookupDialog(key);
    if (reply)
    {
        size_t ap = reply->find(L'\x01');
        std::wstring text = (ap == std::wstring::npos) ? *reply : reply->substr(0, ap);
        std::wstring action = (ap == std::wstring::npos) ? L"" : reply->substr(ap + 1);
        if (!text.empty())
            ChatTalk(GetParent(buttonWnd), text.c_str());
        if (action == L"quit")
            CreateThread(nullptr, 0, [](LPVOID c)->DWORD { Sleep(5000); PostMessageW((HWND)c, WM_CLOSE, 0, 0); return 0; },
                         (LPVOID)GetParent(buttonWnd), 0, nullptr);
        else if (!action.empty())
            AudioComponent::PlayAudioAsset(L"audio\\" + action);
    }
    else
    {
        reply = ChatLookupButton(key);
        if (reply)
            ChatTalk(GetParent(buttonWnd), reply->c_str());
    }

    DestroyWindow(buttonWnd);
}
