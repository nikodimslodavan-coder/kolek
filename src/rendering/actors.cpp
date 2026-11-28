#include "game/game.h"
#include "core/math.h"
#include "rendering/font.h"

namespace kolek {

void Game::drawMonsters()
{
    for (const Monster& m : monsters) {
        if (m.type == 0) drawSpider(m);
        else if (m.type == 1) drawBatThug(m);
        else if (m.type == 2) drawPistolThug(m);
        else if (m.type == 4) drawBoss(m);
        else drawDoberman(m);
    }
}

void Game::drawSpider(const Monster& m)
{
    const Body& b = m.b;
    if (b.x + b.w < camX - 30.0f || b.x > camX + VIEW_W + 30.0f) return;
    if (b.y + b.h < camY - 30.0f || b.y > camY + VIEW_H + 30.0f) return;

    int x = sx(b.x);
    int y = sy(b.y);
    int w = static_cast<int>(std::lround(b.w));
    int h = static_cast<int>(std::lround(b.h));
    int cx = x + w / 2;

    if (b.grounded) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 80);
        SDL_Rect sh{x - 3, y + h - 1, w + 6, 2};
        SDL_RenderFillRect(ren, &sh);
    }

    // Паутина: от потолочного крепления к спускающемуся пауку.
    if (m.webState != 0) {
        int ax = sx(m.webAnchorX);
        int ay = sy(m.webAnchorY);
        int tx = cx;
        int ty = y + 2;
        SDL_SetRenderDrawColor(ren, 226, 228, 238, 165);
        SDL_RenderDrawLine(ren, ax, ay, tx, ty);
        SDL_SetRenderDrawColor(ren, 186, 190, 208, 95);
        SDL_RenderDrawLine(ren, ax + 1, ay, tx + 1, ty);
        SDL_Rect knot{ax - 1, ay - 1, 3, 2};
        SDL_SetRenderDrawColor(ren, 214, 218, 230, 190);
        SDL_RenderFillRect(ren, &knot);
    }

    // Восемь лап: на потолке они выгнуты вверх и держатся за перекрытие,
    // в полёте на паутине и на земле — свисают вниз.
    const bool onCeiling = (m.webState == 0);
    SDL_SetRenderDrawColor(ren, 42, 28, 38, 255);
    for (int side = 0; side < 2; ++side) {
        for (int i = 0; i < 4; ++i) {
            int phase = static_cast<int>(
                std::sin(m.anim * 7.0f + i * 1.6f + side * 1.1f) * 2.0f);
            int rootX = (side == 0) ? cx - 2 : cx + 2;
            int rootY = y + 5 + i * 2;
            int kneeX = (side == 0) ? x - 1 - i / 2 : x + w + 1 + i / 2;
            int kneeY = onCeiling ? rootY - 5 - (i % 2)
                                  : rootY - 4 - (i % 2);
            int footX = (side == 0) ? x - 4 - i : x + w + 4 + i;
            int footY = onCeiling ? y - 3 + phase / 2
                                  : y + h - 1 + phase + (i % 2);
            SDL_RenderDrawLine(ren, rootX, rootY, kneeX, kneeY);
            SDL_RenderDrawLine(ren, kneeX, kneeY, footX, footY);
            SDL_RenderDrawLine(ren, rootX, rootY + 1, kneeX, kneeY + 1);
            SDL_RenderDrawLine(ren, kneeX, kneeY + 1, footX, footY + 1);
        }
    }

    // брюшко с крестом
    float abdX = (b.facing > 0) ? b.x + 1.0f : b.x + b.w - 11.0f;
    fillW(abdX + 1.0f, b.y + 3.0f, 8.0f, b.h - 5.0f, SDL_Color{66, 38, 54, 255});
    fillW(abdX, b.y + 5.0f, 10.0f, b.h - 9.0f, SDL_Color{84, 48, 66, 255});
    fillW(abdX + 2.0f, b.y + 4.0f, 5.0f, 2.0f, SDL_Color{118, 72, 94, 255});
    fillW(abdX + 4.0f, b.y + 6.0f, 2.0f, b.h - 9.0f, SDL_Color{146, 58, 68, 210});
    fillW(abdX + 1.0f, b.y + 9.0f, 8.0f, 2.0f, SDL_Color{146, 58, 68, 210});

    // голова и хелицеры
    float hx = (b.facing > 0) ? b.x + b.w - 6.0f : b.x + 1.0f;
    fillW(hx, b.y + 4.0f, 6.0f, 5.0f, SDL_Color{102, 60, 80, 255});
    fillW(hx + 1.0f, b.y + 3.0f, 4.0f, 2.0f, SDL_Color{126, 78, 100, 255});
    float clX = (b.facing > 0) ? b.x + b.w - 1.0f : b.x - 2.0f;
    fillW(clX, b.y + 7.0f, 3.0f, 1.0f, SDL_Color{52, 34, 46, 255});

    // глаза
    int ex = (b.facing > 0) ? sx(hx + 3.0f) : sx(hx + 1.0f);
    SDL_SetRenderDrawColor(ren, 255, 74, 62, 255);
    SDL_Rect e1{ex, sy(b.y + 5.0f), 2, 2};
    SDL_Rect e2{ex + (b.facing > 0 ? -3 : 3), sy(b.y + 6.0f), 1, 1};
    SDL_RenderFillRect(ren, &e1);
    SDL_RenderFillRect(ren, &e2);
    addLight(b.x + b.w * 0.5f, b.y + 6.0f, 20.0f,
             SDL_Color{255, 100, 70, 255}, 18.0f);

    if (m.hurtFlash > 0.0f) {
        SDL_SetRenderDrawColor(ren, 255, 225, 225, 80);
        SDL_Rect flash{x + 2, y + 2, w - 4, h - 4};
        SDL_RenderFillRect(ren, &flash);
    }
}

void Game::drawBatThug(const Monster& m)
{
    const Body& b = m.b;
    if (b.x + b.w < camX - 40.0f || b.x > camX + VIEW_W + 40.0f) return;
    if (b.y + b.h < camY - 40.0f || b.y > camY + VIEW_H + 40.0f) return;

    int x = sx(b.x);
    int y = sy(b.y);
    int w = static_cast<int>(std::lround(b.w));
    int h = static_cast<int>(std::lround(b.h));
    int dir = b.facing;

    if (b.grounded) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 85);
        SDL_Rect sh{x - 3, y + h - 1, w + 6, 2};
        SDL_RenderFillRect(ren, &sh);
    }

    int step = (static_cast<int>(m.anim * 5.0f) % 2 == 0) ? 1 : 0;
    int bob = (static_cast<int>(m.anim * 3.0f) % 2 == 0) ? 0 : 1;

    // ноги в тёмных спортивных штанах
    fillW(b.x + 2.0f + step, b.y + 18.0f, 4.0f, 6.0f, SDL_Color{42, 44, 48, 255});
    fillW(b.x + 8.0f - step, b.y + 18.0f, 4.0f, 6.0f, SDL_Color{34, 36, 40, 255});
    fillW(b.x + 1.0f + step, b.y + 23.0f, 5.0f, 3.0f, SDL_Color{24, 23, 24, 255});
    fillW(b.x + 8.0f - step, b.y + 23.0f, 5.0f, 3.0f, SDL_Color{20, 19, 20, 255});

    // чёрная куртка и золотая цепь
    fillW(b.x + 1.0f, b.y + 8.0f, b.w - 2.0f, 10.0f, SDL_Color{52, 50, 58, 255});
    fillW(b.x + 1.0f, b.y + 8.0f, 2.0f, 10.0f, SDL_Color{36, 34, 42, 255});
    fillW(b.x + 3.0f, b.y + 16.0f, b.w - 6.0f, 2.0f, SDL_Color{30, 28, 34, 255});
    fillW(b.x + 5.0f, b.y + 8.0f, 5.0f, 1.0f, SDL_Color{196, 158, 68, 255});
    fillW(b.x + 6.0f, b.y + 9.0f, 3.0f, 1.0f, SDL_Color{150, 116, 46, 255});

    // голова, кепка и злой прищур
    fillW(b.x + 2.0f, b.y, 10.0f, 9.0f, SDL_Color{214, 168, 132, 255});
    fillW(b.x + 2.0f, b.y + 6.0f, 10.0f, 3.0f, SDL_Color{190, 144, 112, 255});
    fillW(b.x + 1.0f, b.y - 2.0f, 12.0f, 4.0f, SDL_Color{34, 32, 38, 255});
    fillW(b.x + (dir > 0 ? 2.0f : 9.0f), b.y - 3.0f, 3.0f, 2.0f,
          SDL_Color{30, 28, 34, 255});
    float eyeX = (dir > 0) ? b.x + b.w - 4.0f : b.x + 2.0f;
    fillW(eyeX, b.y + 3.0f, 2.0f, 2.0f, SDL_Color{28, 24, 26, 255});
    fillW(eyeX + (dir > 0 ? -3.0f : 3.0f), b.y + 4.0f, 1.0f, 1.0f,
          SDL_Color{70, 56, 50, 255});
    fillW(b.x + 5.0f, b.y + 7.0f, 4.0f, 1.0f, SDL_Color{120, 62, 60, 255});

    // рука с битой, занесённой вперёд
    float handX = (dir > 0) ? b.x + b.w + 1.0f : b.x - 5.0f;
    fillW((dir > 0) ? b.x + b.w - 2.0f : b.x - 6.0f, b.y + 10.0f,
          8.0f, 3.0f, SDL_Color{44, 42, 50, 255});
    fillW(handX, b.y + 9.0f + bob, 4.0f, 4.0f, SDL_Color{206, 160, 126, 255});

    const SDL_Color batDark{96, 58, 30, 255};
    const SDL_Color batWood{162, 106, 52, 255};
    const SDL_Color batLight{196, 140, 74, 255};
    if (dir > 0) {
        float bx = handX + 3.0f;
        fillW(bx, b.y + 10.0f + bob, 7.0f, 2.0f, batDark);
        fillW(bx + 6.0f, b.y + 8.0f + bob, 9.0f, 5.0f, batWood);
        fillW(bx + 6.0f, b.y + 8.0f + bob, 9.0f, 2.0f, batLight);
    } else {
        float bx = handX - 12.0f;
        fillW(bx + 5.0f, b.y + 10.0f + bob, 7.0f, 2.0f, batDark);
        fillW(bx - 3.0f, b.y + 8.0f + bob, 9.0f, 5.0f, batWood);
        fillW(bx - 3.0f, b.y + 8.0f + bob, 9.0f, 2.0f, batLight);
    }

    addLight(b.x + b.w * 0.5f, b.y + 8.0f, 22.0f,
             SDL_Color{200, 150, 90, 255}, 15.0f);

    if (m.hurtFlash > 0.0f) {
        SDL_SetRenderDrawColor(ren, 255, 235, 225, 85);
        SDL_Rect flash{x + 1, y + 1, w - 2, h - 2};
        SDL_RenderFillRect(ren, &flash);
    }
}

void Game::drawPistolThug(const Monster& m)
{
    const Body& b = m.b;
    if (b.x + b.w < camX - 40.0f || b.x > camX + VIEW_W + 40.0f) return;
    if (b.y + b.h < camY - 40.0f || b.y > camY + VIEW_H + 40.0f) return;

    int x = sx(b.x);
    int y = sy(b.y);
    int w = static_cast<int>(std::lround(b.w));
    int h = static_cast<int>(std::lround(b.h));
    int dir = b.facing;

    if (b.grounded) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 80);
        SDL_Rect sh{x - 2, y + h - 1, w + 4, 2};
        SDL_RenderFillRect(ren, &sh);
    }

    int step = (static_cast<int>(m.anim * 5.0f) % 2 == 0) ? 1 : 0;

    // ноги в спортивных штанах с лампасами
    fillW(b.x + 2.0f + step, b.y + 19.0f, 4.0f, 7.0f, SDL_Color{36, 38, 44, 255});
    fillW(b.x + 8.0f - step, b.y + 19.0f, 4.0f, 7.0f, SDL_Color{28, 30, 36, 255});
    fillW(b.x + 1.0f + step, b.y + 24.0f, 5.0f, 2.0f, SDL_Color{20, 18, 22, 255});
    fillW(b.x + 8.0f - step, b.y + 24.0f, 5.0f, 2.0f, SDL_Color{18, 16, 20, 255});
    fillW(b.x + 4.0f + step, b.y + 19.0f, 1.0f, 6.0f, SDL_Color{150, 148, 140, 255});
    fillW(b.x + 9.0f - step, b.y + 19.0f, 1.0f, 6.0f, SDL_Color{140, 138, 130, 255});

    // куртка
    fillW(b.x + 1.0f, b.y + 7.0f, b.w - 2.0f, 13.0f, SDL_Color{48, 48, 58, 255});
    fillW(b.x + 1.0f, b.y + 7.0f, 3.0f, 13.0f, SDL_Color{34, 34, 44, 255});
    fillW(b.x + 3.0f, b.y + 18.0f, 4.0f, 3.0f, SDL_Color{42, 42, 52, 255});
    fillW(b.x + 9.0f, b.y + 18.0f, 3.0f, 3.0f, SDL_Color{42, 42, 52, 255});

    // пояс и цепь
    fillW(b.x + 1.0f, b.y + 16.0f, b.w - 2.0f, 2.0f, SDL_Color{22, 20, 24, 255});
    fillW(b.x + 5.0f, b.y + 8.0f, 5.0f, 1.0f, SDL_Color{196, 158, 68, 255});

    // короткая стрижка и лицо
    fillW(b.x + 2.0f, b.y, 10.0f, 9.0f, SDL_Color{212, 166, 130, 255});
    fillW(b.x + 1.0f, b.y - 1.0f, 12.0f, 3.0f, SDL_Color{40, 34, 32, 255});
    fillW(b.x + (dir > 0 ? 2.0f : 9.0f), b.y + 1.0f, 3.0f, 1.0f,
          SDL_Color{226, 184, 146, 255});
    fillW(b.x + 4.0f, b.y + 7.0f, 5.0f, 1.0f, SDL_Color{120, 62, 60, 255});

    float eyeX = (dir > 0) ? b.x + b.w - 5.0f : b.x + 3.0f;
    fillW(eyeX, b.y + 4.0f, 2.0f, 2.0f, SDL_Color{30, 26, 24, 255});
    fillW(eyeX + (dir > 0 ? -3.0f : 3.0f), b.y + 5.0f, 1.0f, 1.0f,
          SDL_Color{72, 58, 50, 255});

    // вытянутая рука с пистолетом
    float handX = (dir > 0) ? b.x + b.w + 2.0f : b.x - 6.0f;
    fillW((dir > 0) ? b.x + b.w - 2.0f : b.x - 5.0f, b.y + 9.0f,
          7.0f, 3.0f, SDL_Color{56, 56, 66, 255});
    fillW(handX, b.y + 8.0f, 4.0f, 4.0f, SDL_Color{205, 160, 126, 255});
    float gunX = (dir > 0) ? handX + 4.0f : handX - 7.0f;
    fillW(gunX, b.y + 9.0f, 7.0f, 3.0f, SDL_Color{26, 26, 30, 255});
    fillW(gunX + (dir > 0 ? 2.0f : 1.0f), b.y + 12.0f, 2.0f, 2.0f,
          SDL_Color{34, 34, 38, 255});

    // вспышка сразу после выстрела
    if (m.throwCd > 1.55f) {
        float fx = (dir > 0) ? gunX + 7.0f : gunX - 3.0f;
        fillW(fx, b.y + 8.0f, 3.0f, 5.0f, SDL_Color{255, 220, 120, 220});
        fillW(fx + (dir > 0 ? 1.0f : -1.0f), b.y + 10.0f, 2.0f, 1.0f,
              SDL_Color{255, 246, 200, 230});
    }

    addLight((dir > 0) ? b.x + b.w + 8.0f : b.x - 8.0f, b.y + 9.0f,
             20.0f, SDL_Color{220, 150, 90, 255}, 22.0f);

    if (m.hurtFlash > 0.0f) {
        SDL_SetRenderDrawColor(ren, 255, 220, 220, 80);
        SDL_Rect flash{x + 1, y + 1, w - 2, h - 2};
        SDL_RenderFillRect(ren, &flash);
    }
}

void Game::drawDoberman(const Monster& m)
{
    const Body& b = m.b;
    if (b.x + b.w < camX - 40.0f || b.x > camX + VIEW_W + 40.0f) return;
    if (b.y + b.h < camY - 40.0f || b.y > camY + VIEW_H + 40.0f) return;

    int dir = b.facing;
    int step = (static_cast<int>(m.anim * 6.0f) % 2 == 0) ? 1 : 0;

    const SDL_Color fur{54, 42, 40, 255};
    const SDL_Color furLight{94, 72, 64, 255};
    const SDL_Color tan{158, 108, 62, 255};

    if (b.grounded) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 85);
        SDL_Rect sh{sx(b.x) - 3, sy(b.y) + static_cast<int>(b.h) - 1,
                    static_cast<int>(b.w) + 6, 2};
        SDL_RenderFillRect(ren, &sh);
    }

    // хвост торчком
    float rearX = (dir > 0) ? b.x + 1.0f : b.x + b.w - 3.0f;
    fillW(rearX, b.y - 3.0f, 2.0f, 4.0f, fur);
    fillW(rearX + (dir > 0 ? -2.0f : 2.0f), b.y - 6.0f, 3.0f, 3.0f, furLight);

    // четыре лапы
    fillW(b.x + 3.0f + step, b.y + b.h - 6.0f, 3.0f, 6.0f, fur);
    fillW(b.x + 8.0f - step, b.y + b.h - 6.0f, 3.0f, 6.0f, fur);
    fillW(b.x + b.w - 9.0f - step, b.y + b.h - 6.0f, 3.0f, 6.0f, fur);
    fillW(b.x + b.w - 4.0f + step, b.y + b.h - 6.0f, 3.0f, 6.0f, fur);

    // низкое вытянутое тело
    fillW(b.x + 2.0f, b.y + 2.0f, b.w - 4.0f, 8.0f, fur);
    fillW(b.x + 3.0f, b.y + 3.0f, b.w - 6.0f, 2.0f, furLight);
    fillW(b.x + 4.0f, b.y + 7.0f, b.w - 8.0f, 3.0f, furLight);
    fillW(b.x + (dir > 0 ? b.w - 9.0f : 5.0f), b.y + 4.0f, 4.0f, 5.0f, tan);
    // подпалины на груди и лапах
    fillW(b.x + (dir > 0 ? b.w - 11.0f : 8.0f), b.y + 5.0f, 3.0f, 4.0f, tan);
    fillW(b.x + 3.0f + step, b.y + b.h - 2.0f, 3.0f, 2.0f, tan);
    fillW(b.x + b.w - 4.0f + step, b.y + b.h - 2.0f, 3.0f, 2.0f, tan);

    // голова с ушами и намордником
    float hx = (dir > 0) ? b.x + b.w - 8.0f : b.x + 1.0f;
    fillW(hx, b.y, 9.0f, 8.0f, fur);
    fillW(hx + 1.0f, b.y - 3.0f, 3.0f, 3.0f, fur);
    fillW(hx + 1.0f, b.y - 4.0f, 2.0f, 1.0f, furLight);
    fillW(hx + 5.0f, b.y - 3.0f, 3.0f, 3.0f, fur);
    fillW(hx + 5.0f, b.y - 4.0f, 2.0f, 1.0f, furLight);
    fillW(hx + (dir > 0 ? 7.0f : 0.0f), b.y + 3.0f, 3.0f, 4.0f, tan);
    fillW(hx + (dir > 0 ? 9.0f : -1.0f), b.y + 4.0f, 1.0f, 2.0f,
          SDL_Color{24, 20, 22, 255});
    // красный ошейник — сразу выдаёт собаку в тёмном коридоре
    fillW((dir > 0) ? b.x + b.w - 10.0f : b.x + 2.0f, b.y + 1.0f,
          3.0f, 2.0f, SDL_Color{178, 42, 50, 255});

    // красный глаз и оскал
    float ex = (dir > 0) ? hx + 6.0f : hx + 1.0f;
    fillW(ex, b.y + 2.0f, 2.0f, 2.0f, SDL_Color{240, 60, 52, 255});
    fillW(hx + (dir > 0 ? 6.0f : 1.0f), b.y + 6.0f, 3.0f, 1.0f,
          SDL_Color{230, 220, 200, 255});

    addLight(b.x + b.w * 0.5f, b.y + 4.0f, 20.0f,
             SDL_Color{220, 80, 60, 255}, 14.0f);

    if (m.hurtFlash > 0.0f) {
        SDL_SetRenderDrawColor(ren, 255, 230, 225, 85);
        SDL_Rect flash{sx(b.x) + 1, sy(b.y) + 1,
                       static_cast<int>(b.w) - 2, static_cast<int>(b.h) - 2};
        SDL_RenderFillRect(ren, &flash);
    }
}

void Game::drawBoss(const Monster& m)
{
    const Body& b = m.b;
    if (b.x + b.w + 90.0f < camX || b.x - 90.0f > camX + VIEW_W) return;
    if (b.y + b.h < camY - 40.0f || b.y > camY + VIEW_H + 40.0f) return;

    const float sc = b.w / 80.0f;   // босс ровно в восемь Колянов
    const int dir = b.facing;
    const int step = (static_cast<int>(m.anim * 2.0f) % 2 == 0) ? 1 : 0;

    auto part = [&](float rx, float ry, float rw, float rh, SDL_Color c) {
        fillW(b.x + rx * sc, b.y + ry * sc, rw * sc, rh * sc, c);
    };

    const SDL_Color skin{214, 168, 132, 255};
    const SDL_Color skinShade{182, 138, 106, 255};
    const SDL_Color jacket{48, 46, 56, 255};
    const SDL_Color jacketDark{32, 30, 38, 255};
    const SDL_Color pants{40, 42, 50, 255};
    const SDL_Color shoe{22, 21, 24, 255};
    const SDL_Color gold{204, 166, 72, 255};
    const SDL_Color hair{36, 32, 30, 255};
    const SDL_Color cap{28, 28, 34, 255};
    const SDL_Color wood{150, 98, 48, 255};
    const SDL_Color woodLight{196, 140, 74, 255};

    // Тень под гигантом.
    part(-4.0f, 234.0f, 88.0f, 5.0f, SDL_Color{0, 0, 0, 90});

    // Ноги и ботинки.
    part(10.0f + step * 2.0f, 152.0f, 24.0f, 78.0f, pants);
    part(46.0f - step * 2.0f, 152.0f, 24.0f, 78.0f, pants);
    part(10.0f + step * 2.0f, 152.0f, 4.0f, 78.0f, jacketDark);
    part(46.0f - step * 2.0f, 152.0f, 4.0f, 78.0f, jacketDark);
    part(6.0f + step * 2.0f, 226.0f, 30.0f, 10.0f, shoe);
    part(44.0f - step * 2.0f, 226.0f, 30.0f, 10.0f, shoe);

    // Куртка, пояс и золотая цепь.
    part(4.0f, 52.0f, 72.0f, 104.0f, jacket);
    part(4.0f, 52.0f, 5.0f, 104.0f, jacketDark);
    part(70.0f, 52.0f, 6.0f, 104.0f, SDL_Color{58, 56, 66, 255});
    part(4.0f, 144.0f, 72.0f, 8.0f, jacketDark);
    part(4.0f, 54.0f, 72.0f, 3.0f, SDL_Color{66, 64, 76, 255});
    part(28.0f, 60.0f, 26.0f, 4.0f, gold);
    part(32.0f, 64.0f, 18.0f, 3.0f, SDL_Color{160, 124, 48, 255});
    part(38.0f, 67.0f, 6.0f, 8.0f, gold);
    part(38.0f, 75.0f, 6.0f, 3.0f, SDL_Color{236, 206, 96, 255});

    // Голова, шея и кепка.
    part(24.0f, 46.0f, 32.0f, 10.0f, skinShade);
    part(12.0f, 2.0f, 56.0f, 52.0f, skin);
    part(12.0f, 38.0f, 56.0f, 16.0f, skinShade);
    part(10.0f, -6.0f, 60.0f, 16.0f, hair);
    part(8.0f, -8.0f, 64.0f, 8.0f, cap);
    part(dir > 0 ? 54.0f : 4.0f, -4.0f, 22.0f, 6.0f, cap);
    // Ухо со стороны затылка.
    part(dir > 0 ? 10.0f : 62.0f, 18.0f, 8.0f, 16.0f, skinShade);
    part(dir > 0 ? 12.0f : 64.0f, 22.0f, 3.0f, 8.0f, hair);

    // Красные глаза, злой прищур и золотой зуб.
    float eyeX = (dir > 0) ? 44.0f : 22.0f;
    part(eyeX, 18.0f, 10.0f, 7.0f, SDL_Color{244, 66, 58, 255});
    part(eyeX + (dir > 0 ? -3.0f : 7.0f), 22.0f, 10.0f, 7.0f,
         SDL_Color{220, 44, 46, 255});
    part(eyeX, 15.0f, 14.0f, 3.0f, hair);
    part(eyeX, 25.0f, 12.0f, 2.0f, SDL_Color{120, 50, 48, 255});
    part(30.0f, 40.0f, 24.0f, 5.0f, SDL_Color{96, 44, 46, 255});
    part(40.0f, 40.0f, 4.0f, 4.0f, gold);

    // Руки: дальняя тёмная, ближняя держит биту.
    if (dir > 0) {
        part(-2.0f, 64.0f, 14.0f, 62.0f, jacketDark);
        part(62.0f, 66.0f, 18.0f, 54.0f, jacket);
        part(66.0f, 112.0f, 12.0f, 12.0f, skin);
    } else {
        part(68.0f, 64.0f, 14.0f, 62.0f, jacketDark);
        part(0.0f, 66.0f, 18.0f, 54.0f, jacket);
        part(2.0f, 112.0f, 12.0f, 12.0f, skin);
    }

    // Огромная бита.
    if (dir > 0) {
        part(68.0f, 96.0f, 22.0f, 7.0f, SDL_Color{96, 58, 30, 255});
        part(88.0f, 84.0f, 46.0f, 24.0f, wood);
        part(88.0f, 84.0f, 46.0f, 7.0f, woodLight);
        part(128.0f, 87.0f, 8.0f, 18.0f, woodLight);
    } else {
        part(-10.0f, 96.0f, 22.0f, 7.0f, SDL_Color{96, 58, 30, 255});
        part(-54.0f, 84.0f, 46.0f, 24.0f, wood);
        part(-54.0f, 84.0f, 46.0f, 7.0f, woodLight);
        part(-62.0f, 87.0f, 8.0f, 18.0f, woodLight);
    }

    // Тяжёлое дыхание и красная аура.
    int breath = (static_cast<int>(m.anim * 3.0f) % 2 == 0) ? 0 : 1;
    part(26.0f, 58.0f + breath, 28.0f, 2.0f, SDL_Color{70, 68, 80, 120});
    addLight(b.x + b.w * 0.5f, b.y + b.h * 0.4f, 78.0f,
             SDL_Color{190, 54, 66, 255}, 26.0f);
    if (!m.dead)
        addLight(b.x + (dir > 0 ? b.w - 14.0f : 14.0f),
                 b.y + 22.0f, 26.0f, SDL_Color{255, 70, 54, 255}, 22.0f);

    if (m.hurtFlash > 0.0f) {
        SDL_SetRenderDrawColor(ren, 255, 228, 220, 80);
        SDL_Rect flash{sx(b.x) + 1, sy(b.y) + 1,
                       static_cast<int>(b.w) - 2, static_cast<int>(b.h) - 2};
        SDL_RenderFillRect(ren, &flash);
    }
}

void Game::drawBullets()
{
    for (const Bullet& k : bullets) {
        int x = sx(k.x);
        int y = sy(k.y);
        if (x < -24 || x > VIEW_W + 24 || y < -24 || y > VIEW_H + 24) continue;
        int dir = k.dir;

        // след полёта
        SDL_SetRenderDrawColor(ren, 255, 214, 130, 90);
        SDL_RenderDrawLine(ren, x - dir * 14, y, x - dir * 4, y);
        SDL_SetRenderDrawColor(ren, 255, 238, 190, 150);
        SDL_RenderDrawLine(ren, x - dir * 8, y - 1, x - dir * 3, y - 1);

        // медная пуля с ярким сердечником
        SDL_SetRenderDrawColor(ren, 196, 146, 58, 255);
        SDL_Rect body{(dir > 0) ? x - 5 : x - 1, y - 2, 6, 4};
        SDL_RenderFillRect(ren, &body);
        if (dir > 0) {
            fillTriangle(x + 1, y - 2, x + 1, y + 2, x + 7, y,
                         SDL_Color{236, 196, 96, 255});
        } else {
            fillTriangle(x - 1, y - 2, x - 1, y + 2, x - 7, y,
                         SDL_Color{236, 196, 96, 255});
        }
        SDL_SetRenderDrawColor(ren, 255, 250, 214, 240);
        SDL_Rect shine{x - 2, y - 1, 3, 2};
        SDL_RenderFillRect(ren, &shine);

        addLight(k.x, k.y, 14.0f, SDL_Color{255, 200, 110, 255}, 14.0f);
    }
}

void Game::drawGrenades()
{
    for (const Grenade& g : grenades) {
        int x = sx(g.x);
        int y = sy(g.y);
        if (x < -24 || x > VIEW_W + 24 || y < -24 || y > VIEW_H + 24) continue;

        // Тёмный корпус с металлическим бликом и вращением.
        fillCircle(x, y, 3, SDL_Color{36, 52, 30, 255});
        fillCircle(x, y, 2, SDL_Color{58, 78, 44, 255});
        fillCircle(x - 1, y - 1, 1, SDL_Color{132, 154, 92, 220});

        // Ручка и мигающий красный запал.
        SDL_SetRenderDrawColor(ren, 92, 92, 98, 255);
        SDL_Rect lever{x + 1, y - 5, 2, 4};
        SDL_RenderFillRect(ren, &lever);
        bool blink = (std::fmod(g.fuse, 0.22f) < 0.11f);
        if (blink) {
            SDL_SetRenderDrawColor(ren, 255, 78, 62, 255);
            SDL_Rect cap{x - 1, y - 4, 2, 2};
            SDL_RenderFillRect(ren, &cap);
            addLight(g.x, g.y - 3.0f, 13.0f,
                     SDL_Color{255, 92, 64, 255}, 16.0f);
        } else {
            SDL_SetRenderDrawColor(ren, 132, 42, 38, 255);
            SDL_Rect cap{x - 1, y - 4, 2, 2};
            SDL_RenderFillRect(ren, &cap);
        }

        // Искры от тлеющего фитиля.
        float spark = std::fmod(g.fuse * 9.0f, 1.0f);
        fillW(g.x + 1.0f, g.y - 4.0f - spark * 3.0f, 1.0f, 1.0f,
              SDL_Color{255, 214, 120, 220});
    }
}

void Game::drawGuy(const Body& b, const Palette& p)
{
    if (b.invuln > 0 && b.hurt <= 0 && static_cast<int>(timeSec * 26.0f) % 2 == 0) return;

    int x = sx(b.x);
    int y = sy(b.y);
    int w = static_cast<int>(std::lround(b.w));
    int h = static_cast<int>(std::lround(b.h));
    if (x + w < -20 || x > VIEW_W + 20) return;
    if (y + h < -20 || y > VIEW_H + 20) return;

    bool walking = b.grounded && std::fabs(b.vx) > 8.0f && !b.climbing;
    int step = walking ? (static_cast<int>(b.anim) / 6) % 4 : 0;
    int swing = (step == 1) ? 1 : ((step == 3) ? -1 : 0);
    if (b.climbing) swing = (static_cast<int>(b.anim / 4.0f) % 2 == 0) ? 1 : -1;
    if (b.hurt > 0) swing = 0;

    // Вертикальные координаты — мировые: fillW сам переводит их в экранные.
    float bodyY = b.y + 8.0f;
    float bodyH = b.h - 18.0f;
    float legY = bodyY + bodyH;
    float feet = b.y + b.h;
    int look = b.lookY;
    float hy = b.y + static_cast<float>(look);  // голова чуть выше/ниже

    // тень
    if (b.grounded) {
        fillW(b.x - 2.0f, b.y + b.h - 1.0f, b.w + 4.0f, 2.0f,
              SDL_Color{0, 0, 0, 70});
    }

    // ноги и башмаки
    fillW(b.x + 1.0f + swing, legY, 3.0f, feet - legY - 2.0f, p.pants);
    fillW(b.x + b.w - 4.0f - swing, legY, 3.0f, feet - legY - 2.0f, p.pants);
    fillW(b.x + swing, feet - 3.0f, 4.0f, 3.0f, p.shoe);
    fillW(b.x + b.w - 4.0f - swing, feet - 3.0f, 4.0f, 3.0f, p.shoe);

    // туловище
    fillW(b.x, bodyY, b.w, bodyH, p.coat);
    fillW(b.x, bodyY, 2.0f, bodyH, p.coatDark);
    fillW(b.x, bodyY + bodyH - 2.0f, b.w, 2.0f,
          SDL_Color{38, 30, 30, 255});
    fillW(b.x + b.w - 2.0f, bodyY + 1.0f, 1.0f,
          bodyH - 3.0f, shade(p.coat, 1.18f));

    // руки
    int armOff = (step == 1) ? 1 : ((step == 3) ? -1 : 0);
    if (b.climbing) armOff = (swing > 0) ? -2 : 1;
    fillW(b.x - 2.0f, bodyY + 1.0f + armOff, 2.0f, 5.0f, p.coatDark);
    fillW(b.x + b.w, bodyY + 1.0f - armOff, 2.0f, 5.0f, p.coatDark);

    // шарф
    fillW(b.x - 1.0f, bodyY, b.w + 2.0f, 2.0f, p.accent);
    float tailX = (b.facing > 0) ? b.x - 3.0f : b.x + b.w + 1.0f;
    fillW(tailX, bodyY + 2.0f, 3.0f, 4.0f, p.accent);

    // голова
    fillW(b.x - 1.0f, hy, b.w + 2.0f, 7.0f, p.skin);
    fillW(b.x - 1.0f, hy - 1.0f, b.w + 2.0f, 3.0f, p.hair);
    float sideX = (b.facing > 0) ? b.x - 1.0f : b.x + b.w - 1.0f;
    fillW(sideX, hy + 1.0f, 2.0f, 2.0f, p.hair);
    // чубчик
    fillW(b.x + (b.facing > 0 ? b.w - 3.0f : 1.0f), hy - 2.0f,
          3.0f, 2.0f, p.hair);

    // глаз
    int eyeX = (b.facing > 0) ? x + w - 3 : x + 2;
    int eyeY = sy(hy + 4.0f);
    SDL_SetRenderDrawColor(ren, p.eye.r, p.eye.g, p.eye.b, 255);
    SDL_Rect eye{eyeX, eyeY, 1, 2};
    SDL_RenderFillRect(ren, &eye);
    // брови при взгляде вверх/вниз
    SDL_SetRenderDrawColor(ren, p.hair.r, p.hair.g, p.hair.b, 255);
    SDL_Rect brow{eyeX - 1, eyeY - 1, 3, 1};
    SDL_RenderFillRect(ren, &brow);

    // дробовик в руках: ствол поворачивается по диагонали вместе со взглядом
    float aimAng = b.lookY * AIM_DIAG;
    float ax = b.facing * std::cos(aimAng);
    float ay = std::sin(aimAng);
    float kick = (gunKick > 0.0f) ? (gunKick / 0.14f) * 1.5f : 0.0f;
    float shoulderX = b.x + b.w * 0.5f + b.facing * 1.0f - b.facing * kick * 0.4f;
    float shoulderY = b.y + 12.0f - ay * kick * 0.3f;
    if (!b.climbing && b.hurt <= 0) {
        if (b.lookY == 0) {
            float gx = (b.facing > 0) ? b.x + b.w - 1.0f : b.x - 7.0f;
            gx -= b.facing * kick;
            float gy = shoulderY - 1.5f;
            fillW(gx, gy, 8.0f, 3.0f, SDL_Color{62, 64, 72, 255});
            fillW(gx + (b.facing > 0 ? 6.0f : 0.0f), gy, 2.0f, 2.0f,
                  SDL_Color{126, 130, 142, 255});
            fillW(gx + (b.facing > 0 ? 1.0f : 4.0f), gy + 3.0f, 3.0f, 1.0f,
                  SDL_Color{112, 76, 42, 255});
            float stockX = (b.facing > 0) ? b.x + b.w - 4.0f : b.x - 1.0f;
            stockX -= b.facing * kick;
            fillW(stockX, gy + 1.0f, 4.0f, 3.0f, SDL_Color{104, 68, 36, 255});
        } else {
            for (int i = 0; i < 4; ++i) {
                float t = 1.0f + i * 2.1f;
                fillW(shoulderX + ax * t - 1.5f, shoulderY + ay * t - 1.5f,
                      3.0f, 3.0f, (i == 3 ? SDL_Color{126, 130, 142, 255}
                                         : SDL_Color{62, 64, 72, 255}));
            }
            fillW(shoulderX - ax * 2.2f - 1.0f, shoulderY - ay * 2.2f - 1.0f,
                  3.0f, 3.0f, SDL_Color{104, 68, 36, 255});
        }
    }

    // вспышка выстрела
    if (muzzle > 0.0f) {
        float tipX = shoulderX + ax * 9.0f;
        float tipY = shoulderY + ay * 9.0f;
        int mx = sx(tipX);
        int my = sy(tipY);
        int ex = sx(tipX + ax * 9.0f);
        int ey = sy(tipY + ay * 9.0f);
        int px = sx(tipX - ay * 5.0f);
        int py = sy(tipY + ax * 5.0f);
        int qx = sx(tipX + ay * 5.0f);
        int qy = sy(tipY - ax * 5.0f);
        fillCircle(mx, my, 4, SDL_Color{255, 190, 70, 210});
        fillCircle(mx, my, 2, SDL_Color{255, 250, 210, 245});
        SDL_SetRenderDrawColor(ren, 255, 210, 100, 190);
        SDL_RenderDrawLine(ren, mx, my, ex, ey);
        SDL_RenderDrawLine(ren, px, py, qx, qy);
    }
}

void Game::drawParticles()
{
    for (const Particle& p : parts) {
        float a = clampf(p.life / p.maxLife, 0.0f, 1.0f);
        if (p.x < camX - 10 || p.x > camX + VIEW_W + 10) continue;
        if (p.y < camY - 10 || p.y > camY + VIEW_H + 10) continue;
        SDL_SetRenderDrawColor(ren, p.color.r, p.color.g, p.color.b,
                               static_cast<Uint8>(p.color.a * a));
        SDL_Rect r{sx(p.x), sy(p.y),
                   static_cast<int>(std::max(1.0f, p.size)),
                   static_cast<int>(std::max(1.0f, p.size))};
        SDL_RenderFillRect(ren, &r);
    }
}

void Game::drawTracers()
{
    for (const Tracer& t : tracers) {
        float k = clampf(t.life / t.maxLife, 0.0f, 1.0f);
        if (k <= 0.0f) continue;

        int x0 = sx(t.x0), y0 = sy(t.y0);
        int x1 = sx(t.x1), y1 = sy(t.y1);
        if (std::max(x0, x1) < -8 || std::min(x0, x1) > VIEW_W + 8) continue;
        if (std::max(y0, y1) < -8 || std::min(y0, y1) > VIEW_H + 8) continue;

        float len = std::sqrt(static_cast<float>((x1 - x0) * (x1 - x0) +
                                                 (y1 - y0) * (y1 - y0)));
        float nx = 0.0f, ny = 0.0f;
        if (len > 0.01f) {
            nx = -(y1 - y0) / len;
            ny = (x1 - x0) / len;
        }

        // Мягкое свечение вокруг следа.
        Uint8 glow = static_cast<Uint8>(90.0f * k * k);
        SDL_SetRenderDrawColor(ren, t.color.r, t.color.g, t.color.b, glow);
        for (int o = -1; o <= 1; o += 2) {
            int ox = static_cast<int>(std::lround(nx * o));
            int oy = static_cast<int>(std::lround(ny * o));
            SDL_RenderDrawLine(ren, x0 + ox, y0 + oy, x1 + ox, y1 + oy);
        }

        // Яркое ядро, тающее к концу жизни.
        Uint8 a = static_cast<Uint8>(245.0f * k * k);
        SDL_SetRenderDrawColor(ren, t.color.r, t.color.g, t.color.b, a);
        SDL_RenderDrawLine(ren, x0, y0, x1, y1);
    }
}

void Game::drawGlows()
{
    addLight(player.x + player.w * 0.5f, player.y + player.h * 0.6f,
             46.0f, SDL_Color{255, 200, 140, 255}, 24.0f);
    if (muzzle > 0.0f) {
        float aimAng = player.lookY * AIM_DIAG;
        float dirX = player.facing * std::cos(aimAng);
        float dirY = std::sin(aimAng);
        float tipX = player.x + player.w * 0.5f + player.facing * 1.0f + dirX * 9.0f;
        float tipY = player.y + 12.0f + dirY * 9.0f;
        addLight(tipX, tipY, 52.0f, SDL_Color{255, 190, 80, 255},
                 130.0f * clampf(muzzle / 0.10f, 0.0f, 1.0f));
    }

    for (const Light& L : lights) {
        if (L.x + L.r < camX || L.x - L.r > camX + VIEW_W) continue;
        if (L.y + L.r < camY || L.y - L.r > camY + VIEW_H) continue;
        int r = static_cast<int>(L.r);
        SDL_Rect dst{sx(L.x) - r, sy(L.y) - r, r * 2, r * 2};
        SDL_SetTextureColorMod(glowTex, L.color.r, L.color.g, L.color.b);
        SDL_SetTextureAlphaMod(glowTex,
                               static_cast<Uint8>(clampf(L.alpha, 0.0f, 255.0f)));
        SDL_RenderCopy(ren, glowTex, nullptr, &dst);
    }
    SDL_SetTextureColorMod(glowTex, 255, 255, 255);
    SDL_SetTextureAlphaMod(glowTex, 255);
}

void Game::drawVignette()
{
    SDL_RenderCopy(ren, vignetteTex, nullptr, nullptr);
}

void Game::drawScorePopups()
{
    for (const ScorePopup& p : popups) {
        float fade = clampf(p.life / 0.35f, 0.0f, 1.0f);
        std::string txt = "+" + std::to_string(p.value);
        int tx = sx(p.x) - pixfont::textWidth(txt, 1) / 2;
        int ty = sy(p.y);
        SDL_SetRenderDrawColor(ren, 12, 8, 12, static_cast<Uint8>(180.0f * fade));
        pixfont::drawTextRaw(ren, tx + 1, ty + 1, txt, 1);
        SDL_SetRenderDrawColor(ren, p.color.r, p.color.g, p.color.b,
                               static_cast<Uint8>(255.0f * fade));
        pixfont::drawTextRaw(ren, tx, ty, txt, 1);
    }
}

} // namespace kolek
