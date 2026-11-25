#pragma once

#include <SDL_scancode.h>
#include <string>

namespace kolek {

enum ControlAction {
    CA_LEFT = 0, CA_RIGHT, CA_UP, CA_DOWN, CA_JUMP,
    CA_SHOOT, CA_RESTART, CA_COUNT
};

// На каждое действие можно назначить две клавиши: основную и запасную.
struct Controls {
    SDL_Scancode keys[CA_COUNT][2] = {};

    void reset()
    {
        for (int i = 0; i < CA_COUNT; ++i) {
            keys[i][0] = SDL_SCANCODE_UNKNOWN;
            keys[i][1] = SDL_SCANCODE_UNKNOWN;
        }
        keys[CA_LEFT][0]    = SDL_SCANCODE_A;
        keys[CA_LEFT][1]    = SDL_SCANCODE_LEFT;
        keys[CA_RIGHT][0]   = SDL_SCANCODE_D;
        keys[CA_RIGHT][1]   = SDL_SCANCODE_RIGHT;
        keys[CA_UP][0]      = SDL_SCANCODE_W;
        keys[CA_UP][1]      = SDL_SCANCODE_UP;
        keys[CA_DOWN][0]    = SDL_SCANCODE_S;
        keys[CA_DOWN][1]    = SDL_SCANCODE_DOWN;
        // W остаётся и прыжком, и движением вверх — как в прежней версии.
        keys[CA_JUMP][0]    = SDL_SCANCODE_SPACE;
        keys[CA_JUMP][1]    = SDL_SCANCODE_W;
        keys[CA_SHOOT][0]   = SDL_SCANCODE_LCTRL;
        keys[CA_SHOOT][1]   = SDL_SCANCODE_RCTRL;
        keys[CA_RESTART][0] = SDL_SCANCODE_R;
    }
};

const char* controlName(int action);
std::string keyLabel(SDL_Scancode sc);

} // namespace kolek
