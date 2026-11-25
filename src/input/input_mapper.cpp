#include "input/input_mapper.h"

namespace kolek {

Input collectInput(const Controls& controls, const TouchControls& touch,
                   const Uint8* ks, const Uint8* prevKeys, bool backQueued)
{
    auto held = [&](int action) {
        for (int s = 0; s < 2; ++s) {
            SDL_Scancode sc = controls.keys[action][s];
            if (sc != SDL_SCANCODE_UNKNOWN && ks[sc]) return true;
        }
        return false;
    };
    auto edge = [&](SDL_Scancode sc) { return ks[sc] && !prevKeys[sc]; };
    auto edgeAction = [&](int action) {
        for (int s = 0; s < 2; ++s) {
            SDL_Scancode sc = controls.keys[action][s];
            if (sc != SDL_SCANCODE_UNKNOWN && edge(sc)) return true;
        }
        return false;
    };

    const TouchControls& tc = touch;
    Input in;
    in.left = held(CA_LEFT) || tc.held[TB_LEFT];
    in.right = held(CA_RIGHT) || tc.held[TB_RIGHT];
    in.up = held(CA_UP) || tc.held[TB_UP];
    in.down = held(CA_DOWN) || tc.held[TB_DOWN];
    // Прицел идёт за направлением движения: вверх/вниз — те же клавиши.
    in.lookUp = in.up;
    in.lookDown = in.down;
    in.jumpHeld = held(CA_JUMP) || tc.held[TB_JUMP];

    in.spacePressed = edgeAction(CA_JUMP) || tc.pressed[TB_JUMP];
    in.shootPressed = edgeAction(CA_SHOOT) || tc.pressed[TB_SHOOT];
    in.restart = edgeAction(CA_RESTART) || tc.pressed[TB_RESTART];
    // Меню всегда ходит на W/S и стрелках, подтверждение — Enter/Space
    // или экранная кнопка «ОК», в настройках A/D и ←/→ выбирают одну из
    // двух кнопок, назад — Esc, Android-кнопка «назад» или «МЕНЮ».
    in.menuUp = edge(SDL_SCANCODE_W) || edge(SDL_SCANCODE_UP) ||
                tc.pressed[TB_UP];
    in.menuDown = edge(SDL_SCANCODE_S) || edge(SDL_SCANCODE_DOWN) ||
                  tc.pressed[TB_DOWN];
    in.menuLeft = edge(SDL_SCANCODE_A) || edge(SDL_SCANCODE_LEFT) ||
                  tc.pressed[TB_LEFT];
    in.menuRight = edge(SDL_SCANCODE_D) || edge(SDL_SCANCODE_RIGHT) ||
                   tc.pressed[TB_RIGHT];
    in.confirm = edge(SDL_SCANCODE_RETURN) || edge(SDL_SCANCODE_SPACE) ||
                 tc.pressed[TB_SHOOT];
    in.back = edge(SDL_SCANCODE_ESCAPE) ||
              backQueued || tc.pressed[TB_BACK];
    // DEV: Ctrl+R создаёт секретную дверь. R по умолчанию занята
    // перезапуском, поэтому с зажатым Ctrl перезапуск не срабатывает.
    if (DEV_SECRET_LEVELS) {
        bool ctrlHeld = ks[SDL_SCANCODE_LCTRL] || ks[SDL_SCANCODE_RCTRL];
        in.secretDoor = ctrlHeld && edge(SDL_SCANCODE_R);
        if (in.secretDoor) in.restart = false;
    }
    return in;
}

} // namespace kolek
