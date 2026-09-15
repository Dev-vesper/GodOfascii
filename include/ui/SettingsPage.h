#pragma once
#include "ui/MenuDefs.h"

class CharGrid;
class MenuPanel;

// The fov/minimap/fullscreen page shared by the in-game menu and the start
// menu: left/right adjusts fov, any key flips the toggles. The owning menu
// keeps the selected row and calls in with it.
namespace settingspage {
constexpr int kItemCount = 3;
constexpr int kPanelW = 26;
constexpr int kPanelH = 8;

// Command for a navigation event on the given row.
MenuCommand handle(int item, MenuEvent ev);
// Builds the whole page in a fresh panel (border, title, rows) over grid.
void draw(CharGrid& grid, MenuPanel& panel, int item, int fov,
          bool minimapOn, bool fullscreenOn);
}
