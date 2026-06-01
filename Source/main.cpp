#include "Core/Logger.h"
#include "Core/Timer.h"
#include "Engine/Input/InputDispatcher.h"
#include "Engine/Input/Mouse.h"
#include "Engine/Render/Renderer.h"
#include "Engine/Window/WindowEvents.h"
#include "Engine/Window/WindowLifecycle.h"
#include "Runtime/Scheduler.h"
#include "Runtime/StateManager.h"
#include "Systems/Pet/PetActor.h"
#include "Systems/UI/UIActor.h"

#include <thread>

int wmain() {
    using namespace pet;

    runtime::StateManager::BeginUpdate();
    runtime::StateManager::Set(L"app.mode", L"normal");
    runtime::StateManager::Set(L"pet.state", L"idle");
    runtime::StateManager::EndUpdate();

    engine::window::WindowDesc windowDesc;
    windowDesc.className = L"PetMainWindow";
    windowDesc.title = L"Pet";
    windowDesc.width = GetSystemMetrics(SM_CXSCREEN);
    windowDesc.height = GetSystemMetrics(SM_CYSCREEN);
    windowDesc.style.topMost = true;
    windowDesc.style.clickThrough = false;
    windowDesc.style.useColorKey = true;
    windowDesc.style.colorKey = RGB(0, 0, 0);

    if (!engine::render::Renderer::Initialize()) {
        core::Logger::Error(L"Failed to initialize renderer.");
        return 1;
    }

    HWND hwnd = engine::window::WindowLifecycle::Create(windowDesc, engine::window::WindowLifecycle::HandleMessage);
    if (!hwnd) {
        core::Logger::Error(L"Failed to create main window.");
        engine::render::Renderer::Shutdown();
        return 1;
    }

    engine::window::WindowLifecycle::Show(hwnd);

    systems::pet::PetActor::Get().Initialize();
    systems::ui::UIActor::Get().Initialize();

    core::Logger::Info(L"Pet runtime started.");

    while (engine::window::WindowEvents::Poll()) {
        runtime::Scheduler::Tick();
        systems::pet::PetActor::Get().Update();
        systems::ui::UIActor::Get().Update();

        engine::input::Mouse::ClearFrameDelta();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    systems::ui::UIActor::Get().Shutdown();
    systems::pet::PetActor::Get().Shutdown();
    engine::render::Renderer::Shutdown();
    runtime::StateManager::ClearProfiles();
    runtime::Scheduler::Clear();

    core::Logger::Info(L"Pet runtime stopped.");
    return 0;
}
