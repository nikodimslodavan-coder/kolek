#include "game/game.h"
#include "core/math.h"
#include "rendering/font.h"

namespace kolek {

void Game::drawDecor()
{
    for (const Decor& d : decor) {
        float wx = d.tx * 16.0f;
        float wy = d.ty * 16.0f;
        if (wx < camX - 80 || wx > camX + VIEW_W + 80) continue;
        if (wy < camY - 120 || wy > camY + VIEW_H + 80) continue;
        switch (d.type) {
            case DEC_WINDOW:      drawWindow(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_PAINTING:    drawPainting(static_cast<int>(wx), static_cast<int>(wy),
                                               (d.tx * 7 + d.ty) % 3); break;
            case DEC_SCONCE:      drawSconce(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_CHANDELIER:  drawChandelier(static_cast<int>(wx) + 8,
                                                 static_cast<int>(wy) + 16); break;
            case DEC_SHELF:       drawShelf(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_MIRROR:      drawMirror(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_CLOCK:       drawClock(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_EMERGENCY:   drawEmergencyLamp(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_STEPLADDER:  drawStepLadder(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_PIPE:        drawTechPipes(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_VENT:        drawTechVent(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_BOX:         drawTechBox(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_SIGN:        drawTechSign(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_CABLE:       drawCableCoil(static_cast<int>(wx), static_cast<int>(wy)); break;
            default: break;
        }
    }
}

void Game::drawWindow(int wx, int wy)
{
    int x = sx(static_cast<float>(wx)) + 2;
    int y = sy(static_cast<float>(wy));
    if (x < -48 || x > VIEW_W + 16) return;

    // рама
    SDL_SetRenderDrawColor(ren, 48, 32, 24, 255);
    SDL_Rect frame{x - 3, y - 2, 36, 52};
    SDL_RenderFillRect(ren, &frame);
    SDL_SetRenderDrawColor(ren, 94, 64, 40, 255);
    SDL_Rect innerFrame{x - 1, y, 32, 48};
    SDL_RenderFillRect(ren, &innerFrame);

    // ночное небо
    SDL_SetRenderDrawColor(ren, 18, 24, 52, 255);
    SDL_Rect sky{x, y, 30, 46};
    SDL_RenderFillRect(ren, &sky);
    SDL_SetRenderDrawColor(ren, 32, 40, 78, 255);
    SDL_Rect sky2{x, y + 28, 30, 18};
    SDL_RenderFillRect(ren, &sky2);
    SDL_SetRenderDrawColor(ren, 54, 58, 96, 255);
    SDL_Rect sky3{x, y + 38, 30, 8};
    SDL_RenderFillRect(ren, &sky3);

    // луна и звёзды
    fillCircle(x + 21, y + 12, 4, SDL_Color{236, 232, 198, 235});
    fillCircle(x + 23, y + 10, 3, SDL_Color{24, 32, 68, 255});
    SDL_SetRenderDrawColor(ren, 220, 226, 245, 220);
    SDL_Rect st1{x + 6, y + 8, 1, 1};
    SDL_Rect st2{x + 12, y + 16, 1, 1};
    SDL_Rect st3{x + 25, y + 23, 1, 1};
    SDL_RenderFillRect(ren, &st1);
    SDL_RenderFillRect(ren, &st2);
    SDL_RenderFillRect(ren, &st3);

    // силуэт деревьев
    SDL_SetRenderDrawColor(ren, 12, 18, 24, 255);
    fillTriangle(x + 2, y + 46, x + 8, y + 32, x + 14, y + 46, SDL_Color{12, 18, 24, 255});
    fillTriangle(x + 12, y + 46, x + 20, y + 30, x + 27, y + 46, SDL_Color{10, 16, 22, 255});

    // переплёт
    SDL_SetRenderDrawColor(ren, 74, 50, 32, 255);
    SDL_Rect crossV{x + 14, y, 2, 46};
    SDL_Rect crossH{x, y + 22, 30, 2};
    SDL_RenderFillRect(ren, &crossV);
    SDL_RenderFillRect(ren, &crossH);

    // шторы
    SDL_SetRenderDrawColor(ren, 104, 32, 40, 255);
    SDL_Rect curtL{x - 3, y - 1, 6, 47};
    SDL_Rect curtR{x + 27, y - 1, 6, 47};
    SDL_RenderFillRect(ren, &curtL);
    SDL_RenderFillRect(ren, &curtR);
    SDL_SetRenderDrawColor(ren, 138, 48, 52, 255);
    SDL_Rect curtShine{x - 2, y, 1, 45};
    SDL_RenderFillRect(ren, &curtShine);

    // подоконник
    SDL_SetRenderDrawColor(ren, 120, 84, 52, 255);
    SDL_Rect sill{x - 5, y + 48, 40, 4};
    SDL_RenderFillRect(ren, &sill);
    SDL_SetRenderDrawColor(ren, 66, 44, 30, 255);
    SDL_Rect sillShade{x - 5, y + 52, 40, 2};
    SDL_RenderFillRect(ren, &sillShade);

    addLight(wx + 17.0f, wy + 24.0f, 52.0f, SDL_Color{120, 160, 255, 255}, 26.0f);
}

void Game::drawPainting(int wx, int wy, int variant)
{
    int x = sx(static_cast<float>(wx));
    int y = sy(static_cast<float>(wy));
    if (x < -40 || x > VIEW_W + 20) return;

    SDL_SetRenderDrawColor(ren, 74, 50, 26, 255);
    SDL_Rect outer{x - 2, y - 2, 32, 38};
    SDL_RenderFillRect(ren, &outer);
    SDL_SetRenderDrawColor(ren, 172, 132, 62, 255);
    SDL_Rect gold{x, y, 28, 34};
    SDL_RenderFillRect(ren, &gold);
    SDL_SetRenderDrawColor(ren, 26, 18, 22, 255);
    SDL_Rect canvas{x + 3, y + 3, 22, 28};
    SDL_RenderFillRect(ren, &canvas);

    if (variant == 0) {
        // портрет предка
        fillCircle(x + 14, y + 12, 5, SDL_Color{196, 164, 132, 255});
        SDL_SetRenderDrawColor(ren, 56, 40, 44, 255);
        SDL_Rect body{x + 6, y + 18, 16, 11};
        SDL_RenderFillRect(ren, &body);
        SDL_SetRenderDrawColor(ren, 232, 210, 176, 255);
        SDL_Rect face{x + 12, y + 10, 4, 4};
        SDL_RenderFillRect(ren, &face);
    } else if (variant == 1) {
        // ночной пейзаж
        SDL_SetRenderDrawColor(ren, 34, 44, 66, 255);
        SDL_Rect sky{x + 3, y + 3, 22, 28};
        SDL_RenderFillRect(ren, &sky);
        fillCircle(x + 18, y + 9, 3, SDL_Color{220, 220, 198, 255});
        fillTriangle(x + 2, y + 28, x + 10, y + 14, x + 18, y + 28,
                     SDL_Color{20, 28, 34, 255});
        fillTriangle(x + 11, y + 28, x + 18, y + 18, x + 26, y + 28,
                     SDL_Color{16, 24, 30, 255});
    } else {
        // цветы
        SDL_SetRenderDrawColor(ren, 52, 66, 50, 255);
        SDL_Rect vase{x + 10, y + 20, 9, 8};
        SDL_RenderFillRect(ren, &vase);
        SDL_SetRenderDrawColor(ren, 132, 170, 108, 255);
        SDL_Rect stem1{x + 13, y + 12, 1, 9};
        SDL_Rect stem2{x + 16, y + 14, 1, 7};
        SDL_RenderFillRect(ren, &stem1);
        SDL_RenderFillRect(ren, &stem2);
        fillCircle(x + 13, y + 11, 2, SDL_Color{196, 84, 92, 255});
        fillCircle(x + 17, y + 13, 2, SDL_Color{226, 196, 96, 255});
    }

    SDL_SetRenderDrawColor(ren, 214, 178, 96, 150);
    SDL_Rect glare{x + 1, y + 1, 1, 32};
    SDL_RenderFillRect(ren, &glare);
}

void Game::drawSconce(int wx, int wy)
{
    int x = sx(static_cast<float>(wx)) + 8;
    int y = sy(static_cast<float>(wy));
    if (x < -24 || x > VIEW_W + 24) return;

    float flick = 0.75f + 0.25f * std::sin(timeSec * 8.0f + wx * 0.8f) *
                                   std::sin(timeSec * 13.0f + wx * 0.31f);
    SDL_SetRenderDrawColor(ren, 52, 40, 30, 255);
    SDL_Rect bracket{x - 4, y + 14, 8, 2};
    SDL_Rect stem{x - 1, y + 10, 2, 6};
    SDL_RenderFillRect(ren, &bracket);
    SDL_RenderFillRect(ren, &stem);
    SDL_SetRenderDrawColor(ren, 226, 208, 160, 255);
    SDL_Rect candle{x - 2, y + 2, 4, 9};
    SDL_RenderFillRect(ren, &candle);
    SDL_SetRenderDrawColor(ren, 168, 136, 80, 255);
    SDL_Rect holder{x - 3, y + 10, 6, 2};
    SDL_RenderFillRect(ren, &holder);

    SDL_SetRenderDrawColor(ren, 255, 170, 60, 255);
    SDL_Rect flame{x - 1, y - 2 + static_cast<int>(flick * 1.5f), 2, 4};
    SDL_RenderFillRect(ren, &flame);
    SDL_SetRenderDrawColor(ren, 255, 240, 170, 255);
    SDL_Rect flameCore{x - 1, y + 1, 1, 2};
    SDL_RenderFillRect(ren, &flameCore);

    addLight(wx + 8.0f, wy + 6.0f, 58.0f, SDL_Color{255, 180, 90, 255},
             70.0f * flick);
    addLight(wx + 8.0f, wy + 10.0f, 18.0f, SDL_Color{255, 230, 160, 255},
             90.0f * flick);
}

void Game::drawChandelier(int wx, int wy)
{
    int x = sx(static_cast<float>(wx));
    int y = sy(static_cast<float>(wy));
    if (x < -48 || x > VIEW_W + 48) return;

    float flick = 0.82f + 0.18f * std::sin(timeSec * 5.5f + wx * 0.4f);
    SDL_SetRenderDrawColor(ren, 96, 72, 34, 255);
    SDL_Rect chain{x - 1, y, 2, 9};
    SDL_RenderFillRect(ren, &chain);

    SDL_SetRenderDrawColor(ren, 176, 132, 54, 255);
    SDL_Rect bar{x - 14, y + 9, 28, 3};
    SDL_RenderFillRect(ren, &bar);
    fillCircle(x, y + 9, 3, SDL_Color{206, 162, 70, 255});

    for (int i = -1; i <= 1; ++i) {
        int cx = x + i * 10;
        SDL_SetRenderDrawColor(ren, 118, 86, 38, 255);
        SDL_Rect stem{cx - 1, y + 6, 2, 6};
        SDL_RenderFillRect(ren, &stem);
        SDL_SetRenderDrawColor(ren, 232, 214, 168, 255);
        SDL_Rect candle{cx - 2, y + 12, 4, 7};
        SDL_RenderFillRect(ren, &candle);
        SDL_SetRenderDrawColor(ren, 255, 176, 60, 255);
        SDL_Rect flame{cx - 1, y + 8 + static_cast<int>(flick * 1.5f), 2, 4};
        SDL_RenderFillRect(ren, &flame);
    }

    SDL_SetRenderDrawColor(ren, 214, 172, 84, 200);
    SDL_Rect shine{x - 12, y + 9, 8, 1};
    SDL_RenderFillRect(ren, &shine);

    addLight(wx, wy + 18.0f, 108.0f, SDL_Color{255, 190, 100, 255}, 60.0f * flick);
    addLight(wx, wy + 18.0f, 34.0f, SDL_Color{255, 230, 170, 255}, 85.0f * flick);
}

void Game::drawShelf(int wx, int wy)
{
    int x = sx(static_cast<float>(wx));
    // Шкаф — подвесной: поднимаем его над головой Коляна, чтобы он не
    // «застревал» в шкафу и мог свободно проходить под ним.
    int y = sy(static_cast<float>(wy)) - 32;
    if (x < -56 || x > VIEW_W + 16) return;

    SDL_SetRenderDrawColor(ren, 48, 30, 22, 255);
    SDL_Rect body{x, y, 48, 48};
    SDL_RenderFillRect(ren, &body);
    SDL_SetRenderDrawColor(ren, 82, 52, 32, 255);
    SDL_Rect inner{x + 3, y + 3, 42, 42};
    SDL_RenderFillRect(ren, &inner);

    for (int shelf = 0; shelf < 3; ++shelf) {
        int sy0 = y + 8 + shelf * 13;
        SDL_SetRenderDrawColor(ren, 42, 26, 18, 255);
        SDL_Rect shelfLine{x + 3, sy0 + 8, 42, 2};
        SDL_RenderFillRect(ren, &shelfLine);
        for (int b = 0; b < 7; ++b) {
            uint32_t h = hash2(wx + b, wy + shelf);
            int bw = 3 + static_cast<int>(h % 3);
            SDL_Color cols[5] = {
                {126, 48, 48, 255}, {52, 86, 62, 255}, {60, 66, 112, 255},
                {132, 102, 52, 255}, {92, 58, 84, 255}
            };
            SDL_Color c = cols[(h >> 4) % 5];
            SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
            SDL_Rect book{x + 5 + b * 5, sy0 + 1, bw, 7};
            SDL_RenderFillRect(ren, &book);
        }
    }
    SDL_SetRenderDrawColor(ren, 110, 76, 44, 255);
    SDL_Rect top{x, y, 48, 3};
    SDL_RenderFillRect(ren, &top);
}

void Game::drawMirror(int wx, int wy)
{
    int x = sx(static_cast<float>(wx));
    int y = sy(static_cast<float>(wy));
    if (x < -32 || x > VIEW_W + 16) return;

    SDL_SetRenderDrawColor(ren, 64, 44, 24, 255);
    SDL_Rect frame{x, y, 22, 34};
    SDL_RenderFillRect(ren, &frame);
    SDL_SetRenderDrawColor(ren, 168, 128, 62, 255);
    SDL_Rect gold{x + 2, y + 2, 18, 30};
    SDL_RenderFillRect(ren, &gold);
    SDL_SetRenderDrawColor(ren, 60, 82, 102, 255);
    SDL_Rect glass{x + 4, y + 4, 14, 26};
    SDL_RenderFillRect(ren, &glass);
    SDL_SetRenderDrawColor(ren, 108, 142, 168, 190);
    SDL_Rect shine{x + 5, y + 6, 3, 18};
    SDL_RenderFillRect(ren, &shine);
    SDL_SetRenderDrawColor(ren, 196, 214, 232, 180);
    SDL_Rect shine2{x + 9, y + 8, 1, 9};
    SDL_RenderFillRect(ren, &shine2);
    SDL_SetRenderDrawColor(ren, 116, 80, 40, 255);
    SDL_Rect base{x - 2, y + 34, 26, 3};
    SDL_RenderFillRect(ren, &base);
}

void Game::drawClock(int wx, int wy)
{
    int x = sx(static_cast<float>(wx));
    int y = sy(static_cast<float>(wy));
    if (x < -40 || x > VIEW_W + 16) return;

    SDL_SetRenderDrawColor(ren, 56, 34, 24, 255);
    SDL_Rect body{x, y, 22, 48};
    SDL_RenderFillRect(ren, &body);
    SDL_SetRenderDrawColor(ren, 96, 62, 38, 255);
    SDL_Rect inner{x + 3, y + 3, 16, 42};
    SDL_RenderFillRect(ren, &inner);
    fillCircle(x + 11, y + 12, 7, SDL_Color{186, 160, 108, 255});
    fillCircle(x + 11, y + 12, 5, SDL_Color{238, 226, 190, 255});
    SDL_SetRenderDrawColor(ren, 38, 28, 22, 255);
    SDL_RenderDrawLine(ren, x + 11, y + 12, x + 11, y + 8);
    SDL_RenderDrawLine(ren, x + 11, y + 12, x + 14, y + 14);
    SDL_SetRenderDrawColor(ren, 150, 120, 60, 255);
    SDL_Rect pendulum{x + 10, y + 22, 2, 14};
    SDL_RenderFillRect(ren, &pendulum);
    fillCircle(x + 11, y + 37, 3, SDL_Color{186, 148, 70, 255});
    SDL_SetRenderDrawColor(ren, 124, 88, 46, 255);
    SDL_Rect top{x - 2, y - 2, 26, 3};
    SDL_RenderFillRect(ren, &top);
}

// ---------------------------------------------------------------------------
//  Фурнитура технического этажа
// ---------------------------------------------------------------------------
void Game::drawEmergencyLamp(int wx, int wy)
{
    int x = sx(static_cast<float>(wx)) + 8;
    int y = sy(static_cast<float>(wy));
    if (x < -24 || x > VIEW_W + 24) return;

    // Кронштейн под перекрытием и защитная сетка.
    SDL_SetRenderDrawColor(ren, 58, 60, 64, 255);
    SDL_Rect plate{x - 6, y - 2, 12, 3};
    SDL_Rect stem{x - 1, y + 1, 2, 4};
    SDL_RenderFillRect(ren, &plate);
    SDL_RenderFillRect(ren, &stem);

    SDL_SetRenderDrawColor(ren, 74, 76, 80, 255);
    SDL_Rect body{x - 5, y + 5, 10, 8};
    SDL_RenderFillRect(ren, &body);
    // Железный козырёк, чтобы лампа читалась как аварийная.
    SDL_SetRenderDrawColor(ren, 50, 52, 56, 255);
    SDL_Rect hood{x - 7, y + 3, 14, 3};
    SDL_RenderFillRect(ren, &hood);
    SDL_SetRenderDrawColor(ren, 100, 104, 108, 255);
    SDL_Rect hoodTop{x - 7, y + 3, 14, 1};
    SDL_RenderFillRect(ren, &hoodTop);
    SDL_SetRenderDrawColor(ren, 34, 36, 40, 255);
    for (int i = -4; i <= 4; i += 2) {
        SDL_Rect bar{x + i, y + 4, 1, 10};
        SDL_RenderFillRect(ren, &bar);
    }
    SDL_Rect cap{x - 6, y + 4, 12, 1};
    SDL_Rect cap2{x - 6, y + 13, 12, 1};
    SDL_RenderFillRect(ren, &cap);
    SDL_RenderFillRect(ren, &cap2);

    // Мигающая красная лампа.
    float wave = 0.5f + 0.5f * std::sin(timeSec * 4.6f + wx * 0.05f);
    float pulse = 0.25f + 0.75f * wave * wave;
    fillCircle(x, y + 9, 3, SDL_Color{216, 54, 44,
                                      static_cast<Uint8>(130 + 120 * pulse)});
    fillCircle(x, y + 9, 1, SDL_Color{255, 190, 160,
                                      static_cast<Uint8>(110 + 140 * pulse)});

    addLight(wx + 8.0f, wy + 10.0f, 56.0f, SDL_Color{255, 66, 48, 255},
             45.0f + 55.0f * pulse);
}

void Game::drawStepLadder(int wx, int wy)
{
    int x = sx(static_cast<float>(wx)) + 8;
    int y = sy(static_cast<float>(wy));
    if (x < -24 || x > VIEW_W + 24) return;

    // Задняя опора и передние стойки.
    SDL_SetRenderDrawColor(ren, 126, 88, 42, 255);
    SDL_RenderDrawLine(ren, x + 7, y + 2, x + 11, y + 30);
    SDL_SetRenderDrawColor(ren, 156, 110, 50, 255);
    SDL_RenderDrawLine(ren, x - 6, y + 2, x - 8, y + 30);
    SDL_RenderDrawLine(ren, x - 5, y + 2, x - 7, y + 30);

    // Ступени.
    SDL_SetRenderDrawColor(ren, 182, 132, 60, 255);
    for (int i = 0; i < 4; ++i) {
        int sy0 = y + 7 + i * 6;
        SDL_Rect step{x - 6 + i / 2, sy0, 13 - i / 2, 2};
        SDL_RenderFillRect(ren, &step);
    }

    // Шарнир и резиновые башмаки.
    SDL_SetRenderDrawColor(ren, 124, 128, 130, 255);
    SDL_Rect hinge{x - 2, y + 1, 4, 2};
    SDL_RenderFillRect(ren, &hinge);
    SDL_SetRenderDrawColor(ren, 34, 32, 30, 255);
    SDL_Rect foot1{x - 10, y + 29, 4, 2};
    SDL_Rect foot2{x + 9, y + 29, 4, 2};
    SDL_RenderFillRect(ren, &foot1);
    SDL_RenderFillRect(ren, &foot2);
}

void Game::drawTechPipes(int wx, int wy)
{
    int x = sx(static_cast<float>(wx)) + 8;
    int y = sy(static_cast<float>(wy));
    if (x < -32 || x > VIEW_W + 32) return;

    // Две вертикальные трубы с тёмным контуром.
    SDL_SetRenderDrawColor(ren, 24, 28, 28, 255);
    SDL_Rect o1{x - 7, y - 1, 7, 56};
    SDL_Rect o2{x + 1, y - 1, 6, 56};
    SDL_RenderFillRect(ren, &o1);
    SDL_RenderFillRect(ren, &o2);
    SDL_SetRenderDrawColor(ren, 72, 82, 78, 255);
    SDL_Rect p1{x - 6, y, 5, 54};
    SDL_Rect p2{x + 2, y, 4, 54};
    SDL_RenderFillRect(ren, &p1);
    SDL_RenderFillRect(ren, &p2);
    SDL_SetRenderDrawColor(ren, 118, 130, 124, 255);
    SDL_Rect h1{x - 6, y, 1, 54};
    SDL_Rect h2{x + 2, y, 1, 54};
    SDL_RenderFillRect(ren, &h1);
    SDL_RenderFillRect(ren, &h2);

    // Нижний сварной перегиб.
    SDL_SetRenderDrawColor(ren, 24, 28, 28, 255);
    SDL_Rect joint{x - 7, y + 52, 15, 4};
    SDL_RenderFillRect(ren, &joint);
    SDL_SetRenderDrawColor(ren, 84, 94, 90, 255);
    SDL_Rect jointIn{x - 6, y + 53, 13, 2};
    SDL_RenderFillRect(ren, &jointIn);

    // Хомуты крепления.
    SDL_SetRenderDrawColor(ren, 44, 48, 48, 255);
    for (int i = 0; i < 3; ++i) {
        SDL_Rect clamp{x - 7, y + 8 + i * 18, 14, 2};
        SDL_RenderFillRect(ren, &clamp);
    }

    // Красный вентиль-маховик.
    fillCircle(x - 3, y + 22, 5, SDL_Color{112, 38, 30, 255});
    fillCircle(x - 3, y + 22, 4, SDL_Color{170, 58, 46, 255});
    fillCircle(x - 3, y + 22, 2, SDL_Color{48, 24, 20, 255});
    SDL_SetRenderDrawColor(ren, 210, 96, 72, 255);
    for (int i = 0; i < 4; ++i) {
        float a = i * 1.5707963f + 0.7853982f;
        SDL_RenderDrawLine(ren, x - 3, y + 22,
                           x - 3 + static_cast<int>(std::cos(a) * 4.0f),
                           y + 22 + static_cast<int>(std::sin(a) * 4.0f));
    }
}

void Game::drawTechVent(int wx, int wy)
{
    int x = sx(static_cast<float>(wx)) + 1;
    int y = sy(static_cast<float>(wy));
    if (x < -24 || x > VIEW_W + 24) return;

    SDL_SetRenderDrawColor(ren, 36, 40, 42, 255);
    SDL_Rect frame{x, y, 14, 12};
    SDL_RenderFillRect(ren, &frame);
    SDL_SetRenderDrawColor(ren, 82, 88, 90, 255);
    SDL_Rect inner{x + 1, y + 1, 12, 10};
    SDL_RenderFillRect(ren, &inner);
    SDL_SetRenderDrawColor(ren, 24, 28, 29, 255);
    for (int i = 0; i < 4; ++i) {
        SDL_Rect slot{x + 2, y + 2 + i * 2, 10, 1};
        SDL_RenderFillRect(ren, &slot);
    }
    SDL_SetRenderDrawColor(ren, 124, 130, 132, 255);
    SDL_Rect b1{x + 1, y + 1, 1, 1};
    SDL_Rect b2{x + 12, y + 1, 1, 1};
    SDL_Rect b3{x + 1, y + 10, 1, 1};
    SDL_Rect b4{x + 12, y + 10, 1, 1};
    SDL_RenderFillRect(ren, &b1);
    SDL_RenderFillRect(ren, &b2);
    SDL_RenderFillRect(ren, &b3);
    SDL_RenderFillRect(ren, &b4);
}

void Game::drawTechBox(int wx, int wy)
{
    int x = sx(static_cast<float>(wx));
    int y = sy(static_cast<float>(wy));
    if (x < -32 || x > VIEW_W + 32) return;

    // Серый электрощит.
    SDL_SetRenderDrawColor(ren, 50, 54, 56, 255);
    SDL_Rect body{x, y, 18, 24};
    SDL_RenderFillRect(ren, &body);
    SDL_SetRenderDrawColor(ren, 88, 94, 96, 255);
    SDL_Rect lid{x + 2, y + 2, 14, 20};
    SDL_RenderFillRect(ren, &lid);
    SDL_SetRenderDrawColor(ren, 32, 36, 38, 255);
    SDL_Rect seam{x + 8, y + 3, 1, 18};
    SDL_RenderFillRect(ren, &seam);
    SDL_SetRenderDrawColor(ren, 152, 158, 160, 255);
    SDL_Rect handle{x + 12, y + 11, 2, 5};
    SDL_RenderFillRect(ren, &handle);

    // Жёлтый предупреждающий треугольник.
    fillTriangle(x + 4, y + 17, x + 8, y + 10, x + 12, y + 17,
                 SDL_Color{214, 168, 40, 255});
    SDL_SetRenderDrawColor(ren, 30, 26, 22, 255);
    SDL_Rect mark{x + 7, y + 13, 2, 2};
    SDL_RenderFillRect(ren, &mark);

    // Гофрированный кабель вниз.
    SDL_SetRenderDrawColor(ren, 38, 42, 42, 255);
    for (int i = 0; i < 4; ++i) {
        SDL_Rect rib{x + 6, y + 24 + i * 2, 6, 1};
        SDL_RenderFillRect(ren, &rib);
    }
}

void Game::drawTechSign(int wx, int wy)
{
    int x = sx(static_cast<float>(wx));
    int y = sy(static_cast<float>(wy));
    if (x < -32 || x > VIEW_W + 32) return;

    SDL_SetRenderDrawColor(ren, 26, 28, 28, 255);
    SDL_Rect frame{x, y, 26, 13};
    SDL_RenderFillRect(ren, &frame);
    SDL_SetRenderDrawColor(ren, 198, 158, 44, 255);
    SDL_Rect plate{x + 1, y + 1, 24, 11};
    SDL_RenderFillRect(ren, &plate);

    // Чёрные косые полосы по краям.
    SDL_SetRenderDrawColor(ren, 30, 28, 24, 255);
    for (int i = 0; i < 3; ++i) {
        SDL_Rect l{x + 2 + i * 2, y + 1, 1, 11};
        SDL_Rect r{x + 23 - i * 2, y + 1, 1, 11};
        SDL_RenderFillRect(ren, &l);
        SDL_RenderFillRect(ren, &r);
    }

    pixfont::drawText(ren, x + 9, y + 3, "ТЕХ", 1,
                      SDL_Color{34, 30, 24, 255}, false);
}

void Game::drawCableCoil(int wx, int wy)
{
    int x = sx(static_cast<float>(wx)) + 8;
    int y = sy(static_cast<float>(wy)) + 8;
    if (x < -24 || x > VIEW_W + 24) return;

    // Крюк на стене.
    SDL_SetRenderDrawColor(ren, 84, 88, 90, 255);
    SDL_Rect hook{x - 1, y - 9, 2, 6};
    SDL_RenderFillRect(ren, &hook);

    // Моток кабеля.
    fillCircle(x, y, 7, SDL_Color{46, 44, 42, 255});
    fillCircle(x, y, 5, SDL_Color{98, 76, 46, 255});
    fillCircle(x, y, 2, SDL_Color{28, 28, 28, 255});
    SDL_SetRenderDrawColor(ren, 138, 108, 64, 210);
    SDL_RenderDrawLine(ren, x - 6, y - 2, x + 6, y - 2);
    SDL_RenderDrawLine(ren, x - 7, y + 2, x + 7, y + 2);

    // Свисающий конец.
    SDL_SetRenderDrawColor(ren, 68, 52, 34, 255);
    SDL_RenderDrawLine(ren, x + 4, y + 5, x + 8, y + 13);
}

} // namespace kolek
