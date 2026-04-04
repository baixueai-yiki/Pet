#pragma once

#include <windows.h>
#include <memory>
#include <string>
#include <vector>

class UIComponent;

class UIActor
{
public:
    UIActor(const std::wstring& name);
    ~UIActor();

    void Initialize(HWND parentHwnd);
    void Shutdown();

    void AddComponent(std::unique_ptr<UIComponent> component);

    HWND GetParent() const { return m_parentHwnd; }
    const std::wstring& GetName() const { return m_name; }

    void Show();
    void Hide();
    bool IsVisible() const { return m_visible; }

    void NotifyMouseClick(int x, int y);
    void NotifyMouseWheel(int delta);

private:
    std::wstring m_name;
    HWND m_parentHwnd = nullptr;
    bool m_visible = false;
    std::vector<std::unique_ptr<UIComponent>> m_components;
};

class UIComponent
{
public:
    virtual ~UIComponent() = default;
    virtual void OnInit(UIActor& actor) {}
    virtual void OnShutdown(UIActor& actor) {}
    virtual void OnMouseClick(UIActor& actor, int x, int y) {}
    virtual void OnMouseWheel(UIActor& actor, int delta) {}
};
