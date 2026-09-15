#pragma once
#include "platform/Input.h"

// Shared vocabulary of every menu surface: navigation events distilled
// from a frame's input actions, and the commands pages hand back to the
// game. Keeping them common lets the in-game menu, the start menu and the
// settings page stay interchangeable building blocks.
enum class MenuEvent {
    None, Up, Down, Left, Right, Confirm, Back,
};

enum class MenuCommand {
    None,
    Resume,        // in-game menu: close it
    Exit,          // any menu: quit the game
    StartOffline,  // start menu: play without a server
    HostGame,      // start menu: spawn a local server and join it
    JoinGame,      // start menu: join the typed address
    // Settings page (shared by both menus).
    FovDown,
    FovUp,
    ToggleMinimap,
    ToggleFullscreen,
};

// The navigation event this frame's menu actions imply, if any. Esc maps
// to Back so open menus can treat it as "one page up".
inline MenuEvent menuEventFromActions(const Input& input) {
    if (input.triggered(Action::MenuConfirm)) return MenuEvent::Confirm;
    if (input.triggered(Action::MenuToggle)) return MenuEvent::Back;
    if (input.triggered(Action::MenuUp)) return MenuEvent::Up;
    if (input.triggered(Action::MenuDown)) return MenuEvent::Down;
    if (input.triggered(Action::MenuLeft)) return MenuEvent::Left;
    if (input.triggered(Action::MenuRight)) return MenuEvent::Right;
    return MenuEvent::None;
}
