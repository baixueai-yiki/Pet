#include "UIActor.h"
#include "UIComponents/CloseComponent.h"
#include "UIComponents/ScrollComponent.h"
#include "UIComponents/DragComponent.h"
#include "UIComponents/InputComponent.h"
#include "UIPanels/ToolPanel/TaskToolPanel.h"
#include <memory>

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

// 静态单例方法（从 UIManager 迁移）
std::unique_ptr<UIActor> UIActor::s_instance;

void UIActor::InitializeSingleton(HWND parent)
{
    if (!s_instance)
        s_instance = std::make_unique<UIActor>(L"MainUI");

    s_instance->AddComponent(std::make_unique<CloseComponent>());
    s_instance->AddComponent(std::make_unique<ScrollComponent>());
    s_instance->AddComponent(std::make_unique<DragComponent>());
    s_instance->AddComponent(std::make_unique<InputComponent>());
    s_instance->Initialize(parent);
    s_instance->Show();

    TaskToolPanel::Setup(*s_instance);
}

void UIActor::ShutdownSingleton()
{
    if (s_instance)
    {
        s_instance->Shutdown();
        s_instance.reset();
    }
}

UIActor& UIActor::GetInstance()
{
    if (!s_instance)
        s_instance = std::make_unique<UIActor>(L"MainUI");
    return *s_instance;
}

