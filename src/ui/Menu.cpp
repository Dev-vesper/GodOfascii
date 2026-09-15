#include "ui/Menu.h"
#include "ui/MenuPanel.h"
#include "ui/SettingsPage.h"
#include "render/CharGrid.h"

namespace {
constexpr int kMainItems = 3;
}

void Menu::setActive(bool on) {
    active_ = on;
    page_ = Page::Main;
    item_ = 0;
}

MenuCommand Menu::handle(MenuEvent ev, char typed) {
    (void)typed;  // no text entry on this menu
    if (!active_) return MenuCommand::None;
    switch (ev) {
        case MenuEvent::Up:
            item_ = (item_ + kMainItems - 1) % kMainItems;
            return MenuCommand::None;
        case MenuEvent::Down:
            item_ = (item_ + 1) % kMainItems;
            return MenuCommand::None;
        case MenuEvent::Back:
            if (page_ == Page::Settings) {
                page_ = Page::Main;
                item_ = 0;
                return MenuCommand::None;
            }
            return MenuCommand::Resume;
        default:
            break;
    }
    if (page_ == Page::Main) {
        if (ev != MenuEvent::Confirm) return MenuCommand::None;
        switch (item_) {
            case 0: return MenuCommand::Resume;
            case 1:
                page_ = Page::Settings;
                item_ = 0;
                return MenuCommand::None;
            default: return MenuCommand::Exit;
        }
    }
    return settingspage::handle(item_, ev);
}

void Menu::draw(CharGrid& grid, int fov, bool minimapOn,
                bool fullscreenOn) const {
    if (!active_) return;
    MenuPanel panel;
    if (page_ == Page::Main) {
        panel.reset(grid, settingspage::kPanelW, settingspage::kPanelH);
        panel.putText((panel.width() - 7) / 2, 1, "ascii3d", menuui::kTitle);
        const char* items[kMainItems] = {"resume", "settings", "exit"};
        for (int i = 0; i < kMainItems; ++i) {
            const bool sel = i == item_;
            panel.marker(3 + i, sel);
            panel.putText(3, 3 + i, items[i],
                          sel ? menuui::kItemSelected : menuui::kItem);
        }
    } else {
        settingspage::draw(grid, panel, item_, fov, minimapOn, fullscreenOn);
    }
    panel.blendOver(grid);
}
