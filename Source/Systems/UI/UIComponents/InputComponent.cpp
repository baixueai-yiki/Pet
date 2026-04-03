#include "InputComponent.h"

void InputComponent::OnInit(UIActor& actor)
{
    m_actor = &actor;
}

void InputComponent::OnShutdown(UIActor& actor)
{
    if (m_actor == &actor)
        m_actor = nullptr;
}

void InputComponent::SubmitText(const std::wstring& text)
{
    if (!m_actor || !m_actor->IsVisible())
        return;
    (void)text;
}
