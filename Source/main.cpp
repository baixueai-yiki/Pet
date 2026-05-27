#include "Core/Logger.h"
#include "Core/Timer.h"
#include "Engine/Input/InputDispatcher.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Input/TextInputHandler.h"
#include "Engine/Window/WindowCore.h"
#include "Engine/Window/WindowEvents.h"
#include "Engine/Window/WindowLifecycle.h"
#include "Runtime/EventBus.h"
#include "Runtime/Scheduler.h"
#include "Runtime/StateManager.h"
#include "Systems/Pet/PetActor.h"
#include "Systems/UI/UIActor.h"

#include <thread>

namespace {

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    using namespace pet;

    if (engine::input::InputDispatcher::Dispatch(msg, wParam, lParam)) {
        switch (msg) {
        case WM_CHAR: {
            runtime::EventBus::Emit(L"engine.input.char", std::wstring(1, static_cast<wchar_t>(wParam)));
            break;
        }
        case WM_LBUTTONDOWN:
            runtime::EventBus::Emit(L"engine.input.mouse.left.down");
            break;
        case WM_MOUSEMOVE:
            runtime::EventBus::Emit(L"engine.input.mouse.move");
            break;
        case WM_LBUTTONUP:
            runtime::EventBus::Emit(L"engine.input.mouse.left.up");
            break;
        case WM_MOUSEWHEEL:
            runtime::EventBus::Emit(L"engine.input.mouse.wheel");
            break;
        default:
            break;
        }
        return 0;
    }

    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

} // namespace

int wmain() {
    using namespace pet;

    runtime::StateManager::BeginUpdate();
    runtime::StateManager::Set(L"app.mode", L"normal");
    runtime::StateManager::Set(L"pet.state", L"idle");
    runtime::StateManager::EndUpdate();

    bool running = true;
    const int onQuit = runtime::EventBus::Subscribe(L"app.quit", [&](const runtime::Event&) {
        running = false;
    });

    const int onHeartbeat = runtime::EventBus::Subscribe(L"app.heartbeat", [&](const runtime::Event&) {
        core::Logger::Info(L"heartbeat");
    });

    engine::window::WindowDesc windowDesc;
    windowDesc.className = L"PetMainWindow";
    windowDesc.title = L"Pet";
    windowDesc.width = 960;
    windowDesc.height = 540;
    windowDesc.style.topMost = false;
    windowDesc.style.clickThrough = false;
    windowDesc.style.alpha = 255;

    HWND hwnd = engine::window::WindowLifecycle::Create(windowDesc, MainWndProc);
    if (!hwnd) {
        core::Logger::Error(L"Failed to create main window.");
        return 1;
    }

    engine::window::WindowLifecycle::Show(hwnd);

    systems::pet::PetActor::Get().Initialize();
    systems::ui::UIActor::Get().Initialize();

    const int heartbeatScheduleId = runtime::Scheduler::ScheduleEveryMs(L"app.heartbeat", 1000);

    core::Logger::Info(L"Pet runtime started.");

    while (running && engine::window::WindowEvents::Poll()) {
        engine::input::TextInputHandler::BeginFrame();

        runtime::Scheduler::Tick();
        systems::pet::PetActor::Get().Update();
        systems::ui::UIActor::Get().Update();

        engine::input::Mouse::ClearFrameDelta();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    systems::ui::UIActor::Get().Shutdown();
    systems::pet::PetActor::Get().Shutdown();

    runtime::Scheduler::CancelSchedule(heartbeatScheduleId);
    runtime::EventBus::Unsubscribe(L"app.heartbeat", onHeartbeat);
    runtime::EventBus::Unsubscribe(L"app.quit", onQuit);
    runtime::StateManager::ClearProfiles();
    runtime::Scheduler::Clear();
    runtime::EventBus::Clear();

    core::Logger::Info(L"Pet runtime stopped.");
    return 0;
}
