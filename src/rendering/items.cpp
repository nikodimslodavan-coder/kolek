#include "game/game.h"
#include "core/math.h"
#include "rendering/font.h"

namespace kolek {

void Game::drawDoor()
{
    if (door.w <= 0) return;
    // На парковке роль двери играет джип — его рисует фон.
    if (levelIndex == 3) return;
    // На техническом этаже выход — грузовой лифт.
    if (levelIndex == 1) {
        drawElevatorDoor();
        return;
    }
    int x = sx(door.x);
    int y = sy(door.y);
    int w = static_cast<int>(door.w);
    int h = static_cast<int>(door.h);
    if (x < -80 || x > VIEW_W + 40) return;

    SDL_SetRenderDrawColor(ren, 42, 26, 20, 255);
    SDL_Rect frame{x - 3, y - 3, w + 6, h + 6};
    SDL_RenderFillRect(ren, &frame);
    SDL_SetRenderDrawColor(ren, 78, 44, 28, 255);
    SDL_Rect doorRect{x, y, w, h};
    SDL_RenderFillRect(ren, &doorRect);

    // арка сверху
    SDL_SetRenderDrawColor(ren, 50, 30, 22, 255);
    SDL_Rect arch1{x + 2, y - 2, w - 4, 4};
    SDL_RenderFillRect(ren, &arch1);
    SDL_SetRenderDrawColor(ren, 150, 110, 58, 255);
    SDL_Rect archGold{x + 2, y + 1, w - 4, 2};
    SDL_RenderFillRect(ren, &archGold);

    // створки
    SDL_SetRenderDrawColor(ren, 112, 66, 38, 255);
    SDL_Rect left{x + 3, y + 6, w / 2 - 4, h - 10};
    SDL_Rect right{x + w / 2 + 1, y + 6, w / 2 - 4, h - 10};
    SDL_RenderFillRect(ren, &left);
    SDL_RenderFillRect(ren, &right);
    SDL_SetRenderDrawColor(ren, 66, 38, 24, 255);
    SDL_Rect seam{x + w / 2 - 1, y + 6, 2, h - 10};
    SDL_RenderFillRect(ren, &seam);

    // филёнки
    SDL_SetRenderDrawColor(ren, 138, 86, 46, 255);
    SDL_Rect p1{x + 6, y + 12, w / 2 - 10, 12};
    SDL_Rect p2{x + 6, y + 28, w / 2 - 10, 12};
    SDL_Rect p3{x + w / 2 + 4, y + 12, w / 2 - 10, 12};
    SDL_Rect p4{x + w / 2 + 4, y + 28, w / 2 - 10, 12};
    SDL_RenderDrawRect(ren, &p1);
    SDL_RenderDrawRect(ren, &p2);
    SDL_RenderDrawRect(ren, &p3);
    SDL_RenderDrawRect(ren, &p4);

    // ручки
    fillCircle(x + w / 2 - 5, y + h / 2 + 2, 2, SDL_Color{220, 178, 82, 255});
    fillCircle(x + w / 2 + 5, y + h / 2 + 2, 2, SDL_Color{220, 178, 82, 255});

    // три замка под кубки
    for (int i = 0; i < 3; ++i) {
        int ky = y + 15 + i * 9;
        bool lit = i < cupsGot;
        fillCircle(x + 8, ky, 2,
                   lit ? SDL_Color{255, 214, 90, 255} : SDL_Color{40, 26, 20, 255});
    }

    bool unlocked = (cupsGot >= static_cast<int>(cups.size()));
    if (unlocked) {
        float pulse = 0.5f + 0.5f * std::sin(timeSec * 3.2f);
        addLight(door.x + door.w * 0.5f, door.y + door.h * 0.5f,
                 90.0f, SDL_Color{255, 210, 110, 255}, 55.0f + 45.0f * pulse);
        SDL_SetRenderDrawColor(ren, 255, 226, 140,
                               static_cast<Uint8>(90 + pulse * 100));
        SDL_Rect glowLine{x + 4, y + 4, w - 8, 1};
        SDL_RenderFillRect(ren, &glowLine);
    }

    // табличка
    SDL_SetRenderDrawColor(ren, 176, 138, 66, 255);
    SDL_Rect sign{x + w / 2 - 12, y - 12, 24, 8};
    SDL_RenderFillRect(ren, &sign);
    SDL_SetRenderDrawColor(ren, 42, 28, 18, 255);
    SDL_RenderDrawLine(ren, x + w / 2 - 7, y - 8, x + w / 2 + 7, y - 8);
}

void Game::drawElevatorDoor()
{
    int x = sx(door.x);
    int y = sy(door.y);
    int w = static_cast<int>(door.w);
    int h = static_cast<int>(door.h);
    if (x < -80 || x > VIEW_W + 40) return;

    bool unlocked = (cupsGot >= static_cast<int>(cups.size()));

    // Тёмный стальной портал.
    SDL_SetRenderDrawColor(ren, 28, 32, 34, 255);
    SDL_Rect frame{x - 5, y - 6, w + 10, h + 12};
    SDL_RenderFillRect(ren, &frame);
    SDL_SetRenderDrawColor(ren, 70, 78, 80, 255);
    SDL_Rect frameIn{x - 3, y - 4, w + 6, h + 8};
    SDL_RenderFillRect(ren, &frameIn);
    SDL_SetRenderDrawColor(ren, 20, 22, 24, 255);
    SDL_Rect inner{x - 1, y - 2, w + 2, h + 4};
    SDL_RenderFillRect(ren, &inner);

    // Табло над створками: три лампы, загораются по кубкам.
    SDL_SetRenderDrawColor(ren, 24, 28, 30, 255);
    SDL_Rect header{x, y, w, 12};
    SDL_RenderFillRect(ren, &header);
    SDL_SetRenderDrawColor(ren, 48, 54, 56, 255);
    SDL_Rect headerIn{x + 1, y + 1, w - 2, 10};
    SDL_RenderFillRect(ren, &headerIn);
    for (int i = 0; i < 3; ++i) {
        int lx = x + w / 2 - 8 + i * 6;
        bool lit = i < cupsGot;
        SDL_SetRenderDrawColor(ren, lit ? 255 : 46,
                               lit ? 214 : 52, lit ? 90 : 54, 255);
        SDL_Rect lamp{lx, y + 4, 3, 3};
        SDL_RenderFillRect(ren, &lamp);
        SDL_SetRenderDrawColor(ren, 18, 20, 22, 255);
        SDL_Rect socket{lx - 1, y + 3, 5, 1};
        SDL_RenderFillRect(ren, &socket);
    }

    // Шахта за створками.
    int top = y + 14;
    int doorH = h - 18;
    SDL_SetRenderDrawColor(ren, 10, 9, 10, 255);
    SDL_Rect shaft{x + 2, top, w - 4, doorH};
    SDL_RenderFillRect(ren, &shaft);

    // Раздвижные створки: когда кубки собраны, щель между ними открыта.
    int gap = unlocked ? 4 : 0;
    int half = w / 2;
    int leafW = half - 3 - gap;
    if (unlocked) {
        SDL_SetRenderDrawColor(ren, 214, 170, 96, 70);
        SDL_Rect warm{x + half - 4, top + 2, 8, doorH - 4};
        SDL_RenderFillRect(ren, &warm);
    }
    SDL_Rect leaves[2] = {
        SDL_Rect{x + 2, top, leafW, doorH},
        SDL_Rect{x + half + 1 + gap, top, leafW, doorH}
    };
    for (int i = 0; i < 2; ++i) {
        const SDL_Rect& d = leaves[i];
        SDL_SetRenderDrawColor(ren, 92, 100, 102, 255);
        SDL_RenderFillRect(ren, &d);
        SDL_SetRenderDrawColor(ren, 126, 134, 136, 255);
        SDL_Rect hi{d.x + 2, d.y, 2, d.h};
        SDL_RenderFillRect(ren, &hi);
        SDL_SetRenderDrawColor(ren, 62, 68, 70, 255);
        SDL_Rect lo{d.x + d.w - 2, d.y, 2, d.h};
        SDL_RenderFillRect(ren, &lo);
        SDL_SetRenderDrawColor(ren, 48, 54, 56, 255);
        SDL_Rect topEdge{d.x, d.y, d.w, 2};
        SDL_Rect botEdge{d.x, d.y + d.h - 2, d.w, 2};
        SDL_RenderFillRect(ren, &topEdge);
        SDL_RenderFillRect(ren, &botEdge);
    }

    // Пульт вызова справа от портала.
    SDL_SetRenderDrawColor(ren, 30, 34, 36, 255);
    SDL_Rect panel{x + w + 3, y + h / 2 - 8, 9, 16};
    SDL_RenderFillRect(ren, &panel);
    SDL_SetRenderDrawColor(ren, 62, 68, 70, 255);
    SDL_Rect panelIn{x + w + 4, y + h / 2 - 7, 7, 14};
    SDL_RenderFillRect(ren, &panelIn);
    SDL_Color callGlow = unlocked ? SDL_Color{255, 214, 96, 255}
                                  : SDL_Color{110, 70, 48, 255};
    fillCircle(x + w + 7, y + h / 2 - 2, 2, callGlow);
    fillCircle(x + w + 7, y + h / 2 + 4, 2,
               SDL_Color{70, 76, 78, 255});

    // Табличка «ЛИФТ».
    SDL_SetRenderDrawColor(ren, 26, 30, 32, 255);
    SDL_Rect sign{x + w / 2 - 16, y - 16, 32, 11};
    SDL_RenderFillRect(ren, &sign);
    SDL_SetRenderDrawColor(ren, 180, 144, 56, 255);
    SDL_Rect signIn{x + w / 2 - 15, y - 15, 30, 9};
    SDL_RenderFillRect(ren, &signIn);
    pixfont::drawTextCentered(ren, x + w / 2, y - 14, "ЛИФТ", 1,
                              SDL_Color{38, 32, 24, 255}, false);

    if (unlocked) {
        float pulse = 0.5f + 0.5f * std::sin(timeSec * 3.2f);
        addLight(door.x + door.w * 0.5f, door.y + door.h * 0.5f,
                 96.0f, SDL_Color{255, 210, 120, 255}, 60.0f + 45.0f * pulse);
        SDL_SetRenderDrawColor(ren, 255, 226, 150,
                               static_cast<Uint8>(90 + pulse * 100));
        SDL_Rect glowLine{x + 3, top, w - 6, 1};
        SDL_RenderFillRect(ren, &glowLine);
    }
}

void Game::drawSecretDoor()
{
    if (!DEV_SECRET_LEVELS || !secretDoorActive) return;

    int x = sx(secretDoor.x);
    int y = sy(secretDoor.y);
    int w = static_cast<int>(secretDoor.w);
    int h = static_cast<int>(secretDoor.h);
    if (x < -80 || x > VIEW_W + 40) return;

    float pulse = 0.5f + 0.5f * std::sin(timeSec * 4.2f);

    // каменный портал
    SDL_SetRenderDrawColor(ren, 18, 26, 36, 255);
    SDL_Rect frame{x - 3, y - 3, w + 6, h + 6};
    SDL_RenderFillRect(ren, &frame);

    // светящийся проём
    for (int i = 0; i < 4; ++i) {
        int inset = 2 + i * 2;
        Uint8 a = static_cast<Uint8>(150 - i * 30 + pulse * 40);
        SDL_SetRenderDrawColor(ren, 70 + i * 25, 210 - i * 20,
                               235 - i * 15, a);
        SDL_Rect glow{x + inset, y + inset, w - inset * 2, h - inset * 2};
        SDL_RenderFillRect(ren, &glow);
    }

    // золотая окантовка
    SDL_SetRenderDrawColor(ren, 210, 176, 92, 255);
    SDL_Rect top{x + 2, y + 1, w - 4, 2};
    SDL_Rect left{x + 1, y + 4, 2, h - 6};
    SDL_Rect right{x + w - 3, y + 4, 2, h - 6};
    SDL_RenderFillRect(ren, &top);
    SDL_RenderFillRect(ren, &left);
    SDL_RenderFillRect(ren, &right);

    // ручка и знак секрета
    fillCircle(x + w / 2, y + h / 2, 2, SDL_Color{240, 230, 170, 255});
    fillCircle(x + w / 2, y + h / 2, 1, SDL_Color{80, 230, 245, 255});
    fillTriangle(x + w / 2, y - 12, x + w / 2 - 5, y - 6,
                 x + w / 2 + 5, y - 6, SDL_Color{90, 230, 245, 255});

    addLight(secretDoor.x + secretDoor.w * 0.5f,
             secretDoor.y + secretDoor.h * 0.5f, 70.0f,
             SDL_Color{80, 220, 240, 255}, 110.0f + 50.0f * pulse);
}

void Game::drawCups()
{
    for (const CupItem& c : cups) {
        if (c.taken) continue;
        float bob = std::sin(timeSec * 3.4f + c.tx * 0.7f) * 2.0f;
        float wx = c.tx * 16.0f + 2.0f;
        float wy = c.ty * 16.0f + 3.0f + bob;
        drawCupIcon(sx(wx), sy(wy), 0.0f);
        addLight(c.tx * 16.0f + 8.0f, c.ty * 16.0f + 8.0f, 30.0f,
                 SDL_Color{255, 210, 90, 255}, 50.0f);
    }
}

void Game::drawJewelry()
{
    for (const Jewel& jw : jewelry) {
        if (jw.taken) continue;
        if (jw.x + 16.0f < camX || jw.x > camX + VIEW_W + 16.0f) continue;
        if (jw.y + 16.0f < camY || jw.y > camY + VIEW_H + 16.0f) continue;

        float bob = std::sin(timeSec * 2.8f + jw.phase) * 1.6f;

        // тень на полу
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 60);
        SDL_Rect sh{sx(jw.x) + 1, sy(jw.y) + 11, 9, 2};
        SDL_RenderFillRect(ren, &sh);

        drawJewelIcon(sx(jw.x), sy(jw.y), jw.type, bob);

        SDL_Color glow = (jw.type == 0) ? SDL_Color{150, 205, 255, 255}
                       : (jw.type == 1) ? SDL_Color{130, 240, 160, 255}
                                        : SDL_Color{255, 210, 110, 255};
        addLight(jw.x + 5.0f, jw.y + 5.0f, 22.0f, glow, 34.0f);
    }
}

void Game::drawJewelIcon(int x, int y, int type, float bob)
{
    int yy = y + static_cast<int>(std::lround(bob));

    if (type == 0) {
        // серебряный перстень с голубым камнем
        fillCircle(x + 5, yy + 6, 4, SDL_Color{198, 204, 216, 255});
        fillCircle(x + 5, yy + 6, 2, SDL_Color{42, 36, 42, 255});
        SDL_SetRenderDrawColor(ren, 112, 198, 246, 255);
        SDL_Rect gem{x + 3, yy + 1, 5, 4};
        SDL_RenderFillRect(ren, &gem);
        SDL_SetRenderDrawColor(ren, 206, 240, 255, 230);
        SDL_Rect gl{x + 4, yy + 2, 2, 1};
        SDL_RenderFillRect(ren, &gl);
    } else if (type == 1) {
        // золотой кулон с изумрудом на цепочке
        SDL_SetRenderDrawColor(ren, 238, 206, 120, 200);
        SDL_RenderDrawLine(ren, x + 5, yy, x + 2, yy + 4);
        SDL_RenderDrawLine(ren, x + 5, yy, x + 8, yy + 4);
        fillCircle(x + 5, yy + 6, 4, SDL_Color{240, 190, 60, 255});
        fillTriangle(x + 5, yy + 3, x + 2, yy + 6, x + 8, yy + 6,
                     SDL_Color{80, 214, 128, 255});
        SDL_SetRenderDrawColor(ren, 220, 255, 230, 220);
        SDL_Rect gl{x + 4, yy + 4, 1, 2};
        SDL_RenderFillRect(ren, &gl);
    } else {
        // корона с рубином
        SDL_SetRenderDrawColor(ren, 246, 194, 62, 255);
        SDL_Rect base{x + 1, yy + 5, 9, 3};
        SDL_RenderFillRect(ren, &base);
        SDL_Rect p1{x + 1, yy + 2, 2, 3};
        SDL_Rect p2{x + 4, yy + 1, 3, 4};
        SDL_Rect p3{x + 8, yy + 2, 2, 3};
        SDL_RenderFillRect(ren, &p1);
        SDL_RenderFillRect(ren, &p2);
        SDL_RenderFillRect(ren, &p3);
        fillCircle(x + 5, yy + 3, 1, SDL_Color{232, 72, 82, 255});
        fillCircle(x + 2, yy + 7, 1, SDL_Color{110, 190, 255, 255});
        fillCircle(x + 8, yy + 7, 1, SDL_Color{110, 220, 140, 255});
        SDL_SetRenderDrawColor(ren, 255, 244, 190, 200);
        SDL_Rect gl{x + 2, yy + 5, 2, 1};
        SDL_RenderFillRect(ren, &gl);
    }
}

} // namespace kolek
