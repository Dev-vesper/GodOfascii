#include "ui/MenuPanel.h"
#include "render/CharGrid.h"
#include <algorithm>

void MenuPanel::reset(CharGrid& grid, int w, int h) {
    ok_ = false;
    const int gw = grid.width();
    const int gh = grid.height();
    w_ = std::min(gw - 2, w);
    h_ = h;
    if (w_ < 10 || gh < h_) return;
    x0_ = (gw - w_) / 2;
    y0_ = (gh - h_) / 2;

    cells_.assign(static_cast<size_t>(w_) * h_, Cell{});
    for (int x = 0; x < w_; ++x) {
        const char ch = x == 0 || x == w_ - 1 ? '+' : '-';
        cells_[static_cast<size_t>(x)] = {ch, true, menuui::kBorder};
        cells_[static_cast<size_t>(h_ - 1) * w_ + x] = {ch, true,
                                                        menuui::kBorder};
    }
    for (int y = 1; y < h_ - 1; ++y) {
        cells_[static_cast<size_t>(y) * w_] = {'|', true, menuui::kBorder};
        cells_[static_cast<size_t>(y) * w_ + w_ - 1] = {'|', true,
                                                        menuui::kBorder};
    }
    ok_ = true;
}

void MenuPanel::put(int x, int y, char ch, Rgb fg) {
    if (!ok_ || x < 1 || x >= w_ - 1 || y < 0 || y >= h_) return;
    cells_[static_cast<size_t>(y) * w_ + x] = {ch, true, fg};
}

void MenuPanel::putText(int x, int y, const std::string& s, Rgb fg) {
    for (int i = 0; i < static_cast<int>(s.size()); ++i) {
        put(x + i, y, s[static_cast<size_t>(i)], fg);
    }
}

void MenuPanel::marker(int y, bool selected) {
    put(1, y, selected ? '>' : ' ',
        selected ? menuui::kItemSelected : menuui::kItem);
}

void MenuPanel::blendOver(CharGrid& grid) const {
    if (!ok_) return;
    const int w = grid.width();
    const int h = grid.height();
    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            const int gx = x0_ + x;
            const int gy = y0_ + y;
            if (gx < 0 || gy < 0 || gx >= w || gy >= h) continue;
            const ::Cell old = grid.at(gx, gy);
            const Cell& pc = cells_[static_cast<size_t>(y) * w_ + x];
            const Rgb bg = lerp(old.bg, menuui::kPanelBg, menuui::kPanelAlpha);
            const Rgb fg = pc.glyph ? pc.fg
                                    : lerp(old.fg, menuui::kPanelBg,
                                           menuui::kPanelAlpha);
            grid.set(gx, gy, pc.glyph ? pc.ch : old.ch, fg, bg);
        }
    }
}
