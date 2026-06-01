#pragma once

#include "Systems/UI/Components/CloseComponent.h"
#include "Systems/UI/Components/DragComponent.h"
#include "Systems/UI/Components/InputComponent.h"
#include "Systems/UI/Components/ScrollComponent.h"

#include <string>

namespace pet::systems::ui {

class UIActor {
public:
    static UIActor& Get();

    void Initialize();
    void Shutdown();
    void Update();

    void Show();
    void Hide();
    bool IsVisible() const;

    void OnCloseRequested();
    void OnScroll(int delta);
    void OnDragBegin(int x, int y);
    void OnDragMove(int x, int y);
    void OnDragEnd();

    void SetInputText(const std::wstring& text);
    void SubmitInput();

    int GetScrollOffset() const;
    const components::DragState& GetDragState() const;
    const std::wstring& GetInputText() const;

private:
    UIActor() = default;
    void RegisterRuntimeEvents();
    void UnregisterRuntimeEvents();

    bool initialized_ = false;
    bool visible_ = true;
    bool dragging_ = false;
    int dragOffsetX_ = 0;
    int dragOffsetY_ = 0;
    int onInputChar_ = 0;
    int onMouseDown_ = 0;
    int onMouseMove_ = 0;
    int onMouseUp_ = 0;
    int onMouseWheel_ = 0;

    components::CloseComponent close_;
    components::ScrollComponent scroll_;
    components::DragComponent drag_;
    components::InputComponent input_;
};

} // namespace pet::systems::ui
