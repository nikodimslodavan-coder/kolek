#include "game/game.h"
#include "core/math.h"
#include "rendering/font.h"

namespace kolek {

void Game::drawWallpaper()
{
    // Второй уровень — технический этаж, у него свой бетонный фон.
    if (levelIndex == 1) {
        drawTechnicalWallpaper();
        return;
    }
    // Третий — вестибюль с мрамором и бронзой.
    if (levelIndex == 2) {
        drawLobbyWallpaper();
        return;
    }
    // Четвёртый — ночная парковка перед апартаментами.
    if (levelIndex == 3) {
        drawParkingWallpaper();
        return;
    }

    int tx0 = std::max(0, static_cast<int>(camX / TILE) - 1);
    int tx1 = std::min(lvl.w - 1, static_cast<int>((camX + VIEW_W) / TILE) + 1);

    // Обои повторяются на каждом этаже высотой 272 px.
    for (float base = 0; base < lvl.h * TILE; base += 272.0f) {
        if (base + 272.0f < camY - 8.0f || base > camY + VIEW_H + 8.0f) continue;

        for (int tx = tx0; tx <= tx1; ++tx) {
            int band = ((tx / 24) % 7 + 7) % 7;
            SDL_Color baseCol = wallPalette[band];
            fillW(tx * 16.0f, base + 16.0f, 16.0f, 112.0f, baseCol);
            fillW(tx * 16.0f, base + 144.0f, 16.0f, 96.0f, shade(baseCol, 0.70f));

            // обои в тонкую полоску
            fillW(tx * 16.0f + 14.0f, base + 16.0f, 2.0f, 112.0f, shade(baseCol, 0.86f));
            fillW(tx * 16.0f + 14.0f, base + 144.0f, 2.0f, 96.0f, shade(baseCol, 0.60f));

            // панели-вагонка
            fillW(tx * 16.0f, base + 120.0f, 16.0f, 8.0f, SDL_Color{52, 34, 28, 255});
            fillW(tx * 16.0f, base + 232.0f, 16.0f, 8.0f, SDL_Color{44, 30, 26, 255});
            fillW(tx * 16.0f + 8.0f, base + 120.0f, 1.0f, 8.0f, SDL_Color{76, 52, 36, 255});
            fillW(tx * 16.0f + 8.0f, base + 232.0f, 1.0f, 8.0f, SDL_Color{66, 46, 32, 255});
        }

        // карнизы и плинтусы
        fillW(camX, base + 16.0f, static_cast<float>(VIEW_W), 3.0f, SDL_Color{70, 48, 34, 255});
        fillW(camX, base + 19.0f, static_cast<float>(VIEW_W), 1.0f, SDL_Color{112, 82, 50, 120});
        fillW(camX, base + 144.0f, static_cast<float>(VIEW_W), 3.0f, SDL_Color{58, 40, 30, 255});
        fillW(camX, base + 147.0f, static_cast<float>(VIEW_W), 1.0f, SDL_Color{94, 68, 44, 120});
        fillW(camX, base + 126.0f, static_cast<float>(VIEW_W), 2.0f, SDL_Color{36, 24, 22, 255});
        fillW(camX, base + 238.0f, static_cast<float>(VIEW_W), 2.0f, SDL_Color{30, 20, 18, 255});

        // мягкое затемнение у пола
        for (int i = 0; i < 6; ++i) {
            fillW(camX, base + 96.0f + i * 5.0f, static_cast<float>(VIEW_W), 5.0f,
                  SDL_Color{10, 6, 10, static_cast<Uint8>(6 + i * 3)});
            fillW(camX, base + 202.0f + i * 5.0f, static_cast<float>(VIEW_W), 5.0f,
                  SDL_Color{8, 5, 8, static_cast<Uint8>(10 + i * 4)});
        }
    }
}

void Game::drawTechnicalWallpaper()
{
    int tx0 = std::max(0, static_cast<int>(camX / TILE) - 1);
    int tx1 = std::min(lvl.w - 1, static_cast<int>((camX + VIEW_W) / TILE) + 1);

    // Технический этаж: бетонные панели, кабельные лотки и аварийная
    // разметка. Полосы по 272 px совпадают с этажами карты.
    int band = 0;
    for (float base = 0; base < lvl.h * TILE; base += 272.0f, ++band) {
        if (base + 272.0f < camY - 8.0f || base > camY + VIEW_H + 8.0f) continue;

        for (int tx = tx0; tx <= tx1; ++tx) {
            // Цвет — на крупную панель шириной 4 тайла, а не на каждый тайл:
            // так стена читается как бетонные плиты, а не как шум.
            int block = tx / 4;
            uint32_t h = hash2(block, band * 37 + 11);
            int v = static_cast<int>(h % 14);

            // Холодный серо-зелёный бетон, низ панели темнее.
            SDL_Color panel{static_cast<Uint8>(42 + v), static_cast<Uint8>(48 + v),
                            static_cast<Uint8>(48 + v), 255};
            fillW(tx * 16.0f, base + 16.0f, 16.0f, 112.0f, panel);
            fillW(tx * 16.0f, base + 144.0f, 16.0f, 96.0f, shade(panel, 0.72f));

            // Вертикальные швы бетонных плит и заклёпки по углам.
            if (tx % 4 == 0) {
                fillW(tx * 16.0f, base + 16.0f, 1.0f, 224.0f,
                      SDL_Color{20, 24, 25, 255});
                fillW(tx * 16.0f + 1.0f, base + 16.0f, 1.0f, 224.0f,
                      SDL_Color{78, 86, 86, 90});
                fillW(tx * 16.0f + 3.0f, base + 22.0f, 2.0f, 2.0f,
                      SDL_Color{98, 104, 104, 190});
                fillW(tx * 16.0f + 3.0f, base + 138.0f, 2.0f, 2.0f,
                      SDL_Color{88, 94, 94, 160});
                fillW(tx * 16.0f + 3.0f, base + 150.0f, 2.0f, 2.0f,
                      SDL_Color{90, 96, 96, 160});
                fillW(tx * 16.0f + 3.0f, base + 230.0f, 2.0f, 2.0f,
                      SDL_Color{80, 86, 86, 150});
            }

            // Потёки и пятна на бетоне.
            if ((h >> 4) % 9 == 0)
                fillW(tx * 16.0f + 5.0f, base + 28.0f, 2.0f,
                      22.0f + static_cast<float>(h % 24),
                      SDL_Color{28, 26, 22, 70});
            if ((h >> 8) % 13 == 0)
                fillW(tx * 16.0f + 9.0f, base + 160.0f, 3.0f, 18.0f,
                      SDL_Color{20, 24, 22, 90});

            // Кабельный лоток под перекрытием.
            fillW(tx * 16.0f, base + 20.0f, 16.0f, 3.0f,
                  (tx % 3 == 1) ? SDL_Color{26, 28, 30, 255}
                                : SDL_Color{34, 36, 38, 255});
            fillW(tx * 16.0f, base + 20.0f, 16.0f, 1.0f,
                  SDL_Color{82, 88, 90, 255});
            if ((h >> 6) % 3 == 0)
                fillW(tx * 16.0f + 3.0f, base + 22.0f, 10.0f, 1.0f,
                      SDL_Color{98, 76, 50, 220});
        }

        // Горизонтальный шов между верхней и нижней плитой.
        fillW(camX, base + 143.0f, static_cast<float>(VIEW_W), 2.0f,
              SDL_Color{20, 24, 25, 255});
        fillW(camX, base + 145.0f, static_cast<float>(VIEW_W), 1.0f,
              SDL_Color{78, 86, 86, 80});

        // Аварийная разметка вдоль полов: жёлто-чёрная полоса на стене
        // прямо над перекрытием.
        const int row0 = static_cast<int>(base / TILE);
        for (int ty = row0; ty < row0 + 17; ++ty) {
            for (int tx = tx0; tx <= tx1; ++tx) {
                if (lvl.at(tx, ty) != '.') continue;
                char below = lvl.at(tx, ty + 1);
                if (below != '=' && below != '#') continue;
                float hy = ty * 16.0f + 10.0f;
                fillW(tx * 16.0f, hy, 16.0f, 6.0f,
                      SDL_Color{150, 112, 30, 255});
                for (int k = 0; k < 16; k += 6)
                    fillW(tx * 16.0f + k, hy, 3.0f, 6.0f,
                          SDL_Color{30, 28, 24, 255});
            }
        }

        // Пыльный низ стены и холодный отблеск под перекрытием.
        for (int i = 0; i < 5; ++i) {
            fillW(camX, base + 120.0f + i * 4.0f, static_cast<float>(VIEW_W),
                  4.0f, SDL_Color{10, 12, 12, static_cast<Uint8>(6 + i * 3)});
            fillW(camX, base + 236.0f + i * 7.0f, static_cast<float>(VIEW_W),
                  7.0f, SDL_Color{8, 10, 10, static_cast<Uint8>(8 + i * 4)});
        }
        fillW(camX, base + 16.0f, static_cast<float>(VIEW_W), 1.0f,
              SDL_Color{96, 104, 104, 90});
    }
}

void Game::drawLobbyWallpaper()
{
    // Вестибюль «ДАЙМОНДА»: светлый мрамор, бронзовые пилястры, дубовый
    // низ стены и ковровая дорожка. Пол основного зала — на шестом ряду
    // снизу; всё, что ниже, уходит в тёмную шахту пропастей.
    const float floorY = (lvl.h - 6) * TILE;
    const float wainTop = floorY - 74.0f;

    int tx0 = std::max(0, static_cast<int>(camX / TILE) - 1);
    int tx1 = std::min(lvl.w - 1, static_cast<int>((camX + VIEW_W) / TILE) + 1);

    // Тёмная шахта под полом: толща перекрытия, опоры и тусклый низ.
    fillW(camX - 8.0f, floorY, static_cast<float>(VIEW_W) + 16.0f,
          lvl.h * TILE - floorY, SDL_Color{14, 12, 16, 255});
    fillW(camX, floorY, static_cast<float>(VIEW_W), 6.0f,
          SDL_Color{56, 48, 40, 255});
    fillW(camX, floorY, static_cast<float>(VIEW_W), 1.0f,
          SDL_Color{118, 104, 84, 200});
    for (int tx = tx0; tx <= tx1; ++tx) {
        float x = tx * 16.0f;
        uint32_t h = hash2(tx, 31);
        // Кирпичная кладка дальней стены шахты.
        if (tx % 4 == 0) {
            fillW(x, floorY + 6.0f, 1.0f, lvl.h * TILE - floorY - 6.0f,
                  SDL_Color{8, 7, 9, 255});
            fillW(x + 1.0f, floorY + 6.0f, 1.0f, lvl.h * TILE - floorY - 6.0f,
                  SDL_Color{38, 32, 32, 90});
        }
        if ((h >> 4) % 7 == 0)
            fillW(x + 3.0f, floorY + 12.0f + static_cast<float>(h % 36), 2.0f,
                  12.0f, SDL_Color{30, 26, 28, 120});
        // Опорные пилоны, уходящие вниз.
        if (tx % 6 == 1) {
            fillW(x + 2.0f, floorY + 6.0f, 12.0f,
                  lvl.h * TILE - floorY - 6.0f, SDL_Color{22, 19, 22, 255});
            fillW(x + 3.0f, floorY + 6.0f, 2.0f,
                  lvl.h * TILE - floorY - 6.0f, SDL_Color{42, 36, 36, 255});
            fillW(x + 11.0f, floorY + 6.0f, 2.0f,
                  lvl.h * TILE - floorY - 6.0f, SDL_Color{10, 9, 11, 255});
        }
    }
    for (int i = 0; i < 4; ++i)
        fillW(camX, floorY + 26.0f + i * 14.0f, static_cast<float>(VIEW_W),
              12.0f, SDL_Color{34, 28, 28, static_cast<Uint8>(80 - i * 16)});
    fillW(camX + 40.0f, floorY + 74.0f, static_cast<float>(VIEW_W) - 80.0f,
          40.0f, SDL_Color{52, 40, 30, 45});

    // Потолок с кессонами и золочёным карнизом.
    fillW(camX, 0.0f, static_cast<float>(VIEW_W), 20.0f,
          SDL_Color{38, 30, 28, 255});
    for (int tx = tx0; tx <= tx1; ++tx) {
        if (tx % 2 != 0) continue;
        float x = tx * 16.0f;
        fillW(x + 2.0f, 3.0f, 12.0f, 11.0f, SDL_Color{26, 20, 20, 255});
        fillW(x + 3.0f, 4.0f, 10.0f, 1.0f, SDL_Color{58, 46, 38, 255});
    }
    fillW(camX, 20.0f, static_cast<float>(VIEW_W), 4.0f,
          SDL_Color{172, 140, 76, 255});
    fillW(camX, 24.0f, static_cast<float>(VIEW_W), 2.0f,
          SDL_Color{98, 78, 44, 255});
    fillW(camX, 26.0f, static_cast<float>(VIEW_W), 2.0f,
          SDL_Color{52, 40, 32, 255});

    // Стена по колоннам: мраморные панели, прожилки и дубовый низ.
    for (int tx = tx0; tx <= tx1; ++tx) {
        float x = tx * 16.0f;
        uint32_t h = hash2(tx / 2, 5);
        int v = static_cast<int>(h % 10);

        SDL_Color marble{static_cast<Uint8>(184 + v),
                         static_cast<Uint8>(174 + v),
                         static_cast<Uint8>(156 + v), 255};
        fillW(x, 28.0f, 16.0f, wainTop - 28.0f, marble);

        // Прожилки мрамора.
        if ((h >> 5) % 4 == 0) {
            fillW(x + 3.0f, 46.0f + static_cast<float>(h % 70), 1.0f,
                  16.0f + static_cast<float>(h % 22),
                  SDL_Color{158, 146, 126, 70});
            fillW(x + 9.0f, 108.0f + static_cast<float>(h % 34), 1.0f,
                  12.0f, SDL_Color{166, 154, 134, 55});
        }
        // Тёплый блик сверху панели.
        fillW(x, 28.0f, 16.0f, 2.0f, SDL_Color{216, 206, 186, 70});

        // Рамка панели каждые 5 тайлов.
        if (tx % 5 == 0) {
            fillW(x, 28.0f, 1.0f, wainTop - 28.0f,
                  SDL_Color{146, 132, 110, 120});
            fillW(x + 15.0f, 28.0f, 1.0f, wainTop - 28.0f,
                  SDL_Color{96, 84, 70, 90});
        }

        // Дубовая обшивка снизу, с филёнкой и золотым поясом.
        fillW(x, wainTop - 3.0f, 16.0f, 3.0f, SDL_Color{172, 140, 76, 255});
        fillW(x, wainTop, 16.0f, floorY - wainTop,
              SDL_Color{64, 43, 32, 255});
        fillW(x, wainTop, 16.0f, 1.0f, SDL_Color{96, 68, 46, 255});
        if (tx % 3 == 1)
            fillW(x + 3.0f, wainTop + 12.0f, 10.0f, floorY - wainTop - 20.0f,
                  SDL_Color{52, 35, 27, 255});
        fillW(x, floorY - 2.0f, 16.0f, 1.0f, SDL_Color{34, 23, 19, 255});

        // Бронзовая пилястра с капителью каждые 9 тайлов.
        if (tx % 9 == 3) {
            fillW(x + 1.0f, 26.0f, 14.0f, floorY - 26.0f,
                  SDL_Color{74, 54, 38, 255});
            fillW(x + 3.0f, 26.0f, 3.0f, floorY - 26.0f,
                  SDL_Color{108, 82, 54, 255});
            fillW(x + 10.0f, 26.0f, 3.0f, floorY - 26.0f,
                  SDL_Color{52, 38, 28, 255});
            fillW(x, 22.0f, 16.0f, 6.0f, SDL_Color{150, 118, 62, 255});
            fillW(x + 1.0f, 28.0f, 14.0f, 3.0f, SDL_Color{124, 96, 52, 255});
            fillW(x, floorY - 12.0f, 16.0f, 5.0f,
                  SDL_Color{132, 102, 56, 255});
        }
    }

    // Мягкий свет у пола и лёгкая пыль в воздухе.
    for (int i = 0; i < 4; ++i)
        fillW(camX, floorY - 26.0f + i * 6.0f, static_cast<float>(VIEW_W),
              6.0f, SDL_Color{255, 220, 160, static_cast<Uint8>(6 + i * 4)});
}

void Game::drawParkingWallpaper()
{
    // Ночная парковка перед «ДАЙМОНДОМ»: тёмное небо, фасад дома с
    // освещёнными окнами, асфальт с разметкой, фонари и джип у босса.
    const float floorY = (lvl.h - 2) * TILE;       // ряд 17 — асфальт
    const float asphaltTop = floorY - 42.0f;
    const float facadeTop = 62.0f;
    const float facadeBottom = asphaltTop - 6.0f;

    int tx0 = std::max(0, static_cast<int>(camX / TILE) - 1);
    int tx1 = std::min(lvl.w - 1, static_cast<int>((camX + VIEW_W) / TILE) + 1);

    // Небо: несколько холодных полос от почти чёрного к синему.
    for (int i = 0; i < 8; ++i) {
        SDL_Color sky{static_cast<Uint8>(10 + i * 3),
                      static_cast<Uint8>(14 + i * 4),
                      static_cast<Uint8>(34 + i * 5), 255};
        fillW(camX - 8.0f, static_cast<float>(i * 9), VIEW_W + 16.0f, 10.0f, sky);
    }

    // Редкие звёзды над крышей.
    for (int tx = tx0; tx <= tx1; ++tx) {
        for (int k = 0; k < 3; ++k) {
            uint32_t sh = hash2(tx * 3 + k, 29);
            if (sh % 6 != 0) continue;
            float starX = tx * 16.0f + static_cast<float>((sh >> 4) % 16);
            float starY = 3.0f + static_cast<float>((sh >> 9) % 52);
            Uint8 a = static_cast<Uint8>(90 + (sh >> 17) % 120);
            fillW(starX, starY, 1.0f, 1.0f, SDL_Color{210, 220, 255, a});
        }
    }

    // Луна с лёгким ореолом.
    {
        int mx = sx(386.0f);
        int my = sy(31.0f);
        fillCircle(mx, my, 10, SDL_Color{210, 214, 196, 26});
        fillCircle(mx, my, 8, SDL_Color{224, 224, 202, 235});
        fillCircle(mx - 2, my - 2, 6, SDL_Color{240, 238, 216, 245});
        fillCircle(mx + 3, my + 2, 2, SDL_Color{198, 198, 180, 150});
    }

    // Фасад апартаментов: тёмные бетонные панели и сетка окон.
    fillW(camX - 8.0f, facadeTop, VIEW_W + 16.0f,
          facadeBottom - facadeTop, SDL_Color{34, 37, 52, 255});
    // Парапет крыши.
    fillW(camX - 8.0f, facadeTop - 6.0f, VIEW_W + 16.0f, 7.0f,
          SDL_Color{24, 26, 38, 255});
    fillW(camX - 8.0f, facadeTop - 1.0f, VIEW_W + 16.0f, 2.0f,
          SDL_Color{76, 80, 100, 180});

    for (int tx = tx0; tx <= tx1; ++tx) {
        float x = tx * 16.0f;
        uint32_t h = hash2(tx, 13);

        // Вертикальные швы панелей и потёки.
        if (tx % 4 == 0) {
            fillW(x, facadeTop, 1.0f, facadeBottom - facadeTop,
                  SDL_Color{20, 22, 32, 255});
            fillW(x + 1.0f, facadeTop, 1.0f, facadeBottom - facadeTop,
                  SDL_Color{56, 60, 78, 70});
        }
        if ((h >> 5) % 11 == 0)
            fillW(x + 4.0f, facadeTop + 12.0f + static_cast<float>(h % 30),
                  2.0f, 26.0f + static_cast<float>((h >> 8) % 30),
                  SDL_Color{18, 20, 28, 80});

        // Четыре этажа окон. Часть горит тёплым светом, часть — тёмная.
        for (int row = 0; row < 4; ++row) {
            float wy = facadeTop + 14.0f + static_cast<float>(row) * 38.0f;
            uint32_t wh = hash2(tx, row * 41 + 7);
            bool lit = (wh % 7 < 3);
            bool curtain = ((wh >> 4) % 5 == 0);

            fillW(x + 2.0f, wy, 12.0f, 22.0f, SDL_Color{24, 26, 38, 255});
            SDL_Color glass = lit ? SDL_Color{232, 184, 94, 235}
                                  : SDL_Color{30, 40, 66, 220};
            if (curtain) glass = lit ? SDL_Color{198, 132, 78, 220}
                                     : SDL_Color{26, 32, 54, 220};
            fillW(x + 3.0f, wy + 1.0f, 10.0f, 20.0f, glass);

            // Переплёт и подоконник.
            fillW(x + 7.0f, wy + 1.0f, 1.0f, 20.0f,
                  SDL_Color{18, 20, 28, 200});
            fillW(x + 3.0f, wy + 10.0f, 10.0f, 1.0f,
                  SDL_Color{18, 20, 28, 180});
            fillW(x + 1.0f, wy + 21.0f, 14.0f, 2.0f,
                  SDL_Color{56, 58, 72, 220});
            if (lit)
                fillW(x + 4.0f, wy + 2.0f, 3.0f, 7.0f,
                      SDL_Color{255, 226, 160, 55});

            // Редкие балконы с решёткой.
            if ((wh >> 9) % 6 == 0) {
                fillW(x + 1.0f, wy + 23.0f, 14.0f, 2.0f,
                      SDL_Color{64, 66, 78, 255});
                for (int bx = 2; bx < 14; bx += 3)
                    fillW(x + static_cast<float>(bx), wy + 19.0f, 1.0f, 5.0f,
                          SDL_Color{52, 54, 66, 230});
            }
        }
    }

    // Цоколь и входная группа с тёплым светом.
    fillW(camX - 8.0f, facadeBottom - 22.0f, VIEW_W + 16.0f, 22.0f,
          SDL_Color{26, 27, 38, 255});
    fillW(camX - 8.0f, facadeBottom - 3.0f, VIEW_W + 16.0f, 3.0f,
          SDL_Color{52, 54, 66, 255});
    {
        const float doorX = 298.0f;
        const float doorTop = facadeBottom - 42.0f;
        fillW(doorX - 6.0f, doorTop - 4.0f, 60.0f, 46.0f,
              SDL_Color{20, 22, 32, 255});
        fillW(doorX, doorTop, 48.0f, 40.0f, SDL_Color{44, 42, 44, 255});
        fillW(doorX + 3.0f, doorTop + 4.0f, 18.0f, 33.0f,
              SDL_Color{232, 184, 96, 210});
        fillW(doorX + 27.0f, doorTop + 4.0f, 18.0f, 33.0f,
              SDL_Color{232, 184, 96, 210});
        fillW(doorX + 23.0f, doorTop + 4.0f, 2.0f, 33.0f,
              SDL_Color{28, 26, 30, 255});
        // Козырёк и табличка дома.
        fillW(doorX - 10.0f, doorTop - 8.0f, 68.0f, 5.0f,
              SDL_Color{76, 62, 44, 255});
        addLight(doorX + 24.0f, doorTop + 18.0f, 48.0f,
                 SDL_Color{255, 186, 96, 255}, 34.0f);
    }

    // Вывеска на фасаде.
    {
        const std::string sign = "АПАРТАМЕНТЫ «ДАЙМОНД»";
        int sw = pixfont::textWidth(sign, 1);
        float wx = 320.0f - sw * 0.5f;
        fillW(wx - 3.0f, facadeTop + 2.0f, static_cast<float>(sw) + 6.0f, 10.0f,
              SDL_Color{18, 19, 28, 230});
        pixfont::drawText(ren, sx(wx), sy(facadeTop + 3.0f), sign, 1,
                          SDL_Color{196, 176, 132, 220}, false);
    }

    // Тротуар и асфальт.
    fillW(camX - 8.0f, asphaltTop - 5.0f, VIEW_W + 16.0f, 6.0f,
          SDL_Color{66, 65, 68, 255});
    fillW(camX - 8.0f, asphaltTop - 5.0f, VIEW_W + 16.0f, 1.0f,
          SDL_Color{104, 102, 100, 180});
    fillW(camX - 8.0f, asphaltTop + 1.0f, VIEW_W + 16.0f,
          floorY - asphaltTop, SDL_Color{26, 27, 32, 255});

    // Разметка парковочных мест и мелкий мусор на асфальте.
    for (int tx = tx0; tx <= tx1; ++tx) {
        float x = tx * 16.0f;
        uint32_t h = hash2(tx, 61);
        if ((h >> 3) % 5 == 0)
            fillW(x + static_cast<float>(h % 12), asphaltTop + 8.0f +
                  static_cast<float>((h >> 8) % 24), 1.0f, 1.0f,
                  SDL_Color{56, 56, 62, 160});
    }
    for (float x = 24.0f; x < lvl.w * TILE; x += 64.0f) {
        fillW(x, floorY - 22.0f, 2.0f, 18.0f, SDL_Color{196, 194, 170, 80});
        fillW(x - 5.0f, floorY - 5.0f, 12.0f, 2.0f,
              SDL_Color{196, 194, 170, 55});
    }

    // Фонари: тёмные столбы с тёплыми лампами.
    auto lamp = [&](float lx, bool armRight) {
        float top = floorY - 118.0f;
        fillW(lx + 2.0f, top, 4.0f, floorY - top, SDL_Color{34, 35, 42, 255});
        fillW(lx + 2.0f, top, 1.0f, floorY - top, SDL_Color{66, 68, 78, 110});
        float armX = armRight ? lx + 4.0f : lx - 14.0f;
        fillW(armX, top, 16.0f, 3.0f, SDL_Color{34, 35, 42, 255});
        float headX = armRight ? lx + 16.0f : lx - 18.0f;
        fillW(headX, top - 2.0f, 8.0f, 5.0f, SDL_Color{28, 29, 36, 255});
        fillW(headX + 1.0f, top + 3.0f, 6.0f, 2.0f,
              SDL_Color{255, 214, 132, 245});
        addLight(headX + 4.0f, top + 7.0f, 66.0f,
                 SDL_Color{255, 198, 110, 255}, 58.0f);
        addLight(headX + 4.0f, floorY - 8.0f, 34.0f,
                 SDL_Color{255, 196, 108, 255}, 22.0f);
    };
    lamp(104.0f, true);
    lamp(548.0f, false);

    // Джип у босса.
    {
        const float jx = 396.0f;
        const float jy = floorY - 34.0f;

        fillW(jx, floorY - 3.0f, 78.0f, 3.0f, SDL_Color{0, 0, 0, 90});

        const SDL_Color body{58, 66, 50, 255};
        const SDL_Color bodyDark{38, 45, 34, 255};
        const SDL_Color bodyLight{86, 96, 70, 255};

        fillW(jx + 3.0f, jy + 14.0f, 66.0f, 16.0f, body);
        fillW(jx + 3.0f, jy + 14.0f, 66.0f, 2.0f, bodyLight);
        fillW(jx + 3.0f, jy + 28.0f, 66.0f, 3.0f, bodyDark);
        fillW(jx + 2.0f, jy + 10.0f, 22.0f, 7.0f, body);
        fillW(jx + 2.0f, jy + 10.0f, 22.0f, 1.0f, bodyLight);
        fillW(jx + 26.0f, jy + 3.0f, 30.0f, 14.0f, body);
        fillW(jx + 54.0f, jy + 9.0f, 16.0f, 8.0f, body);
        fillW(jx + 26.0f, jy + 3.0f, 30.0f, 2.0f, bodyLight);

        // Стёкла и стойки кузова.
        fillW(jx + 30.0f, jy + 5.0f, 11.0f, 9.0f, SDL_Color{78, 104, 116, 255});
        fillW(jx + 44.0f, jy + 5.0f, 8.0f, 9.0f, SDL_Color{70, 94, 106, 255});
        fillW(jx + 41.0f, jy + 3.0f, 2.0f, 14.0f, bodyDark);
        fillW(jx + 31.0f, jy + 6.0f, 3.0f, 3.0f, SDL_Color{132, 158, 166, 140});

        // Колёса.
        for (float wx : {jx + 8.0f, jx + 52.0f}) {
            fillW(wx, jy + 26.0f, 14.0f, 14.0f, SDL_Color{22, 22, 25, 255});
            fillW(wx + 3.0f, jy + 29.0f, 8.0f, 8.0f, SDL_Color{48, 49, 52, 255});
            fillW(wx + 5.0f, jy + 31.0f, 4.0f, 4.0f, SDL_Color{92, 94, 96, 255});
        }

        // Фары, бампер и запасное колесо сзади.
        fillW(jx + 66.0f, jy + 14.0f, 3.0f, 4.0f, SDL_Color{255, 230, 150, 245});
        fillW(jx + 67.0f, jy + 15.0f, 2.0f, 2.0f, SDL_Color{255, 248, 210, 255});
        fillW(jx + 2.0f, jy + 24.0f, 68.0f, 3.0f, SDL_Color{30, 31, 34, 255});
        fillW(jx - 1.0f, jy + 13.0f, 6.0f, 12.0f, SDL_Color{24, 25, 27, 255});
        fillW(jx, jy + 15.0f, 4.0f, 8.0f, SDL_Color{46, 48, 44, 255});
        addLight(jx + 68.0f, jy + 16.0f, 30.0f,
                 SDL_Color{255, 210, 130, 255}, 30.0f);
    }
}

// ---------------------------------------------------------------------------
//  Тайлы
// ---------------------------------------------------------------------------
void Game::drawTiles()
{
    int tx0 = std::max(0, static_cast<int>(camX / TILE) - 1);
    int tx1 = std::min(lvl.w - 1, static_cast<int>((camX + VIEW_W) / TILE) + 1);
    for (int ty = 0; ty < lvl.h; ++ty) {
        int y = sy(ty * 16.0f);
        if (y <= -20 || y >= VIEW_H + 20) continue;
        for (int tx = tx0; tx <= tx1; ++tx) {
            int x = sx(tx * 16.0f);
            switch (lvl.at(tx, ty)) {
                case '#': drawBrick(x, y, tx, ty); break;
                case '=': drawFloorTile(x, y, tx, ty); break;
                case 't': drawTableTile(x, y, tx, ty); break;
                case 'S': drawStepTile(x, y, tx, ty); break;
                case 'L': drawLadder(x, y); break;
                case '.': drawPitTile(x, y, tx, ty); break;
                default: break;
            }
        }
    }
}

void Game::drawBrick(int x, int y, int tx, int ty)
{
    if (levelIndex == 1) {
        // Технический этаж: серый бетонный блок с тёмным швом.
        uint32_t bh = hash2(tx, ty);
        int bv = static_cast<int>(bh % 10);
        SDL_Color bbase{static_cast<Uint8>(60 + bv), static_cast<Uint8>(64 + bv),
                        static_cast<Uint8>(64 + bv), 255};
        SDL_SetRenderDrawColor(ren, bbase.r, bbase.g, bbase.b, 255);
        SDL_Rect br{x, y, 16, 16};
        SDL_RenderFillRect(ren, &br);
        SDL_SetRenderDrawColor(ren, 38, 42, 42, 255);
        SDL_RenderDrawLine(ren, x, y + 7, x + 15, y + 7);
        SDL_RenderDrawLine(ren, x, y + 15, x + 15, y + 15);
        int boff = (ty % 2 == 0) ? 8 : 0;
        SDL_RenderDrawLine(ren, x + boff, y, x + boff, y + 6);
        SDL_SetRenderDrawColor(ren, bbase.r + 20, bbase.g + 20, bbase.b + 18, 80);
        SDL_RenderDrawLine(ren, x + 1, y + 1, x + 14, y + 1);
        return;
    }

    if (levelIndex == 2) {
        // Вестибюль: светлые мраморные блоки с тонкими швами.
        uint32_t bh = hash2(tx, ty);
        int bv = static_cast<int>(bh % 8);
        SDL_Color bbase{static_cast<Uint8>(152 + bv),
                        static_cast<Uint8>(144 + bv),
                        static_cast<Uint8>(126 + bv), 255};
        SDL_SetRenderDrawColor(ren, bbase.r, bbase.g, bbase.b, 255);
        SDL_Rect br{x, y, 16, 16};
        SDL_RenderFillRect(ren, &br);
        SDL_SetRenderDrawColor(ren, 118, 108, 92, 255);
        SDL_RenderDrawLine(ren, x, y + 15, x + 15, y + 15);
        SDL_RenderDrawLine(ren, x + 15, y, x + 15, y + 15);
        if ((bh >> 3) % 3 == 0) {
            SDL_SetRenderDrawColor(ren, 132, 122, 104, 200);
            SDL_RenderDrawLine(ren, x + 2, y + 4 + static_cast<int>(bh % 6),
                               x + 10, y + 9 + static_cast<int>(bh % 4));
        }
        SDL_SetRenderDrawColor(ren, 196, 188, 168, 90);
        SDL_RenderDrawLine(ren, x + 1, y + 1, x + 14, y + 1);
        return;
    }

    if (levelIndex == 3) {
        // Парковка: тёмные бетонные блоки ограды.
        uint32_t bh = hash2(tx, ty);
        int bv = static_cast<int>(bh % 8);
        SDL_Color bbase{static_cast<Uint8>(34 + bv),
                        static_cast<Uint8>(36 + bv),
                        static_cast<Uint8>(46 + bv), 255};
        SDL_SetRenderDrawColor(ren, bbase.r, bbase.g, bbase.b, 255);
        SDL_Rect br{x, y, 16, 16};
        SDL_RenderFillRect(ren, &br);
        SDL_SetRenderDrawColor(ren, 20, 22, 30, 255);
        SDL_RenderDrawLine(ren, x, y + 15, x + 15, y + 15);
        SDL_RenderDrawLine(ren, x + 15, y, x + 15, y + 15);
        int boff = (ty % 2 == 0) ? 8 : 0;
        SDL_RenderDrawLine(ren, x + boff, y, x + boff, y + 7);
        SDL_SetRenderDrawColor(ren, bbase.r + 24, bbase.g + 24, bbase.b + 26, 70);
        SDL_RenderDrawLine(ren, x + 1, y + 1, x + 14, y + 1);
        return;
    }

    uint32_t h = hash2(tx, ty);
    int v = static_cast<int>(h % 12);
    SDL_Color base{static_cast<Uint8>(58 + v), static_cast<Uint8>(39 + v / 2),
                   static_cast<Uint8>(43 + v / 3), 255};
    SDL_SetRenderDrawColor(ren, base.r, base.g, base.b, 255);
    SDL_Rect r{x, y, 16, 16};
    SDL_RenderFillRect(ren, &r);

    SDL_SetRenderDrawColor(ren, 36, 24, 29, 255);
    SDL_RenderDrawLine(ren, x, y + 7, x + 15, y + 7);
    SDL_RenderDrawLine(ren, x, y + 15, x + 15, y + 15);
    int off = (ty % 2 == 0) ? 8 : 0;
    SDL_RenderDrawLine(ren, x + off, y, x + off, y + 6);
    SDL_RenderDrawLine(ren, x + (off == 8 ? 0 : 8), y + 8, x + (off == 8 ? 0 : 8), y + 14);

    SDL_SetRenderDrawColor(ren, base.r + 18, base.g + 12, base.b + 10, 90);
    SDL_RenderDrawLine(ren, x + 1, y + 1, x + 14, y + 1);
}

void Game::drawFloorTile(int x, int y, int tx, int ty)
{
    (void)tx;
    if (levelIndex == 1) {
        // Технический этаж: стальной настил с поперечными рёбрами.
        SDL_SetRenderDrawColor(ren, 58, 62, 64, 255);
        SDL_Rect fr{x, y, 16, 16};
        SDL_RenderFillRect(ren, &fr);
        SDL_SetRenderDrawColor(ren, 92, 98, 100, 255);
        SDL_Rect ftop{x, y, 16, 4};
        SDL_RenderFillRect(ren, &ftop);
        SDL_SetRenderDrawColor(ren, 128, 134, 136, 255);
        SDL_Rect fshine{x, y, 16, 1};
        SDL_RenderFillRect(ren, &fshine);
        SDL_SetRenderDrawColor(ren, 40, 44, 46, 255);
        int foff = (ty % 2 == 0) ? 4 : 12;
        SDL_RenderDrawLine(ren, x + foff, y + 4, x + foff, y + 15);
        SDL_SetRenderDrawColor(ren, 22, 26, 28, 160);
        SDL_Rect fbottom{x, y + 13, 16, 3};
        SDL_RenderFillRect(ren, &fbottom);
        return;
    }

    if (levelIndex == 2) {
        // Вестибюль: полированный мрамор в шахматку.
        bool darkTile = ((tx / 2 + ty) % 2 == 0);
        SDL_Color ftile = darkTile ? SDL_Color{76, 68, 60, 255}
                                   : SDL_Color{112, 102, 88, 255};
        SDL_SetRenderDrawColor(ren, ftile.r, ftile.g, ftile.b, 255);
        SDL_Rect fr{x, y, 16, 16};
        SDL_RenderFillRect(ren, &fr);
        SDL_SetRenderDrawColor(ren, 46, 40, 36, 255);
        SDL_RenderDrawLine(ren, x, y + 15, x + 15, y + 15);
        SDL_RenderDrawLine(ren, x + 15, y, x + 15, y + 15);
        SDL_SetRenderDrawColor(ren, 148, 138, 120, 90);
        SDL_RenderDrawLine(ren, x + 1, y + 1, x + 14, y + 1);
        if ((hash2(tx, ty) >> 4) % 4 == 0) {
            SDL_SetRenderDrawColor(ren, 94, 86, 74, 140);
            SDL_RenderDrawLine(ren, x + 3, y + 4, x + 8, y + 10);
        }
        return;
    }

    if (levelIndex == 3) {
        // Парковка: тёмный асфальт с белой разметкой.
        uint32_t fh = hash2(tx, ty);
        int fv = static_cast<int>(fh % 8);
        SDL_Color abase{static_cast<Uint8>(40 + fv),
                        static_cast<Uint8>(40 + fv),
                        static_cast<Uint8>(46 + fv), 255};
        SDL_SetRenderDrawColor(ren, abase.r, abase.g, abase.b, 255);
        SDL_Rect fr{x, y, 16, 16};
        SDL_RenderFillRect(ren, &fr);
        SDL_SetRenderDrawColor(ren, 68, 68, 76, 200);
        SDL_Rect atop{x, y, 16, 2};
        SDL_RenderFillRect(ren, &atop);
        SDL_SetRenderDrawColor(ren, 88, 88, 94, 90);
        SDL_Rect aline{x, y, 16, 1};
        SDL_RenderFillRect(ren, &aline);
        if ((fh >> 4) % 4 == 0) {
            SDL_SetRenderDrawColor(ren, 26, 26, 30, 140);
            SDL_Rect spot{x + 3 + static_cast<int>(fh % 6), y + 5,
                          3, 2};
            SDL_RenderFillRect(ren, &spot);
        }
        if (tx % 4 == 0) {
            SDL_SetRenderDrawColor(ren, 196, 194, 170, 120);
            SDL_Rect mark{x + 7, y + 4, 2, 12};
            SDL_RenderFillRect(ren, &mark);
        }
        return;
    }

    SDL_SetRenderDrawColor(ren, 62, 40, 29, 255);
    SDL_Rect r{x, y, 16, 16};
    SDL_RenderFillRect(ren, &r);
    SDL_SetRenderDrawColor(ren, 112, 74, 44, 255);
    SDL_Rect top{x, y, 16, 4};
    SDL_RenderFillRect(ren, &top);
    SDL_SetRenderDrawColor(ren, 146, 100, 58, 255);
    SDL_Rect shine{x, y, 16, 1};
    SDL_RenderFillRect(ren, &shine);

    SDL_SetRenderDrawColor(ren, 44, 28, 22, 255);
    int off = (ty % 2 == 0) ? 4 : 12;
    SDL_RenderDrawLine(ren, x + off, y + 4, x + off, y + 15);
    SDL_SetRenderDrawColor(ren, 24, 16, 14, 150);
    SDL_Rect bottom{x, y + 13, 16, 3};
    SDL_RenderFillRect(ren, &bottom);
}

void Game::drawTableTile(int x, int y, int tx, int ty)
{
    if (levelIndex == 1) {
        // Технический этаж: вместо стола — тяжёлый металлический ящик.
        SDL_SetRenderDrawColor(ren, 62, 66, 62, 255);
        SDL_Rect cr{x, y, 16, 16};
        SDL_RenderFillRect(ren, &cr);
        if (lvl.at(tx, ty - 1) != 't') {
            SDL_SetRenderDrawColor(ren, 104, 108, 96, 255);
            SDL_Rect ctop{x, y, 16, 5};
            SDL_RenderFillRect(ren, &ctop);
            SDL_SetRenderDrawColor(ren, 138, 142, 126, 255);
            SDL_Rect cline{x, y, 16, 1};
            SDL_RenderFillRect(ren, &cline);
        }
        if (lvl.at(tx, ty + 1) != 't') {
            SDL_SetRenderDrawColor(ren, 40, 44, 42, 255);
            SDL_Rect cshadow{x, y + 13, 16, 3};
            SDL_RenderFillRect(ren, &cshadow);
            SDL_SetRenderDrawColor(ren, 48, 52, 48, 255);
            SDL_Rect clegL{x + 2, y + 5, 3, 11};
            SDL_Rect clegR{x + 11, y + 5, 3, 11};
            SDL_RenderFillRect(ren, &clegL);
            SDL_RenderFillRect(ren, &clegR);
        }
        SDL_SetRenderDrawColor(ren, 82, 86, 80, 160);
        SDL_Rect crib{x + 3, y + 8, 10, 1};
        SDL_RenderFillRect(ren, &crib);
        SDL_SetRenderDrawColor(ren, 152, 156, 140, 140);
        for (int i = 0; i < 2; ++i) {
            SDL_Rect rivet{x + 3 + i * 9, y + 11, 1, 1};
            SDL_RenderFillRect(ren, &rivet);
        }
        return;
    }

    if (levelIndex == 2) {
        // Вестибюль: стойка ресепшена из тёмного дуба с латунной кромкой.
        SDL_SetRenderDrawColor(ren, 62, 40, 28, 255);
        SDL_Rect cr{x, y, 16, 16};
        SDL_RenderFillRect(ren, &cr);
        if (lvl.at(tx, ty - 1) != 't') {
            SDL_SetRenderDrawColor(ren, 128, 96, 54, 255);
            SDL_Rect ctop{x, y, 16, 5};
            SDL_RenderFillRect(ren, &ctop);
            SDL_SetRenderDrawColor(ren, 178, 142, 78, 255);
            SDL_Rect cline{x, y, 16, 1};
            SDL_RenderFillRect(ren, &cline);
            SDL_SetRenderDrawColor(ren, 84, 58, 36, 255);
            SDL_Rect cedg{x, y + 5, 16, 1};
            SDL_RenderFillRect(ren, &cedg);
        }
        if (lvl.at(tx, ty + 1) != 't') {
            SDL_SetRenderDrawColor(ren, 40, 26, 20, 255);
            SDL_Rect cshadow{x, y + 13, 16, 3};
            SDL_RenderFillRect(ren, &cshadow);
            SDL_SetRenderDrawColor(ren, 48, 32, 22, 255);
            SDL_Rect clegL{x + 2, y + 6, 3, 10};
            SDL_Rect clegR{x + 11, y + 6, 3, 10};
            SDL_RenderFillRect(ren, &clegL);
            SDL_RenderFillRect(ren, &clegR);
        }
        SDL_SetRenderDrawColor(ren, 148, 116, 64, 120);
        SDL_Rect crib{x + 3, y + 9, 10, 1};
        SDL_RenderFillRect(ren, &crib);
        fillCircle(x + 8, y + 8, 1, SDL_Color{190, 150, 84, 255});
        return;
    }

    if (levelIndex == 3) {
        // Парковка: тяжёлый бетонный блок с жёлто-чёрной полосой.
        uint32_t ch = hash2(tx, ty);
        int cv = static_cast<int>(ch % 8);
        SDL_Color cbase{static_cast<Uint8>(92 + cv),
                        static_cast<Uint8>(90 + cv),
                        static_cast<Uint8>(84 + cv), 255};
        SDL_SetRenderDrawColor(ren, cbase.r, cbase.g, cbase.b, 255);
        SDL_Rect cr{x, y, 16, 16};
        SDL_RenderFillRect(ren, &cr);

        // Верхняя кромка блока и нижняя тень.
        if (lvl.at(tx, ty - 1) != 't') {
            SDL_SetRenderDrawColor(ren, 142, 138, 128, 255);
            SDL_Rect ctop{x, y, 16, 5};
            SDL_RenderFillRect(ren, &ctop);
            SDL_SetRenderDrawColor(ren, 176, 172, 158, 255);
            SDL_Rect cline{x, y, 16, 1};
            SDL_RenderFillRect(ren, &cline);
        }
        if (lvl.at(tx, ty + 1) != 't') {
            SDL_SetRenderDrawColor(ren, 54, 52, 50, 255);
            SDL_Rect cshadow{x, y + 13, 16, 3};
            SDL_RenderFillRect(ren, &cshadow);
            // Полосатая предупреждающая лента у основания.
            for (int k = 0; k < 16; k += 8) {
                SDL_SetRenderDrawColor(ren, 196, 158, 42, 230);
                SDL_Rect warn{x + k, y + 8, 4, 5};
                SDL_RenderFillRect(ren, &warn);
                SDL_SetRenderDrawColor(ren, 34, 32, 30, 230);
                SDL_Rect warn2{x + k + 4, y + 8, 4, 5};
                SDL_RenderFillRect(ren, &warn2);
            }
        }

        SDL_SetRenderDrawColor(ren, 70, 68, 64, 180);
        SDL_RenderDrawLine(ren, x, y + 7, x + 15, y + 7);
        SDL_SetRenderDrawColor(ren, 66, 64, 62, 160);
        SDL_RenderDrawLine(ren, x + 15, y, x + 15, y + 15);
        SDL_SetRenderDrawColor(ren, 132, 128, 118, 130);
        SDL_Rect cchip{x + 3, y + 10, 4, 1};
        SDL_RenderFillRect(ren, &cchip);
        return;
    }

    SDL_SetRenderDrawColor(ren, 76, 47, 30, 255);
    SDL_Rect r{x, y, 16, 16};
    SDL_RenderFillRect(ren, &r);
    if (lvl.at(tx, ty - 1) != 't') {
        SDL_SetRenderDrawColor(ren, 138, 88, 48, 255);
        SDL_Rect top{x, y, 16, 5};
        SDL_RenderFillRect(ren, &top);
        SDL_SetRenderDrawColor(ren, 176, 120, 68, 255);
        SDL_Rect line{x, y, 16, 1};
        SDL_RenderFillRect(ren, &line);
    }
    if (lvl.at(tx, ty + 1) != 't') {
        SDL_SetRenderDrawColor(ren, 50, 30, 22, 255);
        SDL_Rect shadow{x, y + 13, 16, 3};
        SDL_RenderFillRect(ren, &shadow);
        SDL_SetRenderDrawColor(ren, 60, 38, 26, 255);
        SDL_Rect legL{x + 2, y + 5, 3, 11};
        SDL_Rect legR{x + 11, y + 5, 3, 11};
        SDL_RenderFillRect(ren, &legL);
        SDL_RenderFillRect(ren, &legR);
    }
    SDL_SetRenderDrawColor(ren, 102, 64, 38, 120);
    SDL_Rect grain{x + 3, y + 8, 10, 1};
    SDL_RenderFillRect(ren, &grain);
}

void Game::drawStepTile(int x, int y, int tx, int ty)
{
    (void)ty;
    if (levelIndex == 1) {
        // Технический этаж: стальная ступень с жёлтой кромкой.
        SDL_SetRenderDrawColor(ren, 78, 84, 86, 255);
        SDL_Rect sboard{x, y, 16, 7};
        SDL_RenderFillRect(ren, &sboard);
        SDL_SetRenderDrawColor(ren, 128, 134, 136, 255);
        SDL_Rect stop{x, y, 16, 2};
        SDL_RenderFillRect(ren, &stop);
        SDL_SetRenderDrawColor(ren, 170, 132, 40, 255);
        SDL_Rect sstripe{x, y + 2, 16, 1};
        SDL_RenderFillRect(ren, &sstripe);
        SDL_SetRenderDrawColor(ren, 42, 46, 48, 255);
        SDL_Rect slow{x, y + 6, 16, 2};
        SDL_RenderFillRect(ren, &slow);
        if (lvl.at(tx - 1, ty) != 'S') {
            SDL_SetRenderDrawColor(ren, 36, 40, 42, 255);
            SDL_Rect scap{x, y, 2, 8};
            SDL_RenderFillRect(ren, &scap);
        }
        if (lvl.at(tx + 1, ty) != 'S') {
            SDL_SetRenderDrawColor(ren, 36, 40, 42, 255);
            SDL_Rect scap{x + 14, y, 2, 8};
            SDL_RenderFillRect(ren, &scap);
        }
        return;
    }

    if (levelIndex == 2) {
        // Вестибюль: мраморный карниз-платформа с золотой кромкой.
        SDL_SetRenderDrawColor(ren, 44, 38, 34, 255);
        SDL_Rect sbase{x, y + 7, 16, 3};
        SDL_RenderFillRect(ren, &sbase);
        SDL_SetRenderDrawColor(ren, 138, 128, 112, 255);
        SDL_Rect sboard{x, y, 16, 7};
        SDL_RenderFillRect(ren, &sboard);
        SDL_SetRenderDrawColor(ren, 188, 178, 158, 255);
        SDL_Rect stop{x, y, 16, 2};
        SDL_RenderFillRect(ren, &stop);
        SDL_SetRenderDrawColor(ren, 170, 134, 66, 255);
        SDL_Rect strim{x, y + 3, 16, 1};
        SDL_RenderFillRect(ren, &strim);
        SDL_SetRenderDrawColor(ren, 106, 98, 84, 255);
        SDL_Rect svein{x + 3, y + 5, 9, 1};
        SDL_RenderFillRect(ren, &svein);
        if (lvl.at(tx - 1, ty) != 'S') {
            SDL_SetRenderDrawColor(ren, 78, 70, 60, 255);
            SDL_Rect scap{x, y, 2, 10};
            SDL_RenderFillRect(ren, &scap);
        }
        if (lvl.at(tx + 1, ty) != 'S') {
            SDL_SetRenderDrawColor(ren, 78, 70, 60, 255);
            SDL_Rect scap{x + 14, y, 2, 10};
            SDL_RenderFillRect(ren, &scap);
        }
        return;
    }

    if (levelIndex == 3) {
        // Парковка: бетонный бортик с металлической кромкой и разметкой.
        SDL_SetRenderDrawColor(ren, 30, 32, 34, 255);
        SDL_Rect sbase{x, y + 7, 16, 3};
        SDL_RenderFillRect(ren, &sbase);
        SDL_SetRenderDrawColor(ren, 84, 88, 92, 255);
        SDL_Rect sboard{x, y, 16, 7};
        SDL_RenderFillRect(ren, &sboard);
        SDL_SetRenderDrawColor(ren, 128, 134, 138, 255);
        SDL_Rect stop{x, y, 16, 2};
        SDL_RenderFillRect(ren, &stop);
        SDL_SetRenderDrawColor(ren, 196, 158, 48, 255);
        SDL_Rect sstripe{x + 2, y + 4, 5, 1};
        SDL_RenderFillRect(ren, &sstripe);
        SDL_SetRenderDrawColor(ren, 58, 62, 64, 255);
        SDL_Rect slow{x, y + 6, 16, 2};
        SDL_RenderFillRect(ren, &slow);
        if (lvl.at(tx - 1, ty) != 'S') {
            SDL_SetRenderDrawColor(ren, 54, 58, 60, 255);
            SDL_Rect scap{x, y, 2, 10};
            SDL_RenderFillRect(ren, &scap);
        }
        if (lvl.at(tx + 1, ty) != 'S') {
            SDL_SetRenderDrawColor(ren, 54, 58, 60, 255);
            SDL_Rect scap{x + 14, y, 2, 10};
            SDL_RenderFillRect(ren, &scap);
        }
        return;
    }

    // Широкая деревянная ступень: доска у верхней кромки тайла.
    SDL_SetRenderDrawColor(ren, 92, 58, 34, 255);
    SDL_Rect board{x, y, 16, 7};
    SDL_RenderFillRect(ren, &board);
    SDL_SetRenderDrawColor(ren, 150, 100, 58, 255);
    SDL_Rect topEdge{x, y, 16, 2};
    SDL_RenderFillRect(ren, &topEdge);
    SDL_SetRenderDrawColor(ren, 60, 38, 24, 255);
    SDL_Rect lowEdge{x, y + 6, 16, 2};
    SDL_RenderFillRect(ren, &lowEdge);

    // Торец с одной стороны ступени — как у настоящей лестницы.
    if (lvl.at(tx - 1, ty) != 'S') {
        SDL_SetRenderDrawColor(ren, 44, 28, 18, 255);
        SDL_Rect cap{x, y, 2, 8};
        SDL_RenderFillRect(ren, &cap);
    }
    if (lvl.at(tx + 1, ty) != 'S') {
        SDL_SetRenderDrawColor(ren, 44, 28, 18, 255);
        SDL_Rect cap{x + 14, y, 2, 8};
        SDL_RenderFillRect(ren, &cap);
    }
    if (lvl.at(tx, ty - 1) == 'S' || lvl.at(tx, ty + 1) == 'S') {
        SDL_SetRenderDrawColor(ren, 74, 46, 28, 210);
        SDL_Rect grain{x + 3, y + 3, 10, 1};
        SDL_RenderFillRect(ren, &grain);
    }
}

void Game::drawLadder(int x, int y)
{
    if (levelIndex == 1) {
        // Технический этаж: сварная стальная лестница.
        SDL_SetRenderDrawColor(ren, 96, 102, 104, 255);
        SDL_Rect mrailL{x + 4, y, 2, 16};
        SDL_Rect mrailR{x + 11, y, 2, 16};
        SDL_RenderFillRect(ren, &mrailL);
        SDL_RenderFillRect(ren, &mrailR);
        SDL_SetRenderDrawColor(ren, 138, 144, 146, 255);
        SDL_Rect mrung1{x + 4, y + 3, 9, 2};
        SDL_Rect mrung2{x + 4, y + 10, 9, 2};
        SDL_RenderFillRect(ren, &mrung1);
        SDL_RenderFillRect(ren, &mrung2);
        SDL_SetRenderDrawColor(ren, 56, 60, 62, 140);
        SDL_Rect mshade{x + 12, y, 1, 16};
        SDL_RenderFillRect(ren, &mshade);
        return;
    }

    if (levelIndex == 2) {
        // Вестибюль: латунная лестница.
        SDL_SetRenderDrawColor(ren, 142, 112, 58, 255);
        SDL_Rect railL{x + 4, y, 2, 16};
        SDL_Rect railR{x + 11, y, 2, 16};
        SDL_RenderFillRect(ren, &railL);
        SDL_RenderFillRect(ren, &railR);
        SDL_SetRenderDrawColor(ren, 186, 152, 82, 255);
        SDL_Rect rung1{x + 4, y + 3, 9, 2};
        SDL_Rect rung2{x + 4, y + 10, 9, 2};
        SDL_RenderFillRect(ren, &rung1);
        SDL_RenderFillRect(ren, &rung2);
        SDL_SetRenderDrawColor(ren, 84, 62, 32, 140);
        SDL_Rect shadeL{x + 12, y, 1, 16};
        SDL_RenderFillRect(ren, &shadeL);
        return;
    }

    SDL_SetRenderDrawColor(ren, 132, 92, 50, 255);
    SDL_Rect railL{x + 4, y, 2, 16};
    SDL_Rect railR{x + 11, y, 2, 16};
    SDL_RenderFillRect(ren, &railL);
    SDL_RenderFillRect(ren, &railR);
    SDL_SetRenderDrawColor(ren, 174, 126, 70, 255);
    SDL_Rect rung1{x + 4, y + 3, 9, 2};
    SDL_Rect rung2{x + 4, y + 10, 9, 2};
    SDL_RenderFillRect(ren, &rung1);
    SDL_RenderFillRect(ren, &rung2);
    SDL_SetRenderDrawColor(ren, 70, 44, 24, 120);
    SDL_Rect shadeL{x + 12, y, 1, 16};
    SDL_RenderFillRect(ren, &shadeL);
}

void Game::drawPitTile(int x, int y, int tx, int ty)
{
    bool edgeL = (tx > 0 && lvl.at(tx - 1, ty) == '=');
    bool edgeR = (tx + 1 < lvl.w && lvl.at(tx + 1, ty) == '=');
    if (!edgeL && !edgeR) return;

    if (levelIndex == 2) {
        // Мраморный пол вестибюля обрывается в тёмную шахту.
        SDL_SetRenderDrawColor(ren, 24, 20, 22, 255);
        SDL_Rect top{x, y, 16, 5};
        SDL_RenderFillRect(ren, &top);
        SDL_SetRenderDrawColor(ren, 15, 13, 15, 255);
        SDL_Rect mid{x, y + 5, 16, 5};
        SDL_RenderFillRect(ren, &mid);
        SDL_SetRenderDrawColor(ren, 7, 6, 8, 255);
        SDL_Rect deep{x, y + 10, 16, 6};
        SDL_RenderFillRect(ren, &deep);
        SDL_SetRenderDrawColor(ren, 96, 88, 76, 255);
        if (edgeL) {
            SDL_Rect e{x, y, 3, 4};
            SDL_Rect e2{x + 3, y + 1, 2, 2};
            SDL_RenderFillRect(ren, &e);
            SDL_RenderFillRect(ren, &e2);
        }
        if (edgeR) {
            SDL_Rect e{x + 13, y, 3, 4};
            SDL_Rect e2{x + 11, y + 1, 2, 2};
            SDL_RenderFillRect(ren, &e);
            SDL_RenderFillRect(ren, &e2);
        }
        return;
    }

    // Провал в полу: тёмный колодец с обломанными краями.
    SDL_SetRenderDrawColor(ren, 20, 14, 18, 255);
    SDL_Rect top{x, y, 16, 5};
    SDL_RenderFillRect(ren, &top);
    SDL_SetRenderDrawColor(ren, 13, 9, 12, 255);
    SDL_Rect mid{x, y + 5, 16, 5};
    SDL_RenderFillRect(ren, &mid);
    SDL_SetRenderDrawColor(ren, 6, 4, 7, 255);
    SDL_Rect deep{x, y + 10, 16, 6};
    SDL_RenderFillRect(ren, &deep);

    // тусклые кирпичи на дне
    SDL_SetRenderDrawColor(ren, 42, 28, 24, 255);
    SDL_Rect b1{x + 3, y + 12, 4, 2};
    SDL_Rect b2{x + 10, y + 13, 3, 2};
    SDL_RenderFillRect(ren, &b1);
    SDL_RenderFillRect(ren, &b2);

    // обломанные доски по краям
    SDL_SetRenderDrawColor(ren, 104, 66, 38, 255);
    if (edgeL) {
        SDL_Rect e{x, y, 2, 4};
        SDL_Rect e2{x + 2, y, 1, 2};
        SDL_RenderFillRect(ren, &e);
        SDL_RenderFillRect(ren, &e2);
    }
    if (edgeR) {
        SDL_Rect e{x + 14, y, 2, 4};
        SDL_Rect e2{x + 13, y, 1, 2};
        SDL_RenderFillRect(ren, &e);
        SDL_RenderFillRect(ren, &e2);
    }
}

void Game::drawRugs()
{
    // На техническом этаже вместо ковров — жёлто-чёрная разметка.
    if (levelIndex == 1) {
        for (const RectF& rug : rugs) {
            fillW(rug.x, rug.y, rug.w, rug.h, SDL_Color{134, 102, 30, 235});
            for (float px = rug.x + 2; px < rug.x + rug.w - 2; px += 10)
                fillW(px, rug.y, 5, rug.h, SDL_Color{30, 28, 24, 235});
            fillW(rug.x, rug.y, rug.w, 1, SDL_Color{196, 160, 70, 200});
        }
        return;
    }

    for (const RectF& rug : rugs) {
        fillW(rug.x, rug.y, rug.w, rug.h, SDL_Color{108, 30, 38, 235});
        fillW(rug.x + 2, rug.y + 1, rug.w - 4, rug.h - 2, SDL_Color{132, 42, 48, 235});
        fillW(rug.x + 3, rug.y + 2, rug.w - 6, 1, SDL_Color{214, 168, 86, 160});
        fillW(rug.x, rug.y, rug.w, 1, SDL_Color{196, 152, 80, 190});
        for (float px = rug.x + 12; px < rug.x + rug.w - 8; px += 22) {
            fillW(px, rug.y + 2, 2, 2, SDL_Color{198, 160, 88, 120});
        }
    }
}

} // namespace kolek
