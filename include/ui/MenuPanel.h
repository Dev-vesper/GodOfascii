#pragma once
#include "core/Color.h"
#include <string>
#include <vector>

class CharGrid;

// Shared look of the menu overlays: the palette plus a bordered cell grid
// that pages fill in and then blend translucently over the rendered frame.
namespace menuui {
constexpr float kPanelAlpha = 0.7f;  // panel opacity over the scene
constexpr Rgb kPanelBg{14, 18, 30};
constexpr Rgb kBorder{96, 110, 148};
constexpr Rgb kTitle{214, 190, 120};
constexpr Rgb kItemSelected{120, 220, 255};
constexpr Rgb kItem{160, 165, 180};
constexpr Rgb kStatus{230, 120, 110};
}

// One panel cell: either a glyph with its own color, or an unused cell in
// which case the scene underneath stays visible and only gets dimmed.
class MenuPanel {
public:
    // Fresh bordered panel of w x h cells, clamped to the terminal width
    // and centered on the grid; blendOver() later stamps it there. ok()
    // stays false when the terminal is too small for the panel.
    void reset(CharGrid& grid, int w, int h);
    bool ok() const { return ok_; }
    int width() const { return w_; }

    // Glyphs land inside the borders; writes outside are dropped.
    void put(int x, int y, char ch, Rgb fg);
    void putText(int x, int y, const std::string& s, Rgb fg);
    // Selection marker for an item row: '>' when selected, blank otherwise.
    void marker(int y, bool selected);

    void blendOver(CharGrid& grid) const;

private:
    struct Cell {
        char ch = ' ';
        bool glyph = false;
        Rgb fg{menuui::kItem};
    };

    std::vector<Cell> cells_;
    int w_ = 0;
    int h_ = 0;
    int x0_ = 0;
    int y0_ = 0;
    bool ok_ = false;
};
