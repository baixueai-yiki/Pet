#include "UIActor.h"

UIActor::UIActor(const std::wstring& name)
    : m_name(name)
{
}

UIActor::~UIActor()
{
    Shutdown();
}

void UIActor::Initialize(HWND parentHwnd)
{
    if (m_parentHwnd == parentHwnd)
        return;

    m_parentHwnd = parentHwnd;
    for (auto& component : m_components)
        component->OnInit(*this);
}

void UIActor::Shutdown()
{
    for (auto& component : m_components)
        component->OnShutdown(*this);
    m_components.clear();
}

void UIActor::AddComponent(std::unique_ptr<UIComponent> component)
{
    if (component)
        m_components.push_back(std::move(component));
}

void UIActor::Show()
{
    m_visible = true;
}

void UIActor::Hide()
{
    m_visible = false;
}

void UIActor::NotifyMouseClick(int x, int y)
{
    if (!m_visible)
        return;
    for (auto& component : m_components)
        component->OnMouseClick(*this, x, y);
}

void UIActor::NotifyMouseWheel(int delta)
{
    if (!m_visible)
        return;
    for (auto& component : m_components)
        component->OnMouseWheel(*this, delta);
}

