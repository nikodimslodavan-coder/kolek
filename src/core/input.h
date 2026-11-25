#pragma once

namespace kolek {

struct Input {
    bool left = false, right = false, up = false, down = false;
    bool lookUp = false, lookDown = false;
    bool spacePressed = false, jumpHeld = false;
    bool shootPressed = false;
    bool confirm = false;
    bool restart = false;
    bool secretDoor = false;   // DEV: Ctrl+R — секретная дверь
    // Навигация по меню (не переназначается).
    bool menuUp = false, menuDown = false, menuLeft = false, menuRight = false;
    bool back = false;
};

} // namespace kolek
