#include "Systems/UI/UIActor.h"

#include "Engine/Input/Mouse.h"
#include "Runtime/EventBus.h"

namespace pet::systems::ui {

UIActor& UIActor::Get() {
    static UIActor instance;
    return instance;
}

void UIActor::Initialize() {
    if (initialized_) {
        return;
    }

    RegisterRuntimeEvents();
    visible_ = true;
    initialized_ = true;
    runtime::EventBus::Emit(L"ui.actor.initialized");
}

void UIActor::Shutdown() {
    if (!initialized_) {
        return;
    }

    UnregisterRuntimeEvents();
    runtime::EventBus::Emit(L"ui.actor.shutdown");
    initialized_ = false;
}

void UIActor::Update() {
    if (!initialized_) {
        return;
    }

    if (close_.IsRequested()) {
        visible_ = false;
        close_.Reset();
    }
}

void UIActor::Show() {
    visible_ = true;
    runtime::EventBus::Emit(L"ui.actor.show");
}

void UIActor::Hide() {
    visible_ = false;
    runtime::EventBus::Emit(L"ui.actor.hide");
}

bool UIActor::IsVisible() const {
    return visible_;
}

void UIActor::OnCloseRequested() {
    close_.RequestCloseAll();
}

void UIActor::OnScroll(int delta) {
    scroll_.AddDelta(delta);
}

void UIActor::OnDragBegin(int x, int y) {
    drag_.Begin(x, y);
}

void UIActor::OnDragMove(int x, int y) {
    drag_.MoveTo(x, y);
}

void UIActor::OnDragEnd() {
    drag_.End();
}

void UIActor::SetInputText(const std::wstring& text) {
    input_.SetText(text);
}

void UIActor::SubmitInput() {
    input_.Submit();
}

int UIActor::GetScrollOffset() const {
    return scroll_.GetOffset();
}

const components::DragState& UIActor::GetDragState() const {
    return drag_.GetState();
}

const std::wstring& UIActor::GetInputText() const {
    return input_.GetText();
}

void UIActor::RegisterRuntimeEvents() {
    if (onInputChar_ == 0) {
        onInputChar_ = runtime::EventBus::Subscribe(L"engine.input.char", [&](const runtime::Event& evt) {
            if (evt.payload.empty()) {
                return;
            }
            const wchar_t ch = evt.payload.front();
            if (ch == L'\r' || ch == L'\n') {
                SubmitInput();
                return;
            }
            SetInputText(GetInputText() + std::wstring(1, ch));
        });
    }

    if (onMouseDown_ == 0) {
        onMouseDown_ = runtime::EventBus::Subscribe(L"engine.input.mouse.left.down", [&](const runtime::Event&) {
            const auto m = engine::input::Mouse::GetState();
            OnDragBegin(m.x, m.y);
        });
    }

    if (onMouseMove_ == 0) {
        onMouseMove_ = runtime::EventBus::Subscribe(L"engine.input.mouse.move", [&](const runtime::Event&) {
            const auto m = engine::input::Mouse::GetState();
            OnDragMove(m.x, m.y);
        });
    }

    if (onMouseUp_ == 0) {
        onMouseUp_ = runtime::EventBus::Subscribe(L"engine.input.mouse.left.up", [&](const runtime::Event&) {
            OnDragEnd();
        });
    }

    if (onMouseWheel_ == 0) {
        onMouseWheel_ = runtime::EventBus::Subscribe(L"engine.input.mouse.wheel", [&](const runtime::Event&) {
            const auto m = engine::input::Mouse::GetState();
            OnScroll(m.wheelDelta);
        });
    }
}

void UIActor::UnregisterRuntimeEvents() {
    if (onInputChar_ != 0) {
        runtime::EventBus::Unsubscribe(L"engine.input.char", onInputChar_);
        onInputChar_ = 0;
    }
    if (onMouseDown_ != 0) {
        runtime::EventBus::Unsubscribe(L"engine.input.mouse.left.down", onMouseDown_);
        onMouseDown_ = 0;
    }
    if (onMouseMove_ != 0) {
        runtime::EventBus::Unsubscribe(L"engine.input.mouse.move", onMouseMove_);
        onMouseMove_ = 0;
    }
    if (onMouseUp_ != 0) {
        runtime::EventBus::Unsubscribe(L"engine.input.mouse.left.up", onMouseUp_);
        onMouseUp_ = 0;
    }
    if (onMouseWheel_ != 0) {
        runtime::EventBus::Unsubscribe(L"engine.input.mouse.wheel", onMouseWheel_);
        onMouseWheel_ = 0;
    }
}

} // namespace pet::systems::ui
