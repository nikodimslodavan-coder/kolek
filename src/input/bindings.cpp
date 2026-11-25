#include "game/game.h"

namespace kolek {

void Game::assignBind(SDL_Scancode sc)
{
    if (bindTarget < 0 || bindTarget >= CA_COUNT) return;
    if (bindSlot < 0 || bindSlot > 1) return;
    if (sc == SDL_SCANCODE_UNKNOWN || sc == SDL_SCANCODE_ESCAPE) return;

    // Одинаковых клавиш в двух слотах одного действия не держим:
    // назначенная клавиша меняется местами со второй.
    int other = 1 - bindSlot;
    if (controls.keys[bindTarget][other] == sc)
        controls.keys[bindTarget][other] = controls.keys[bindTarget][bindSlot];
    controls.keys[bindTarget][bindSlot] = sc;
    bindTarget = -1;
    saveControls();
}

} // namespace kolek
