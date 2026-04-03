#pragma once

#include "../UIActor.h"
#include <string>

class InputComponent : public UIComponent
{
public:
    void OnInit(UIActor& actor) override;
    void OnShutdown(UIActor& actor) override;
    void SubmitText(const std::wstring& text);

private:
    UIActor* m_actor = nullptr;
};
