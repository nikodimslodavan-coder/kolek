#include "game/game.h"
#include <fstream>

namespace kolek {

static std::string configPath()
{
#ifdef KOLEK_ANDROID
    // SDL_GetBasePath() на Android возвращает NULL, а корень "/" недоступен
    // для записи. Раскладку храним во внутреннем каталоге приложения.
    char* pref = SDL_GetPrefPath("kolek", "diamond");
    if (pref) {
        std::string path = pref;
        SDL_free(pref);
        path += "kolek.cfg";
        return path;
    }
#endif
    std::string path;
    char* base = SDL_GetBasePath();
    if (base) {
        path = base;
        SDL_free(base);
    }
    path += "kolek.cfg";
    return path;
}

void Game::loadControls()
{
    controls.reset();
    std::ifstream f(configPath());
    if (!f) return;

    std::string magic;
    int version = 0;
    if (!(f >> magic >> version)) return;
    if (magic != "kolek-controls") return;

    auto valid = [](int v) {
        return v >= static_cast<int>(SDL_SCANCODE_UNKNOWN) &&
               v < SDL_NUM_SCANCODES &&
               v != static_cast<int>(SDL_SCANCODE_ESCAPE);
    };

    if (version == 1) {
        // Старый файл: по одной клавише на действие, включая отдельный прицел.
        int oldKeys[9] = {};
        for (int i = 0; i < 9; ++i)
            if (!(f >> oldKeys[i]) || !valid(oldKeys[i])) return;
        // Старые индексы: влево, вправо, вверх, вниз, прыжок, прицел↑,
        // прицел↓, выстрел, заново. Прицел теперь идёт от движения.
        const int map[CA_COUNT] = {0, 1, 2, 3, 4, 7, 8};
        for (int i = 0; i < CA_COUNT; ++i) {
            controls.keys[i][0] = static_cast<SDL_Scancode>(oldKeys[map[i]]);
            if (controls.keys[i][1] == controls.keys[i][0])
                controls.keys[i][1] = SDL_SCANCODE_UNKNOWN;
        }
        return;
    }
    if (version != 2) return;

    for (int i = 0; i < CA_COUNT; ++i) {
        int a = 0, b = 0;
        if (!(f >> a >> b)) return;
        if (!valid(a) || !valid(b)) return;
        if (a == b && a != static_cast<int>(SDL_SCANCODE_UNKNOWN)) return;
        controls.keys[i][0] = static_cast<SDL_Scancode>(a);
        controls.keys[i][1] = static_cast<SDL_Scancode>(b);
    }
}

void Game::saveControls() const
{
    if (!persistControls) return;

    std::ofstream f(configPath(), std::ios::trunc);
    if (!f) return;
    f << "kolek-controls 2\n";
    for (int i = 0; i < CA_COUNT; ++i) {
        if (i) f << ' ';
        f << static_cast<int>(controls.keys[i][0]) << ' '
          << static_cast<int>(controls.keys[i][1]);
    }
    f << '\n';
}

} // namespace kolek
