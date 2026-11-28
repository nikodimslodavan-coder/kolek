#include "game/game.h"
#include "core/math.h"
#include "data/levels.h"

namespace kolek {

void Game::resetWorld(int index)
{
    secretDoorActive = false;
    secretDoor = RectF{};
    bossDefeated = false;

    levelIndex = std::max(0, std::min(index, LEVEL_COUNT - 1));
    lvl.load(LEVEL_MAPS[levelIndex]);

    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = lvl.spawnPX;
    player.y = lvl.spawnPY;
    player.facing = 1;
    player.airJumps = 1;

    // кубки
    cups.clear();
    for (int ty = 0; ty < lvl.h; ++ty)
        for (int tx = 0; tx < lvl.w; ++tx)
            if (lvl.at(tx, ty) == 'K') cups.push_back(CupItem{tx, ty, false});
    cupsGot = 0;
    if (levelIndex == 0) score = 0;
    popups.clear();

    // дверь
    door = RectF{};
    int dx0 = lvl.w, dx1 = -1, dy0 = lvl.h, dy1 = -1;
    for (int ty = 0; ty < lvl.h; ++ty) {
        for (int tx = 0; tx < lvl.w; ++tx) {
            if (lvl.at(tx, ty) == 'D') {
                dx0 = std::min(dx0, tx);
                dx1 = std::max(dx1, tx);
                dy0 = std::min(dy0, ty);
                dy1 = std::max(dy1, ty);
            }
        }
    }
    if (dx1 >= 0) {
        door.x = dx0 * TILE;
        door.y = dy0 * TILE;
        door.w = (dx1 - dx0 + 1) * TILE;
        door.h = (dy1 - dy0 + 1) * TILE;
    }
    // На парковке роль двери играет джип: зона посадки у его борта.
    if (levelIndex == 3) {
        door.x = JEEP_X;
        door.y = 17 * TILE - JEEP_DOOR_H;
        door.w = JEEP_W;
        door.h = JEEP_DOOR_H;
    }

    // украшения: девять случайных тайлов на твёрдом полу, по три каждого вида.
    // На парковке с боссом наград нет — только бой.
    jewelry.clear();
    if (levelIndex != 3) {
    std::vector<RectF> spots;
    std::vector<RectF> raised;
    for (int ty = 1; ty < lvl.h; ++ty) {
        for (int tx = 0; tx < lvl.w; ++tx) {
            if (lvl.at(tx, ty) != '.') continue;
            if (!lvl.solid(tx, ty + 1)) continue;
            float jx = tx * 16.0f + 3.0f;
            float jy = ty * 16.0f + 4.0f;
            if (std::fabs(jx - player.x) < 64.0f &&
                std::fabs(jy - player.y) < 96.0f) continue;
            if (door.w > 0.0f &&
                overlapRect(jx - 8.0f, jy - 8.0f, 28.0f, 24.0f,
                            door.x - 24.0f, door.y - 24.0f,
                            door.w + 48.0f, door.h + 48.0f)) continue;
            bool nearCup = false;
            for (const CupItem& c : cups)
                if (std::abs(c.tx - tx) <= 2 && std::abs(c.ty - ty) <= 2)
                    nearCup = true;
            if (nearCup) continue;
            auto nearAny = [&](const std::vector<RectF>& v) {
                for (const RectF& s : v)
                    if (std::fabs(s.x - jx) < 48.0f &&
                        std::fabs(s.y - jy) < 28.0f) return true;
                return false;
            };
            if (nearAny(spots) || nearAny(raised)) continue;
            RectF spot{jx, jy, 11.0f, 10.0f};
            // В вестибюле часть наград лежит на верхних платформах: за ними
            // нужно прыгать, поэтому такие места выбираем в первую очередь.
            if (levelIndex == 2 && ty <= lvl.h - 8)
                raised.push_back(spot);
            else
                spots.push_back(spot);
        }
    }
    auto shuffleSpots = [&](std::vector<RectF>& v) {
        for (size_t i = v.size(); i > 1; --i) {
            size_t j = static_cast<size_t>(rndf(rngState, 0.0f,
                                                static_cast<float>(i)));
            if (j >= i) j = i - 1;
            std::swap(v[i - 1], v[j]);
        }
    };
    shuffleSpots(spots);
    shuffleSpots(raised);

    std::vector<RectF> chosen;
    size_t ri = 0;
    if (levelIndex == 2)
        for (; ri < raised.size() && chosen.size() < 3; ++ri)
            chosen.push_back(raised[ri]);
    for (size_t i = 0; i < spots.size() && chosen.size() < 9; ++i)
        chosen.push_back(spots[i]);
    for (; ri < raised.size() && chosen.size() < 9; ++ri)
        chosen.push_back(raised[ri]);

    for (size_t i = 0; i < chosen.size(); ++i) {
        Jewel jw;
        jw.x = chosen[i].x;
        jw.y = chosen[i].y;
        jw.type = static_cast<int>(i % 3);
        jw.phase = rndf(rngState, 0.0f, 6.28f);
        jewelry.push_back(jw);
    }
    }

    // декор стен
    decor.clear();
    auto D = [&](int tx, int ty, DecorType t) { decor.push_back(Decor{tx, ty, static_cast<int>(t)}); };

    if (levelIndex == 0) {
        const int gWin[][2] = {{6,9},{22,9},{43,9},{59,9},{71,9},{86,9},{107,9},{121,9},{137,9},{151,9}};
        for (auto& p : gWin) D(p[0], p[1], DEC_WINDOW);
        const int gPaint[][2] = {{11,10},{18,10},{25,10},{34,10},{64,10},{80,10},{93,10},{112,10},{127,10},{141,10},{148,10}};
        for (int i = 0; i < static_cast<int>(sizeof(gPaint) / sizeof(gPaint[0])); ++i)
            if (i % 2 == 0) D(gPaint[i][0], gPaint[i][1], DEC_PAINTING);
        const int gSconce[][2] = {{2,11},{15,11},{30,11},{32,11},{52,11},{56,11},{75,11},{79,11},
                                  {102,11},{105,11},{131,11},{134,11},{144,11},{146,11},{155,11}};
        for (auto& p : gSconce) D(p[0], p[1], DEC_SCONCE);
        const int gChand[][2] = {{9,8},{28,8},{39,8},{68,8},{95,8},{116,8},{140,8}};
        for (auto& p : gChand) D(p[0], p[1], DEC_CHANDELIER);
        const int gShelf[][2] = {{33,12},{56,12},{81,12},{100,12},{125,12}};
        for (auto& p : gShelf) D(p[0], p[1], DEC_SHELF);
        D(146, 12, DEC_CLOCK);

        const int uWin[][2] = {{5,2},{12,2},{20,2},{33,2},{42,2},{57,2},{64,2},{76,2},
                               {91,2},{99,2},{112,2},{120,2},{129,2},{146,2},{154,2}};
        for (auto& p : uWin) D(p[0], p[1], DEC_WINDOW);
        const int uPaint[][2] = {{8,5},{16,5},{24,5},{31,5},{53,5},{68,5},{80,5},{87,5},
                                 {103,5},{116,5},{126,5},{138,5},{150,5}};
        for (int i = 0; i < static_cast<int>(sizeof(uPaint) / sizeof(uPaint[0])); ++i)
            if (i % 2 == 0) D(uPaint[i][0], uPaint[i][1], DEC_PAINTING);
        const int uSconce[][2] = {{2,6},{10,6},{18,6},{29,6},{35,6},{51,6},{60,6},{72,6},
                                  {85,6},{97,6},{110,6},{123,6},{131,6},{144,6},{156,6}};
        for (auto& p : uSconce) D(p[0], p[1], DEC_SCONCE);
        const int uChand[][2] = {{7,0},{22,0},{37,0},{50,0},{63,0},{83,0},{95,0},{117,0},{127,0},{150,0}};
        for (auto& p : uChand) D(p[0], p[1], DEC_CHANDELIER);
        const int uShelf[][2] = {{30,5},{55,5},{94,5}};
        for (auto& p : uShelf) D(p[0], p[1], DEC_SHELF);
        D(22, 5, DEC_MIRROR);
        D(66, 5, DEC_MIRROR);
    } else if (levelIndex == 1) {
        // Второй уровень — технический этаж: вместо картин и канделябров
        // развешиваем аварийные лампы, вентиляцию, щиты, трубы и стремянки.
        auto free = [&](int tx, int ty) {
            return lvl.at(tx, ty) == '.';
        };
        const int bandTop[] = {1, 9, 18, 26};     // под перекрытием
        const int bandFloor[] = {8, 15, 25, 32};  // пол этажа
        for (int b = 0; b < 4; ++b) {
            const int top = bandTop[b];
            const int floor = bandFloor[b];
            for (int tx = 3; tx < lvl.w - 3; ++tx) {
                if (tx % 12 == 4 && free(tx, top))
                    D(tx, top, DEC_EMERGENCY);
                if (tx % 17 == 8 && free(tx, top + 1))
                    D(tx, top + 1, DEC_VENT);
                if (tx % 21 == 5 && free(tx, top + 2))
                    D(tx, top + 2, DEC_BOX);
                if (tx % 25 == 15 && free(tx, top + 3))
                    D(tx, top + 3, DEC_SIGN);
                if (tx % 19 == 11 && free(tx, top + 1) && free(tx, top + 2) &&
                    free(tx, top + 3) && free(tx, top + 4))
                    D(tx, top + 1, DEC_PIPE);
                if (tx % 23 == 17 && free(tx, top + 4))
                    D(tx, top + 4, DEC_CABLE);
                if (tx % 15 == 7 && free(tx, floor - 2) && free(tx, floor - 1))
                    D(tx, floor - 2, DEC_STEPLADDER);
            }
        }
    } else if (levelIndex == 2) {
        // Третий уровень — парадный вестибюль: люстры, портреты, бра,
        // зеркала и напольные часы.
        auto free = [&](int tx, int ty) {
            return lvl.at(tx, ty) == '.';
        };
        for (int tx = 3; tx < lvl.w - 3; ++tx) {
            if (tx % 13 == 5 && free(tx, 1))
                D(tx, 1, DEC_CHANDELIER);
            if (tx % 9 == 3 && free(tx, 8))
                D(tx, 8, DEC_SCONCE);
            if (tx % 15 == 7 && free(tx, 7))
                D(tx, 7, DEC_PAINTING);
            if (tx % 27 == 16 && free(tx, 5))
                D(tx, 5, DEC_WINDOW);
            if (tx % 34 == 11 && free(tx, 4))
                D(tx, 4, DEC_MIRROR);
            if (tx % 41 == 23 && free(tx, 9))
                D(tx, 9, DEC_CLOCK);
        }
    }

    // Три этажа: базовый декор второго этажа повторяем вниз на 17 и 34 ряда.
    // У технического этажа декор уже расставлен по всем четырём полосам,
    // поэтому дублировать его не нужно.
    if (levelIndex == 0) {
        const size_t n = decor.size();
        // первый этаж — полная копия
        for (size_t i = 0; i < n; ++i)
            decor.push_back(Decor{decor[i].tx, decor[i].ty + 17, decor[i].type});
        // подвал — без окон, портретов и зеркал; он вдвое ниже, поэтому
        // украшения, вылезающие за его пол, отбрасываем
        for (size_t i = 0; i < n; ++i) {
            int t = decor[i].type;
            if (t == DEC_WINDOW || t == DEC_PAINTING || t == DEC_MIRROR) continue;
            int by = decor[i].ty + 34;
            if (by > lvl.h - 3) continue;
            decor.push_back(Decor{decor[i].tx, by, t});
        }
    }

    // Декор не должен накладываться на ступени, лестницы, дверь, кубки и
    // другие украшения. У каждого элемента считаем реальную рамку рисунка
    // в пикселях и оставляем только те, что ни с чем не пересекаются.
    auto decorPixels = [](const Decor& d) -> SDL_Rect {
        switch (d.type) {
            case DEC_WINDOW:     return SDL_Rect{d.tx * 16 - 3, d.ty * 16 - 2, 40, 57};
            case DEC_PAINTING:   return SDL_Rect{d.tx * 16 - 2, d.ty * 16 - 2, 32, 38};
            case DEC_SCONCE:     return SDL_Rect{d.tx * 16 + 4, d.ty * 16 - 2, 8, 18};
            case DEC_CHANDELIER: return SDL_Rect{d.tx * 16 - 6, d.ty * 16 + 16, 28, 19};
            case DEC_SHELF:      return SDL_Rect{d.tx * 16, d.ty * 16 - 32, 48, 48};
            case DEC_MIRROR:     return SDL_Rect{d.tx * 16 - 2, d.ty * 16, 26, 37};
            case DEC_CLOCK:      return SDL_Rect{d.tx * 16 - 2, d.ty * 16 - 2, 26, 50};
            case DEC_EMERGENCY:  return SDL_Rect{d.tx * 16 - 2, d.ty * 16 - 2, 20, 18};
            case DEC_STEPLADDER: return SDL_Rect{d.tx * 16 - 2, d.ty * 16 - 2, 20, 34};
            case DEC_PIPE:       return SDL_Rect{d.tx * 16 - 4, d.ty * 16 - 2, 24, 56};
            case DEC_VENT:       return SDL_Rect{d.tx * 16 - 1, d.ty * 16 - 2, 18, 16};
            case DEC_BOX:        return SDL_Rect{d.tx * 16 - 2, d.ty * 16 - 2, 22, 34};
            case DEC_SIGN:       return SDL_Rect{d.tx * 16 - 2, d.ty * 16 - 2, 30, 18};
            case DEC_CABLE:      return SDL_Rect{d.tx * 16 - 2, d.ty * 16 - 8, 20, 28};
            default:             return SDL_Rect{d.tx * 16, d.ty * 16, 16, 16};
        }
    };
    auto hitRect = [](const SDL_Rect& a, const SDL_Rect& b) {
        return a.x < b.x + b.w && b.x < a.x + a.w &&
               a.y < b.y + b.h && b.y < a.y + a.h;
    };

    std::vector<Decor> keptDecor;
    std::vector<SDL_Rect> keptRects;
    for (const Decor& d : decor) {
        SDL_Rect r = decorPixels(d);
        bool blocked = false;

        // Ступени и лестницы.
        for (int ty = r.y / TILE; ty <= (r.y + r.h - 1) / TILE && !blocked; ++ty) {
            for (int tx = r.x / TILE; tx <= (r.x + r.w - 1) / TILE; ++tx) {
                char c = lvl.at(tx, ty);
                if (c == 'S' || c == 'L') {
                    blocked = true;
                    break;
                }
            }
        }

        // Дверь.
        if (!blocked && door.w > 0.0f) {
            SDL_Rect dr{static_cast<int>(door.x), static_cast<int>(door.y),
                        static_cast<int>(door.w), static_cast<int>(door.h)};
            if (hitRect(r, dr)) blocked = true;
        }

        // Кубки.
        for (const CupItem& c : cups) {
            if (blocked) break;
            SDL_Rect cr{c.tx * 16, c.ty * 16, 16, 16};
            if (hitRect(r, cr)) blocked = true;
        }

        // Другой декор.
        for (const SDL_Rect& q : keptRects) {
            if (hitRect(r, q)) {
                blocked = true;
                break;
            }
        }

        if (!blocked) {
            keptDecor.push_back(d);
            keptRects.push_back(r);
        }
    }
    decor.swap(keptDecor);

    // ковры
    rugs.clear();
    if (levelIndex == 0) {
        rugs.push_back(RectF{2 * 16.f, 240.f, 12 * 16.f, 6.f});
        rugs.push_back(RectF{61 * 16.f, 240.f, 11 * 16.f, 6.f});
        rugs.push_back(RectF{138 * 16.f, 240.f, 17 * 16.f, 6.f});
        rugs.push_back(RectF{6 * 16.f, 128.f, 7 * 16.f, 5.f});
        rugs.push_back(RectF{56 * 16.f, 128.f, 7 * 16.f, 5.f});
        rugs.push_back(RectF{115 * 16.f, 128.f, 8 * 16.f, 5.f});
    } else if (levelIndex == 1) {
        const int groundRugs[] = {4, 44, 76, 124};
        for (int x : groundRugs) rugs.push_back(RectF{x * 16.f, 240.f, 9 * 16.f, 6.f});
        const int upperRugs[] = {6, 36, 80, 120};
        for (int x : upperRugs) rugs.push_back(RectF{x * 16.f, 128.f, 8 * 16.f, 5.f});
    } else if (levelIndex == 2) {
        // Вестибюль: одна длинная ковровая дорожка, разбитая пропастями.
        const int runX[] = {4, 24, 58, 105, 150, 205};
        const int runW[] = {14, 16, 20, 15, 20, 19};
        const float floorTop = (lvl.h - 6) * TILE;
        for (int i = 0; i < 6; ++i)
            rugs.push_back(RectF{runX[i] * 16.f, floorTop, runW[i] * 16.f, 6.f});
    }

    // ковры: этажи повторяются со сдвигом 272 px, подвал — только напольные
    if (levelIndex == 0 || levelIndex == 1) {
        const size_t n = rugs.size();
        for (size_t i = 0; i < n; ++i)
            rugs.push_back(RectF{rugs[i].x, rugs[i].y + 272.0f, rugs[i].w, rugs[i].h});
        if (levelIndex == 0) {
            // Подвал укорочен: ковры лежат на его полу, а не на прежнем 49-м ряду.
            const float basementFloorY = (lvl.h - 2) * TILE;
            for (size_t i = 0; i < n; ++i)
                if (rugs[i].y == 240.0f)
                    rugs.push_back(RectF{rugs[i].x, basementFloorY,
                                         rugs[i].w, rugs[i].h});
        }
    }

    // монстры
    monsters.clear();
    for (int ty = 0; ty < lvl.h; ++ty) {
        for (int tx = 0; tx < lvl.w; ++tx) {
            char c = lvl.at(tx, ty);
            if (c != 'z' && c != 'v' && c != 'n' && c != 'd' && c != 'B')
                continue;
            Monster m;
            m.type = (c == 'z') ? 0
                     : (c == 'v') ? 1
                     : (c == 'n') ? 2
                     : (c == 'B') ? 4 : 3;
            m.seed = rndf(rngState, 0.0f, 6.28f);
            m.b.facing = (hash2(tx, ty) & 1u) ? 1 : -1;
            if (m.type == 4) {
                // Босс парковки: гигантский браток в восемь Колянов ростом,
                // стоит у джипа на асфальте и смотрит в сторону игрока.
                m.b.w = BOSS_W;
                m.b.h = BOSS_H;
                m.b.x = tx * 16.0f;
                int floorRow = ty;
                while (floorRow < lvl.h && !lvl.solid(tx, floorRow)) ++floorRow;
                m.b.y = (floorRow < lvl.h)
                            ? (floorRow * 16.0f - m.b.h)
                            : (ty * 16.0f + 16.0f - m.b.h);
                m.b.facing = -1;
                m.hp = m.maxHp = BOSS_HP;
                m.throwCd = rndf(rngState, 0.9f, 1.7f);
            } else if (m.type == 0) {
                // паук: в полтора раза выше прежнего, юркий; живёт на потолке
                // своей комнаты и спускается к Коляну на паутине.
                m.b.w = 16;
                m.b.h = 18;
                m.b.x = tx * 16.0f;
                int ceilRow = ty - 1;
                while (ceilRow >= 0 && !lvl.solid(tx, ceilRow)) --ceilRow;
                m.b.y = (ceilRow >= 0) ? (ceilRow + 1) * 16.0f + 4.0f : 4.0f;
                m.ceilingY = m.b.y;
                m.webAnchorX = m.b.x + m.b.w * 0.5f;
                m.webAnchorY = m.b.y;
                m.hp = m.maxHp = 3;
            } else if (m.type == 3) {
                // доберман: длинный и низкий, хитбокс занижен — проходит
                // под высокими выстрелами и кусает за ноги.
                m.b.w = 24;
                m.b.h = 14;
                m.b.x = tx * 16.0f - 4.0f;
                int floorRow = ty;
                while (floorRow < lvl.h && !lvl.solid(tx, floorRow)) ++floorRow;
                m.b.y = (floorRow < lvl.h) ? (floorRow * 16.0f - m.b.h)
                                           : (ty * 16.0f + 16.0f - m.b.h);
                m.hp = m.maxHp = 2;
            } else {
                // братки: высокие, ищут пол под своей клеткой.
                m.b.w = 14;
                m.b.h = 26;
                m.b.x = tx * 16.0f + 1.0f;
                int floorRow = ty;
                while (floorRow < lvl.h && !lvl.solid(tx, floorRow)) ++floorRow;
                m.b.y = (floorRow < lvl.h) ? (floorRow * 16.0f - m.b.h)
                                           : (ty * 16.0f + 16.0f - m.b.h);
                if (m.type == 1) {
                    m.hp = m.maxHp = 4;
                } else {
                    m.hp = m.maxHp = 3;
                    m.throwCd = rndf(rngState, 0.5f, 2.0f);
                }
            }
            m.homeX = m.b.x;
            m.homeY = m.b.y;
            monsters.push_back(m);
        }
    }

    mag = MAG_SIZE;
    reloading = false;
    shootCd = reloadTimer = standStill = muzzle = gunKick = 0;
    reloadDuration = RELOAD_FULL;
    prevSpacePressed = false;
    jumpEdge = false;

    parts.clear();
    tracers.clear();
    bullets.clear();
    grenades.clear();
    lights.clear();
    checkpoint = RectF{player.x, player.y, player.w, player.h};
    snapCamera();
    shake = hurtFlash = winDelay = 0;
    introTimer = 0;
    state = ST_PLAY;
}

void Game::startGame()
{
    levelIndex = 0;
    kills = 0;
    score = 0;
    lives = START_LIVES;
    deathTimer = 0;
    // Украшения каждый забег лежат по-новому.
    rngState ^= SDL_GetTicks();
    resetWorld(0);
    state = ST_PLAY;
}

} // namespace kolek
