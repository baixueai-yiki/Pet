#pragma once

#include "../PetActor.h"
#include <string>

// 处理用户输入内容
class InputComponent : public PetComponent
{
public:
    void OnInit(PetActor& actor) override;
    void OnShutdown(PetActor& actor) override;

    // 文字输入处理
    static void HandleInput(HWND hwnd, const std::wstring& input);
    // 按钮选项处理
    static void HandleButtonInput(HWND buttonWnd, const std::wstring& key);
};
