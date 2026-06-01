#include "Engine/Render/Renderer.h"

#include "Core/Path.h"
#include "Systems/Pet/Components/PetRenderComponent.h"

#include <gdiplus.h>
#include <vector>

using namespace Gdiplus;

namespace pet::engine::render {

namespace {
Image* g_image = nullptr;
ULONG_PTR g_token = 0;

Image* LoadImage(const std::vector<std::wstring>& candidates) {
    for (const auto& path : candidates) {
        Image* img = Image::FromFile(path.c_str());
        if (img && img->GetLastStatus() == Ok) {
            return img;
        }
        delete img;
    }
    return nullptr;
}
} // namespace

bool Renderer::Initialize() {
    if (g_image) {
        return true;
    }

    GdiplusStartupInput input;
    if (GdiplusStartup(&g_token, &input, nullptr) != Ok) {
        return false;
    }

    if (!LoadImageFromCandidates()) {
        GdiplusShutdown(g_token);
        g_token = 0;
        return false;
    }

    return true;
}

bool Renderer::LoadImageFromCandidates() {
    std::vector<std::wstring> candidates = {
        core::Path::GetImagePath(L"qing.png"),
        core::Path::GetImagePath(L"character.png"),
        core::Path::GetExeDir() + L"\\..\\Content\\Images\\qing.png",
        core::Path::GetExeDir() + L"\\..\\Content\\Images\\character.png",
        core::Path::GetExeDir() + L"\\assets\\images\\qing.png",
        core::Path::GetExeDir() + L"\\assets\\images\\character.png"
    };

    g_image = LoadImage(candidates);
    return g_image != nullptr;
}

void Renderer::Shutdown() {
    delete g_image;
    g_image = nullptr;

    if (g_token != 0) {
        GdiplusShutdown(g_token);
        g_token = 0;
    }
}

void Renderer::Render(HDC hdc, const pet::systems::pet::components::RenderState& state) {
    if (!hdc) {
        return;
    }

    RECT rc = {};
    GetClipBox(hdc, &rc);

    Graphics graphics(hdc);
    graphics.Clear(Color(0, 0, 0, 0));

    if (g_image && g_image->GetLastStatus() == Ok) {
        graphics.DrawImage(g_image, state.x, state.y, state.width, state.height);
    }
}

bool Renderer::HasImage() {
    return g_image != nullptr;
}

} // namespace pet::engine::render
