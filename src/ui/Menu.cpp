#include "ui/Menu.h"
#include "core/Color.h"
#include "render/CharGrid.h"
#include <algorithm>
#include <string>

namespace {
constexpr int kPanelW = 26;
constexpr int kPanelH = 8;
constexpr int kItemCount = 3;
constexpr float kPanelAlpha = 0.7f;  // panel opacity over the scene
constexpr Rgb kPanelBg{14, 18, 30};
constexpr Rgb kBorder{96, 110, 148};
constexpr Rgb kTitle{214, 190, 120};
constexpr Rgb kItemSelected{120, 220, 255};
constexpr Rgb kItem{160, 165, 180};

// One panel cell: either a glyph with its own color, or an unused cell in
// which case the scene underneath stays visible and only gets dimmed.
struct PanelCell {
    char ch = ' ';
    bool glyph = false;
    Rgb fg{kItem};
};
}  // namespace

void Menu::setActive(bool on) {
    active_ = on;
    page_ = Page::Main;
    item_ = 0;
}

Menu::Command Menu::handle(Event ev) {
    if (!active_) return Command::None;
    switch (ev) {
        case Event::Up:
            item_ = (item_ + kItemCount - 1) % kItemCount;
            return Command::None;
        case Event::Down:
            item_ = (item_ + 1) % kItemCount;
            return Command::None;
        case Event::Back:
            if (page_ == Page::Settings) {
                page_ = Page::Main;
                item_ = 0;
                return Command::None;
            }
            return Command::Resume;
        case Event::Confirm:
        case Event::Left:
        case Event::Right:
            break;
    }
    if (page_ == Page::Main) {
        if (ev != Event::Confirm) return Command::None;
        switch (item_) {
            case 0: return Command::Resume;
            case 1: page_ = Page::Settings; item_ = 0; return Command::None;
            default: return Command::Exit;
        }
    }
    // Settings page: fov adjusts left/right, the toggles flip on any key.
    switch (item_) {
        case 0:
            return ev == Event::Left ? Command::FovDown : Command::FovUp;
        case 1:
            return Command::ToggleMinimap;
        default:
            return Command::ToggleFullscreen;
    }
}

void Menu::draw(CharGrid& grid, int fov, bool minimapOn, bool fullscreenOn) const {
    if (!active_) return;
    const int w = grid.width();
    const int h = grid.height();
    if (w < 10 || h < kPanelH) return;

    const int panelW = std::min(w - 2, kPanelW);
    const int x0 = (w - panelW) / 2;
    const int y0 = (h - kPanelH) / 2;

    PanelCell cells[kPanelH][kPanelW] = {};
    for (int x = 0; x < panelW; ++x) {
        const PanelCell corner{x == 0 || x == panelW - 1 ? '+' : '-', true, kBorder};
        cells[0][x] = corner;
        cells[kPanelH - 1][x] = corner;
    }
    for (int y = 1; y < kPanelH - 1; ++y) {
        cells[y][0] = {'|', true, kBorder};
        cells[y][panelW - 1] = {'|', true, kBorder};
    }
    const auto putText = [&](int x, int y, const std::string& s, Rgb fg) {
        for (int i = 0; i < static_cast<int>(s.size()); ++i) {
            if (x + i < 1 || x + i >= panelW - 1) continue;
            cells[y][x + i] = {s[static_cast<size_t>(i)], true, fg};
        }
    };

    if (page_ == Page::Main) {
        putText((panelW - 7) / 2, 1, "ascii3d", kTitle);
        const char* items[kItemCount] = {"resume", "settings", "exit"};
        for (int i = 0; i < kItemCount; ++i) {
            const bool sel = i == item_;
            cells[3 + i][1] = {sel ? '>' : ' ', true, sel ? kItemSelected : kItem};
            putText(3, 3 + i, items[i], sel ? kItemSelected : kItem);
        }
    } else {
        putText((panelW - 8) / 2, 1, "settings", kTitle);
        const std::string labels[kItemCount] = {"fov", "minimap", "fullscreen"};
        const std::string values[kItemCount] = {
            "< " + std::to_string(fov) + " >",
            minimapOn ? "on" : "off",
            fullscreenOn ? "on" : "off",
        };
        for (int i = 0; i < kItemCount; ++i) {
            const bool sel = i == item_;
            cells[3 + i][1] = {sel ? '>' : ' ', true, sel ? kItemSelected : kItem};
            putText(3, 3 + i, labels[i], sel ? kItemSelected : kItem);
            putText(std::max(panelW - 2 - static_cast<int>(values[i].size()), 3),
                    3 + i, values[i], sel ? kItemSelected : kItem);
        }
    }

    // Blend pass: the panel color bleeds into the frame underneath, and
    // unused cells keep the scene glyph, dimmed.
    for (int y = 0; y < kPanelH; ++y) {
        for (int x = 0; x < panelW; ++x) {
            const int gx = x0 + x;
            const int gy = y0 + y;
            if (gx < 0 || gy < 0 || gx >= w || gy >= h) continue;
            const Cell old = grid.at(gx, gy);
            const PanelCell& pc = cells[y][x];
            const Rgb bg = lerp(old.bg, kPanelBg, kPanelAlpha);
            const Rgb fg = pc.glyph ? pc.fg : lerp(old.fg, kPanelBg, kPanelAlpha);
            grid.set(gx, gy, pc.glyph ? pc.ch : old.ch, fg, bg);
        }
    }
}
