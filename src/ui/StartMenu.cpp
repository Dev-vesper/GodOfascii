#include "ui/StartMenu.h"
#include "ui/MenuPanel.h"
#include "ui/SettingsPage.h"
#include "render/CharGrid.h"

namespace {
constexpr int kMainPanelW = 32;
constexpr int kMainPanelH = 10;
constexpr int kMainItems = 5;
constexpr size_t kMaxAddress = 24;
}

void StartMenu::setActive(bool on) {
    active_ = on;
    page_ = Page::Main;
    item_ = 0;
    status_.clear();
}

MenuCommand StartMenu::handle(MenuEvent ev, char typed) {
    if (!active_) return MenuCommand::None;

    if (page_ == Page::Join) {
        if (typed == '\b') {
            if (!address_.empty()) address_.pop_back();
            return MenuCommand::None;
        }
        if (typed >= 0x20 && typed < 0x7f && address_.size() < kMaxAddress) {
            address_ += typed;
            status_.clear();
            return MenuCommand::None;
        }
        if (ev == MenuEvent::Back) {
            page_ = Page::Main;
            return MenuCommand::None;
        }
        if (ev == MenuEvent::Confirm) return MenuCommand::JoinGame;
        return MenuCommand::None;
    }

    const int count =
        page_ == Page::Main ? kMainItems : settingspage::kItemCount;
    switch (ev) {
        case MenuEvent::Up:
            item_ = (item_ + count - 1) % count;
            return MenuCommand::None;
        case MenuEvent::Down:
            item_ = (item_ + 1) % count;
            return MenuCommand::None;
        case MenuEvent::Back:
            // The main page is the root: Esc waits there instead of
            // quitting, so nobody leaves the game by accident.
            if (page_ == Page::Settings) {
                page_ = Page::Main;
                item_ = 3;
            }
            return MenuCommand::None;
        default:
            break;
    }
    if (page_ == Page::Main) {
        if (ev != MenuEvent::Confirm) return MenuCommand::None;
        switch (item_) {
            case 0: return MenuCommand::StartOffline;
            case 1: return MenuCommand::HostGame;
            case 2:
                page_ = Page::Join;
                status_.clear();
                return MenuCommand::None;
            case 3:
                page_ = Page::Settings;
                item_ = 0;
                return MenuCommand::None;
            default: return MenuCommand::Exit;
        }
    }
    return settingspage::handle(item_, ev);
}

void StartMenu::draw(CharGrid& grid, int fov, bool minimapOn,
                     bool fullscreenOn) const {
    if (!active_) return;
    MenuPanel panel;
    if (page_ == Page::Main || page_ == Page::Join) {
        panel.reset(grid, kMainPanelW, kMainPanelH);
        if (page_ == Page::Main) {
            panel.putText((panel.width() - 7) / 2, 1, "ascii3d", menuui::kTitle);
            const char* items[kMainItems] = {"start", "host game",
                                             "join game", "settings", "exit"};
            for (int i = 0; i < kMainItems; ++i) {
                const bool sel = i == item_;
                panel.marker(3 + i, sel);
                panel.putText(3, 3 + i, items[i],
                              sel ? menuui::kItemSelected : menuui::kItem);
            }
        } else {
            panel.putText((panel.width() - 11) / 2, 1, "join server",
                          menuui::kTitle);
            panel.putText(3, 4, "address:", menuui::kItem);
            // The trailing cursor keeps the entry point visible even when
            // the address fills the field.
            panel.putText(13, 4, address_ + "_", menuui::kItemSelected);
            panel.putText(3, 6, "enter: join   esc: back", menuui::kItem);
        }
        if (!status_.empty()) {
            panel.putText(3, kMainPanelH - 2, status_, menuui::kStatus);
        }
    } else {
        settingspage::draw(grid, panel, item_, fov, minimapOn, fullscreenOn);
    }
    panel.blendOver(grid);
}
