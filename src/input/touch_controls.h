#pragma once

#include "core/constants.h"
#include <SDL.h>

namespace kolek {

enum TouchButton {
    TB_LEFT = 0, TB_RIGHT, TB_UP, TB_DOWN, TB_JUMP, TB_SHOOT,
    TB_BACK, TB_RESTART, TB_COUNT
};

// Мультитач-состояние виртуальных кнопок. Кнопки живут в логических
// координатах 480x272 и рисуются поверх игры, поэтому они одинаково
// выглядят на любом телефоне и не зависят от плотности экрана.
struct TouchControls {
    bool enabled = false;
    bool mouseFallback = true;
    bool held[TB_COUNT] = {};
    bool pressed[TB_COUNT] = {};
    int  fingerButton[64] = {};
    SDL_Rect rect[TB_COUNT] = {};
    uint32_t visibleMask = 0;

    TouchControls()
    {
        for (int i = 0; i < 64; ++i) fingerButton[i] = -1;
        // Левая часть — крестовина.
        rect[TB_UP]    = SDL_Rect{ 66, 152, 44, 32 };
        rect[TB_LEFT]  = SDL_Rect{ 18, 188, 44, 42 };
        rect[TB_RIGHT] = SDL_Rect{118, 188, 44, 42 };
        rect[TB_DOWN]  = SDL_Rect{ 66, 234, 44, 28 };
        // Правая часть — прыжок и выстрел.
        rect[TB_JUMP]  = SDL_Rect{356, 178, 56, 60 };
        rect[TB_SHOOT] = SDL_Rect{418, 196, 48, 56 };
        // Служебные кнопки: «МЕНЮ» ниже панели патронов.
        rect[TB_BACK]    = SDL_Rect{438,  26, 34, 20 };
        rect[TB_RESTART] = SDL_Rect{184, 238, 112, 26 };
    }

    bool visible(int b) const
    {
        return (visibleMask & (1u << static_cast<unsigned>(b))) != 0;
    }

    void setVisibleForState(int state, bool confirmingExit = false)
    {
        if (confirmingExit) state = ST_TITLE;
        uint32_t m = 0;
        auto add = [&](int b) { m |= 1u << static_cast<unsigned>(b); };
        switch (state) {
            case ST_TITLE:
                add(TB_UP); add(TB_DOWN); add(TB_SHOOT); add(TB_BACK);
                break;
            case ST_SETTINGS:
                add(TB_UP); add(TB_DOWN); add(TB_LEFT); add(TB_RIGHT);
                add(TB_SHOOT); add(TB_BACK);
                break;
            case ST_SLIDE:
                add(TB_SHOOT); add(TB_BACK);
                break;
            case ST_WIN:
            case ST_GAMEOVER:
                add(TB_RESTART); add(TB_BACK);
                break;
            case ST_PLAY:
            case ST_DYING:
            default:
                add(TB_LEFT); add(TB_RIGHT); add(TB_UP); add(TB_DOWN);
                add(TB_JUMP); add(TB_SHOOT); add(TB_BACK);
                break;
        }
        visibleMask = m;
        // Кнопки, которые исчезли вместе с экраном, отпускаем.
        for (int i = 0; i < 64; ++i) {
            int b = fingerButton[i];
            if (b >= 0 && !visible(b)) fingerButton[i] = -1;
        }
    }

    int buttonAt(float lx, float ly) const
    {
        for (int b = 0; b < TB_COUNT; ++b) {
            if (!visible(b)) continue;
            const SDL_Rect& r = rect[b];
            if (lx >= r.x && lx < r.x + r.w &&
                ly >= r.y && ly < r.y + r.h)
                return b;
        }
        return -1;
    }

    void beginFrame()
    {
        for (int b = 0; b < TB_COUNT; ++b) pressed[b] = false;
    }

    void endEvents()
    {
        bool now[TB_COUNT] = {};
        for (int i = 0; i < 64; ++i) {
            int b = fingerButton[i];
            if (b >= 0 && visible(b)) now[b] = true;
        }
        for (int b = 0; b < TB_COUNT; ++b) held[b] = now[b];
    }

    void setFinger(int id, float lx, float ly)
    {
        if (id < 0 || id >= 64) return;
        int b = buttonAt(lx, ly);
        fingerButton[id] = b;
        // Нажатие засчитываем сразу, а не в конце кадра: быстрый тап
        // (down и up между двумя кадрами) иначе терялся бы.
        if (b >= 0 && !held[b]) pressed[b] = true;
    }

    // SDL_RenderSetLogicalSize не только вписывает кадр 480x272 в окно, но и
    // сам пересчитывает координаты касаний и мыши в систему логического кадра:
    // tfinger приходит в 0..1 по видимой области, mouse — сразу в пикселях
    // 480x272. Поэтому здесь никакого дополнительного масштабирования не нужно.
    void handleEvent(const SDL_Event& ev, int winW, int winH)
    {
        if (!enabled || winW <= 0 || winH <= 0) return;

        if (ev.type == SDL_FINGERDOWN || ev.type == SDL_FINGERMOTION ||
            ev.type == SDL_FINGERUP) {
            float lx = ev.tfinger.x * VIEW_W;
            float ly = ev.tfinger.y * VIEW_H;
            int id = static_cast<int>(ev.tfinger.fingerId);
            if (ev.type == SDL_FINGERUP) {
                if (id >= 0 && id < 64) fingerButton[id] = -1;
            } else {
                setFinger(id, lx, ly);
            }
            mouseFallback = false;
        } else if (mouseFallback &&
                   (ev.type == SDL_MOUSEBUTTONDOWN ||
                    ev.type == SDL_MOUSEMOTION ||
                    ev.type == SDL_MOUSEBUTTONUP)) {
            // На десктопе с --touch кнопки удобно проверять мышью.
            float lx = 0, ly = 0;
            if (ev.type == SDL_MOUSEMOTION) {
                if (!(ev.motion.state & SDL_BUTTON_LMASK)) return;
                lx = static_cast<float>(ev.motion.x);
                ly = static_cast<float>(ev.motion.y);
            } else {
                if (ev.type == SDL_MOUSEBUTTONUP) {
                    fingerButton[0] = -1;
                    return;
                }
                if (ev.button.button != SDL_BUTTON_LEFT) return;
                lx = static_cast<float>(ev.button.x);
                ly = static_cast<float>(ev.button.y);
            }
            setFinger(0, lx, ly);
        }
    }
};

} // namespace kolek
