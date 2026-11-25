#include "input/controls.h"

namespace kolek {

const char* controlName(int action)
{
    switch (action) {
        case CA_LEFT:    return "ВЛЕВО";
        case CA_RIGHT:   return "ВПРАВО";
        case CA_UP:      return "ВВЕРХ / ЛАЗАТЬ";
        case CA_DOWN:    return "ВНИЗ / ЛАЗАТЬ";
        case CA_JUMP:    return "ПРЫЖОК";
        case CA_SHOOT:   return "ВЫСТРЕЛ";
        case CA_RESTART: return "ЗАНОВО";
        default:         return "";
    }
}

std::string keyLabel(SDL_Scancode sc)
{
    if (sc == SDL_SCANCODE_UNKNOWN) return "НЕТ";
    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
        return std::string(1, static_cast<char>('A' + (sc - SDL_SCANCODE_A)));
    if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9)
        return std::string(1, static_cast<char>('1' + (sc - SDL_SCANCODE_1)));
    if (sc == SDL_SCANCODE_0) return "0";
    switch (sc) {
        case SDL_SCANCODE_SPACE:     return "ПРОБЕЛ";
        case SDL_SCANCODE_UP:        return "↑";
        case SDL_SCANCODE_DOWN:      return "↓";
        case SDL_SCANCODE_LEFT:      return "←";
        case SDL_SCANCODE_RIGHT:     return "→";
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL:     return "CTRL";
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:    return "SHIFT";
        case SDL_SCANCODE_LALT:
        case SDL_SCANCODE_RALT:      return "ALT";
        case SDL_SCANCODE_RETURN:    return "ENTER";
        case SDL_SCANCODE_TAB:       return "TAB";
        case SDL_SCANCODE_BACKSPACE: return "BACKSPACE";
        default: break;
    }
    if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12)
        return "F" + std::to_string(sc - SDL_SCANCODE_F1 + 1);
    return "КЛАВИША " + std::to_string(static_cast<int>(sc));
}

} // namespace kolek
