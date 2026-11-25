#include "platform/application.h"
#include "game/game.h"
#include "input/input_mapper.h"
#include <clocale>
#include <cstring>

namespace kolek {

int runApplication(int argc, char** argv)
{
#if defined(__linux__) && !defined(__ANDROID__)
    // SDL/X11 использует LC_CTYPE при установке заголовка окна.
    // Начальная локаль C неверно интерпретирует UTF-8 как однобайтовый текст.
    // C.UTF-8 есть в Ubuntu и работает даже при запуске с LANG=C.
    if (!std::setlocale(LC_CTYPE, "C.UTF-8")) {
        std::setlocale(LC_CTYPE, "");
    }
#endif

    bool screens = false;
    bool smoke = false;
#ifdef KOLEK_ANDROID
    bool touchUi = true;
#else
    bool touchUi = false;
#endif
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--screens") == 0) screens = true;
        if (strcmp(argv[i], "--smoke") == 0) smoke = true;
        if (strcmp(argv[i], "--touch") == 0) touchUi = true;
        if (strcmp(argv[i], "--no-touch") == 0) touchUi = false;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
#ifdef KOLEK_ANDROID
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif

    Uint32 winFlags = SDL_WINDOW_RESIZABLE;
#ifdef KOLEK_ANDROID
    // На телефоне окно всегда занимает весь экран.
    winFlags = SDL_WINDOW_FULLSCREEN;
#endif
    winFlags |= (screens || smoke) ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN;
    SDL_Window* win = SDL_CreateWindow(
        "Апартаменты Даймонд — Колян",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        960, 544, winFlags);
    if (!win) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    Uint32 rflags = SDL_RENDERER_ACCELERATED;
    if (!screens && !smoke) rflags |= SDL_RENDERER_PRESENTVSYNC;
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, rflags);
    if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(ren, VIEW_W, VIEW_H);
#ifdef KOLEK_ANDROID
    // На телефонах дробный масштаб заполняет экран заметно лучше.
    SDL_RenderSetIntegerScale(ren, SDL_FALSE);
#else
    SDL_RenderSetIntegerScale(ren, SDL_TRUE);
#endif

    Game game;
    game.touch.enabled = touchUi && !smoke;
#ifdef KOLEK_ANDROID
    game.touch.mouseFallback = false;   // на телефоне не дублируем касания мышью
#else
    game.touch.mouseFallback = true;    // --touch удобно проверять мышью
#endif
    if (screens || smoke) game.persistControls = false;
    if (!game.init(ren)) {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    if (screens) {
        game.makeScenarios();
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 0;
    }
    if (smoke) {
        game.runSmoke();
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 0;
    }

    Uint8 prevKeys[SDL_NUM_SCANCODES] = {};
    memcpy(prevKeys, SDL_GetKeyboardState(nullptr), SDL_NUM_SCANCODES);

    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 last = SDL_GetPerformanceCounter();
    double accumulator = 0.0;
    const double fixedStep = 1.0 / 120.0;
    bool running = true;

    while (running) {
        bool swallowMenu = false;
        bool backQueued = false;
        game.touch.setVisibleForState(game.state, game.exitAction != EXIT_NONE);
        game.touch.beginFrame();
        int winW = 0, winH = 0;
        SDL_GetWindowSize(win, &winW, &winH);

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                game.requestExit(EXIT_GAME);
                swallowMenu = true;
            }
            // F11 — полный экран. Клавиша зарезервирована и не участвует
            // в переназначении управления.
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_F11 && !ev.key.repeat) {
                Uint32 flags = SDL_GetWindowFlags(win);
                bool fullscreen = (flags & SDL_WINDOW_FULLSCREEN) != 0;
                if (SDL_SetWindowFullscreen(
                        win, fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
                    SDL_Log("SDL_SetWindowFullscreen failed: %s", SDL_GetError());
                }
                continue;
            }
            game.touch.handleEvent(ev, winW, winH);
            // Системная кнопка «Назад» может быть нажата и отпущена между
            // двумя кадрами. Запоминаем сам факт нажатия, а не состояние.
            if (ev.type == SDL_KEYDOWN &&
                ev.key.keysym.scancode == SDL_SCANCODE_AC_BACK &&
                !ev.key.repeat)
                backQueued = true;
            if (ev.type == SDL_KEYDOWN && game.bindTarget >= 0 &&
                game.exitAction == EXIT_NONE) {
                SDL_Scancode sc = ev.key.keysym.scancode;
                if (sc == SDL_SCANCODE_ESCAPE) {
                    game.bindTarget = -1;      // отмена переназначения
                } else {
                    game.assignBind(sc);
                }
                // Клавиша, назначенная только что, не должна ещё и
                // сработать как навигация по меню в этом же кадре.
                swallowMenu = true;
            }
        }
        game.touch.endEvents();
        // На Android нет физической клавиатуры: переназначение можно
        // отменить экранной кнопкой «МЕНЮ».
        if (game.bindTarget >= 0 && game.exitAction == EXIT_NONE &&
            game.touch.pressed[TB_BACK]) {
            game.bindTarget = -1;
            swallowMenu = true;
        }

        const Uint8* ks = SDL_GetKeyboardState(nullptr);
        Input in = collectInput(game.controls, game.touch, ks, prevKeys, backQueued);
        if (swallowMenu)
            in.menuUp = in.menuDown = in.menuLeft = in.menuRight =
                in.confirm = in.back = false;

        memcpy(prevKeys, ks, SDL_NUM_SCANCODES);

        Uint64 now = SDL_GetPerformanceCounter();
        double dt = static_cast<double>(now - last) / static_cast<double>(freq);
        last = now;
        if (dt > 0.05) dt = 0.05;
        accumulator += dt;
        int guard = 0;
        bool firstStep = true;
        while (accumulator >= fixedStep && guard < 8) {
            Input stepIn = in;
            if (!firstStep) {
                stepIn.menuUp = stepIn.menuDown = stepIn.menuLeft =
                    stepIn.menuRight = stepIn.confirm = stepIn.back = false;
                stepIn.secretDoor = false;
            }
            game.update(static_cast<float>(fixedStep), stepIn);
            firstStep = false;
            accumulator -= fixedStep;
            ++guard;
        }

        if (game.quitRequested) break;

        game.render();
        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

} // namespace kolek
