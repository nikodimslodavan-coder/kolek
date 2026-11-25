#pragma once

#include <SDL_pixels.h>
#include <cstdint>

namespace kolek {

struct RectF {
    float x = 0, y = 0, w = 0, h = 0;
};

struct Body {
    float x = 0, y = 0, w = 10, h = PLAYER_H;
    float vx = 0, vy = 0;
    bool  grounded = false;
    bool  climbing = false;
    bool  blockedX = false;
    int   facing = 1;
    int   airJumps = 0;
    float coyote = 0;
    float jumpBuf = 0;
    float ladderLock = 0;
    float anim = 0;
    float hurt = 0;
    float invuln = 0;
    int   lookY = 0;         // -1 — смотрит вверх, 0 — вперёд, 1 — вниз
};

struct Monster {
    int   type = 0;          // 0 — паук, 1 — браток с битой,
                             // 2 — браток с пистолетом, 3 — доберман
    Body  b;
    int   hp = 1, maxHp = 1;
    float anim = 0;
    float hurtFlash = 0;
    float homeX = 0, homeY = 0;
    float seed = 0;
    float turnTimer = 0;
    float throwCd = 0;       // браток с пистолетом: пауза до следующего выстрела
    // Паук: 0 — ползёт по потолку, 1 — спускается на паутине,
    // 2 — висит, 3 — поднимается обратно.
    int   webState = 0;
    float ceilingY = 0;
    float webAnchorX = 0, webAnchorY = 0, webTargetY = 0;
    float webTimer = 0;
    bool  dead = false;
};

struct Particle {
    float x = 0, y = 0, vx = 0, vy = 0;
    float life = 0, maxLife = 1;
    float size = 2, grav = 400;
    SDL_Color color{255, 255, 255, 255};
};

// Угасающий след дроби: отрезок, который быстро тает после выстрела.
struct Tracer {
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    float life = 0, maxLife = 0.2f;
    SDL_Color color{255, 230, 160, 255};
};

// Пуля братка с пистолетом: летит по прямой, но медленнее Коляна.
struct Bullet {
    float x = 0, y = 0;      // центр пули в мировых координатах
    float vx = 0;            // горизонтальная скорость
    int   dir = 1;           // куда летит
    float spin = 0;          // лёгкий след/мерцание
};

// Кубок-трофей: три штуки открывают парадную дверь, каждый даёт 1000 очков.
struct CupItem {
    int  tx = 0, ty = 0;
    bool taken = false;
};

// Украшение: случайно лежит на полу и приносит 200, 400 или 600 очков.
struct Jewel {
    float x = 0, y = 0;
    int   type = 0;
    bool  taken = false;
    float phase = 0;
};

// Всплывающие очки: короткая надпись поднимается над Коляном и тает.
struct ScorePopup {
    float x = 0, y = 0;
    int   value = 0;
    float life = 0, maxLife = 1.1f;
    SDL_Color color{255, 226, 150, 255};
};

struct Decor {
    int tx = 0, ty = 0, type = 0;
};

struct Light {
    float x = 0, y = 0, r = 60;
    SDL_Color color{255, 220, 140, 255};
    float alpha = 100;
};

enum DecorType {
    DEC_WINDOW, DEC_PAINTING, DEC_SCONCE, DEC_CHANDELIER,
    DEC_SHELF, DEC_MIRROR, DEC_CLOCK,
    // фурнитура технического этажа (второй уровень)
    DEC_EMERGENCY, DEC_STEPLADDER, DEC_PIPE, DEC_VENT, DEC_BOX,
    DEC_SIGN, DEC_CABLE
};

} // namespace kolek
