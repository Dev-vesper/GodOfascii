#include "ui/SettingsPage.h"
#include "ui/MenuPanel.h"
#include "render/CharGrid.h"
#include <algorithm>
#include <string>

namespace settingspage {

MenuCommand handle(int item, MenuEvent ev) {
    switch (item) {
        case 0:
            return ev == MenuEvent::Left ? MenuCommand::FovDown
                                         : MenuCommand::FovUp;
        case 1:
            return MenuCommand::ToggleMinimap;
        default:
            return MenuCommand::ToggleFullscreen;
    }
}

void draw(CharGrid& grid, MenuPanel& panel, int item, int fov,
          bool minimapOn, bool fullscreenOn) {
    panel.reset(grid, kPanelW, kPanelH);
    panel.putText((panel.width() - 8) / 2, 1, "settings", menuui::kTitle);
    const std::string labels[kItemCount] = {"fov", "minimap", "fullscreen"};
    const std::string values[kItemCount] = {
        "< " + std::to_string(fov) + " >",
        minimapOn ? "on" : "off",
        fullscreenOn ? "on" : "off",
    };
    for (int i = 0; i < kItemCount; ++i) {
        const bool sel = i == item;
        panel.marker(3 + i, sel);
        panel.putText(3, 3 + i, labels[i], sel ? menuui::kItemSelected
                                                : menuui::kItem);
        panel.putText(
            std::max(panel.width() - 2 - static_cast<int>(values[i].size()), 3),
            3 + i, values[i], sel ? menuui::kItemSelected : menuui::kItem);
    }
}

}  // namespace settingspage
