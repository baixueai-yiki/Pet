#include "Systems/Pet/Components/DiaryComponent.h"

#include "Core/Path.h"
#include "Core/TextFile.h"
#include "Runtime/EventBus.h"

namespace pet::systems::pet::components {

bool DiaryComponent::AppendLine(const std::wstring& line) {
    std::wstring normalized = line;
    if (normalized.empty() || normalized.back() != L'\n') {
        normalized += L"\n";
    }

    const std::wstring path = core::Path::GetExeDir() + L"\\diary.txt";
    const bool ok = core::TextFile::WriteText(path, normalized, true, true);
    if (ok) {
        runtime::EventBus::Emit(L"pet.diary.append", line);
    }
    return ok;
}

} // namespace pet::systems::pet::components
