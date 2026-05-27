#pragma once

namespace pet::systems::ui::panels::tool {

class SettingToolPanel {
public:
    static void Setup();
    static void Open();
    static void Close();
    static bool IsOpen();
};

} // namespace pet::systems::ui::panels::tool
