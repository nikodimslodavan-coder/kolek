#include "game/game.h"
#include "core/math.h"
#include "rendering/font.h"
#include "data/levels.h"

namespace kolek {

void Game::drawHUD()
{
    const bool bossLevel = (levelIndex == 3);
    const bool sawmillLevel = (levelIndex == 4);

    if (bossLevel) {
        // На парковке вместо кубков — полоса здоровья босса.
        int bossHp = 0, bossMax = BOSS_HP;
        for (const Monster& bm : monsters) {
            if (bm.type == 4 && !bm.dead) {
                bossHp = bm.hp;
                bossMax = bm.maxHp;
                break;
            }
        }

        SDL_Rect panel{4, 4, 128, 17};
        SDL_SetRenderDrawColor(ren, 16, 8, 12, 155);
        SDL_RenderFillRect(ren, &panel);
        SDL_SetRenderDrawColor(ren, 176, 66, 64, 205);
        SDL_RenderDrawRect(ren, &panel);

        // Крошечная злая голова босса.
        SDL_SetRenderDrawColor(ren, 214, 168, 132, 255);
        SDL_Rect bhead{7, 7, 10, 9};
        SDL_RenderFillRect(ren, &bhead);
        SDL_SetRenderDrawColor(ren, 32, 30, 36, 255);
        SDL_Rect bcap{6, 5, 12, 3};
        SDL_RenderFillRect(ren, &bcap);
        SDL_SetRenderDrawColor(ren, 244, 66, 58, 255);
        SDL_Rect beye{9, 9, 2, 2};
        SDL_Rect beye2{13, 9, 2, 2};
        SDL_RenderFillRect(ren, &beye);
        SDL_RenderFillRect(ren, &beye2);

        pixfont::drawText(ren, 21, 9, "БОСС", 1,
                          SDL_Color{240, 196, 150, 255}, false);

        SDL_SetRenderDrawColor(ren, 88, 48, 48, 180);
        SDL_RenderDrawLine(ren, 55, 7, 55, 17);

        float k = (bossMax > 0)
                      ? clampf(static_cast<float>(bossHp) /
                                   static_cast<float>(bossMax), 0.0f, 1.0f)
                      : 0.0f;
        SDL_SetRenderDrawColor(ren, 40, 22, 24, 230);
        SDL_Rect barBg{60, 8, 40, 6};
        SDL_RenderFillRect(ren, &barBg);
        SDL_SetRenderDrawColor(ren, static_cast<Uint8>(196 + 40 * k), 58, 62, 255);
        SDL_Rect bar{60, 8, static_cast<int>(40.0f * k), 6};
        SDL_RenderFillRect(ren, &bar);
        SDL_SetRenderDrawColor(ren, 120, 92, 92, 160);
        SDL_RenderDrawRect(ren, &barBg);

        std::string hpText = std::to_string(std::max(0, bossHp));
        pixfont::drawText(ren, 104, 9, hpText, 1,
                          SDL_Color{255, 220, 206, 255}, false);

        // Очки — рядом, отдельной узкой панелью.
        SDL_Rect scorePanel{136, 4, 60, 17};
        SDL_SetRenderDrawColor(ren, 10, 8, 12, 145);
        SDL_RenderFillRect(ren, &scorePanel);
        SDL_SetRenderDrawColor(ren, 106, 86, 52, 200);
        SDL_RenderDrawRect(ren, &scorePanel);
        fillTriangle(144, 7, 147, 11, 144, 15, SDL_Color{255, 205, 80, 255});
        fillTriangle(144, 7, 141, 11, 144, 15, SDL_Color{255, 232, 160, 255});
        pixfont::drawText(ren, 151, 9, std::to_string(score), 1,
                          SDL_Color{255, 234, 172, 255}, false);

        // После победы над боссом путь лежит к джипу.
        if (bossDefeated) {
            float pulse = 0.5f + 0.5f * std::sin(timeSec * 5.0f);
            pixfont::drawTextCentered(
                ren, VIEW_W / 2, 26, "К ДЖИПУ!", 1,
                SDL_Color{255, 214, 130,
                          static_cast<Uint8>(170 + 70 * pulse)});
        }
    } else {
        // Верхний левый блок: кубки и очки в одной тонкой рамке.
        bool allCups = cupsGot >= static_cast<int>(cups.size());
        float pulse = allCups ? (0.5f + 0.5f * std::sin(timeSec * 4.5f)) : 0.0f;
        SDL_Rect panel{4, 4, 100, 17};
        SDL_SetRenderDrawColor(ren, 10, 8, 12, 145);
        SDL_RenderFillRect(ren, &panel);
        SDL_SetRenderDrawColor(ren,
                               allCups ? static_cast<Uint8>(160 + pulse * 60) : 110,
                               allCups ? static_cast<Uint8>(126 + pulse * 50) : 86,
                               allCups ? static_cast<Uint8>(58 + pulse * 40) : 48, 200);
        SDL_RenderDrawRect(ren, &panel);

        drawCupIcon(6, 7, std::sin(timeSec * 2.5f) * (allCups ? 1.0f : 0.4f));
        std::string cupText = std::to_string(cupsGot) + "/" + std::to_string(cups.size());
        pixfont::drawText(ren, 21, 9, cupText, 1,
                          allCups ? SDL_Color{255, 224, 120, 255}
                                  : SDL_Color{236, 226, 206, 255}, false);

        SDL_SetRenderDrawColor(ren, 88, 70, 42, 180);
        SDL_RenderDrawLine(ren, 42, 7, 42, 17);

        // Маленькая монетка и счёт.
        fillTriangle(49, 7, 52, 11, 49, 15, SDL_Color{255, 205, 80, 255});
        fillTriangle(49, 7, 46, 11, 49, 15, SDL_Color{255, 232, 160, 255});
        pixfont::drawText(ren, 56, 9, std::to_string(score), 1,
                          SDL_Color{255, 234, 172, 255}, false);
    }

    // Сердца вверху по центру, без рамки.
    for (int i = 0; i < START_LIVES; ++i) {
        int cx = VIEW_W / 2 - 16 + i * 12;
        int cy = 7;
        SDL_Color hc = (i < lives) ? SDL_Color{232, 66, 72, 255}
                                   : SDL_Color{74, 52, 56, 210};
        fillCircle(cx - 2, cy, 2, hc);
        fillCircle(cx + 2, cy, 2, hc);
        fillTriangle(cx - 4, cy, cx + 4, cy, cx, cy + 4, hc);
        if (i < lives) {
            SDL_SetRenderDrawColor(ren, 255, 152, 152, 170);
            SDL_Rect shine{cx - 2, cy - 2, 2, 1};
            SDL_RenderFillRect(ren, &shine);
        }
    }
    std::string floorText = bossLevel
        ? "ПАРКОВКА"
        : sawmillLevel
              ? "ЛЕСОПИЛКА"
              : "ЭТАЖ " + std::to_string(levelIndex + 1) + "/" +
                    std::to_string(LEVEL_COUNT);
    pixfont::drawTextCentered(ren, VIEW_W / 2, 13, floorText, 1,
                              SDL_Color{214, 204, 184, 220}, false);

    // Патроны: крошечная пиктограмма, число и гильзы.
    bool lowAmmo = (mag == 0);
    SDL_Rect ammoPanel{VIEW_W - 92, 4, 88, 17};
    SDL_SetRenderDrawColor(ren, 10, 8, 12, 145);
    SDL_RenderFillRect(ren, &ammoPanel);
    SDL_SetRenderDrawColor(ren, lowAmmo ? 200 : 106, lowAmmo ? 92 : 86,
                           lowAmmo ? 70 : 52, 200);
    SDL_RenderDrawRect(ren, &ammoPanel);

    {
        int ix = ammoPanel.x + 4;
        int iy = ammoPanel.y + 4;
        SDL_SetRenderDrawColor(ren, 104, 68, 36, 255);
        SDL_Rect stock{ix, iy + 3, 4, 4};
        SDL_RenderFillRect(ren, &stock);
        SDL_SetRenderDrawColor(ren, 70, 74, 84, 255);
        SDL_Rect body{ix + 3, iy + 3, 10, 3};
        SDL_RenderFillRect(ren, &body);
        SDL_SetRenderDrawColor(ren, 140, 146, 158, 255);
        SDL_Rect barrel{ix + 12, iy + 3, 5, 2};
        SDL_RenderFillRect(ren, &barrel);
        SDL_SetRenderDrawColor(ren, 120, 82, 44, 255);
        SDL_Rect pump{ix + 8, iy + 6, 4, 2};
        SDL_RenderFillRect(ren, &pump);
    }

    std::string ammoText = std::to_string(mag) + "/" + std::to_string(MAG_SIZE);
    pixfont::drawText(ren, ammoPanel.x + 24, 9, ammoText, 1,
                      lowAmmo ? SDL_Color{230, 120, 90, 255}
                              : SDL_Color{245, 235, 215, 255}, false);

    for (int i = 0; i < MAG_SIZE; ++i) {
        int px = ammoPanel.x + 46 + i * 5;
        int py = ammoPanel.y + 5;
        if (i < mag) {
            SDL_SetRenderDrawColor(ren, 222, 172, 74, 255);
            SDL_Rect shell{px, py, 3, 7};
            SDL_RenderFillRect(ren, &shell);
            SDL_SetRenderDrawColor(ren, 255, 214, 120, 255);
            SDL_Rect tip{px, py, 3, 2};
            SDL_RenderFillRect(ren, &tip);
        } else {
            SDL_SetRenderDrawColor(ren, 70, 58, 46, 220);
            SDL_Rect shell{px, py, 3, 7};
            SDL_RenderDrawRect(ren, &shell);
        }
    }

    if (reloading) {
        float k = reloadDuration > 0.001f
                      ? clampf(1.0f - reloadTimer / reloadDuration, 0.0f, 1.0f)
                      : 1.0f;
        SDL_SetRenderDrawColor(ren, 58, 44, 30, 210);
        SDL_Rect barBg{ammoPanel.x + 4, ammoPanel.y + 13, 80, 2};
        SDL_RenderFillRect(ren, &barBg);
        SDL_SetRenderDrawColor(ren, 240, 190, 90, 255);
        SDL_Rect bar{ammoPanel.x + 4, ammoPanel.y + 13,
                     static_cast<int>(80 * k), 2};
        SDL_RenderFillRect(ren, &bar);
    }

    // Короткая подсказка в начале — тонкая полоска без рамки.
    if (introTimer < 14.0f) {
        float a = introTimer < 11.0f ? 1.0f : clampf((14.0f - introTimer) / 3.0f, 0.0f, 1.0f);
        std::string hint = touch.enabled
            ? "СЛЕВА - ИДТИ И ЛАЗАТЬ   СПРАВА - ПРЫЖОК И ОГОНЬ"
            : "A/D ИДТИ  W/↑ ПРИЦЕЛ ВВЕРХ  S/↓ ВНИЗ  CTRL ВЫСТРЕЛ";
        int tw = pixfont::textWidth(hint, 1);
        SDL_SetRenderDrawColor(ren, 8, 6, 10, static_cast<Uint8>(120 * a));
        SDL_Rect hp{VIEW_W / 2 - tw / 2 - 4, VIEW_H - 13, tw + 8, 10};
        SDL_RenderFillRect(ren, &hp);
        pixfont::drawText(ren, VIEW_W / 2 - tw / 2, VIEW_H - 11, hint, 1,
                          SDL_Color{220, 205, 180, static_cast<Uint8>(210 * a)},
                          false);
    }
}

void Game::drawTouchControls()
{
    if (!touch.enabled) return;
    touch.setVisibleForState(state, exitAction != EXIT_NONE);

    auto button = [&](int b, const char* label, int labelScale) {
        if (!touch.visible(b)) return;
        const SDL_Rect& r = touch.rect[b];
        bool down = touch.held[b];
        SDL_SetRenderDrawColor(ren, 14, 10, 16, down ? 190 : 120);
        SDL_RenderFillRect(ren, &r);
        SDL_SetRenderDrawColor(ren,
                               down ? 255 : 226,
                               down ? 216 : 202,
                               down ? 150 : 176,
                               down ? 245 : 170);
        SDL_RenderDrawRect(ren, &r);
        SDL_Rect inner{r.x + 2, r.y + 2, r.w - 4, r.h - 4};
        SDL_RenderDrawRect(ren, &inner);
        if (label && *label) {
            int th = 7 * labelScale;
            pixfont::drawTextCentered(ren, r.x + r.w / 2,
                                      r.y + (r.h - th) / 2, label, labelScale,
                                      down ? SDL_Color{255, 240, 190, 255}
                                           : SDL_Color{232, 220, 196, 235},
                                      false);
        }
    };

    // В меню выстрел превращается в кнопку подтверждения.
    bool inMenu = (exitAction != EXIT_NONE || state == ST_TITLE || state == ST_SETTINGS ||
                   state == ST_SLIDE || state == ST_WIN ||
                   state == ST_GAMEOVER);
    button(TB_UP, "↑", 2);
    button(TB_DOWN, "↓", 2);
    button(TB_LEFT, "←", 2);
    button(TB_RIGHT, "→", 2);
    button(TB_JUMP, "ПРЫЖОК", 1);
    button(TB_SHOOT, inMenu ? "ОК" : "ОГОНЬ", inMenu ? 2 : 1);
    button(TB_BACK, "МЕНЮ", 1);
    button(TB_RESTART, "ЗАНОВО", 1);
}

void Game::drawExitConfirmation()
{
    SDL_SetRenderDrawColor(ren, 8, 5, 10, 210);
    SDL_RenderFillRect(ren, nullptr);
    SDL_Rect panel{72, 64, 336, 128};
    SDL_SetRenderDrawColor(ren, 28, 21, 30, 255);
    SDL_RenderFillRect(ren, &panel);
    SDL_SetRenderDrawColor(ren, 214, 174, 92, 255);
    SDL_RenderDrawRect(ren, &panel);
    pixfont::drawTextCentered(ren, VIEW_W / 2, 80,
        exitAction == EXIT_GAME ? "ВЫЙТИ ИЗ ИГРЫ?" : "ВЫЙТИ С УРОВНЯ?",
        2, SDL_Color{255, 222, 130, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 104,
        exitAction == EXIT_GAME ? "ИГРА БУДЕТ ЗАКРЫТА" : "ПРОГРЕСС ИГРЫ БУДЕТ ПОТЕРЯН",
        1, SDL_Color{212, 196, 170, 255});
    const char* choices[] = {"НЕТ, ОСТАТЬСЯ", "ДА, ВЫЙТИ"};
    for (int i = 0; i < 2; ++i) {
        int y = 126 + i * 24;
        bool selected = (i == (exitAccepted ? 1 : 0));
        if (selected) {
            SDL_Rect highlight{100, y - 4, 280, 21};
            SDL_SetRenderDrawColor(ren, 70, 54, 28, 255);
            SDL_RenderFillRect(ren, &highlight);
        }
        pixfont::drawTextCentered(ren, VIEW_W / 2, y, choices[i], 2,
            selected ? SDL_Color{255, 232, 160, 255} : SDL_Color{196, 184, 162, 255});
    }
    pixfont::drawTextCentered(ren, VIEW_W / 2, 178,
        touch.enabled ? "↑/↓ - ВЫБОР   ОК - ПОДТВЕРДИТЬ" : "↑/↓ - ВЫБОР   ENTER - ОК   ESC - НАЗАД",
        1, SDL_Color{150, 140, 128, 255});
}

void Game::drawTitleScreen()
{
    SDL_SetRenderDrawColor(ren, 8, 5, 10, 150);
    SDL_RenderFillRect(ren, nullptr);

    // золотые линейки
    SDL_SetRenderDrawColor(ren, 214, 174, 92, 140);
    SDL_Rect l1{70, 44, VIEW_W - 140, 1};
    SDL_Rect l2{70, 128, VIEW_W - 140, 1};
    SDL_RenderFillRect(ren, &l1);
    SDL_RenderFillRect(ren, &l2);

    pixfont::drawTextCentered(ren, VIEW_W / 2, 52, "АПАРТАМЕНТЫ", 3,
                              SDL_Color{255, 222, 130, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 84, "«ДАЙМОНД»", 3,
                              SDL_Color{238, 206, 120, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 112, "ПРИКЛЮЧЕНИЕ КОЛЯНА", 1,
                              SDL_Color{212, 196, 170, 255});

    const char* items[3] = {"НОВАЯ ИГРА", "НАСТРОЙКИ", "ВЫХОД"};
    for (int i = 0; i < 3; ++i) {
        int y = 148 + i * 24;
        bool sel = (i == menuIndex);
        if (sel) {
            SDL_SetRenderDrawColor(ren, 70, 54, 28, 210);
            SDL_Rect bg{VIEW_W / 2 - 92, y - 4, 184, 17};
            SDL_RenderFillRect(ren, &bg);
            SDL_SetRenderDrawColor(ren, 214, 174, 92, 220);
            SDL_Rect ln{VIEW_W / 2 - 92, y + 13, 184, 1};
            SDL_RenderFillRect(ren, &ln);
        }
        pixfont::drawTextCentered(ren, VIEW_W / 2, y, items[i], 2,
                                  sel ? SDL_Color{255, 232, 160, 255}
                                      : SDL_Color{196, 184, 162, 255});
    }

    pixfont::drawTextCentered(ren, VIEW_W / 2, 232,
                              touch.enabled ? "↑/↓ - ВЫБОР   ОК - ПОДТВЕРДИТЬ"
                                         : "W/S ИЛИ ↑/↓ - ВЫБОР   ENTER - ОК", 1,
                              SDL_Color{196, 176, 148, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 248,
                              touch.enabled ? "МЕНЮ - ВЫХОД ИЗ ИГРЫ"
                                            : "F11 - ПОЛНЫЙ ЭКРАН", 1,
                              SDL_Color{150, 140, 128, 255});
}

void Game::drawSettingsScreen()
{
    SDL_SetRenderDrawColor(ren, 8, 5, 10, 190);
    SDL_RenderFillRect(ren, nullptr);

    // золотые линейки
    SDL_SetRenderDrawColor(ren, 214, 174, 92, 140);
    SDL_Rect l1{40, 20, VIEW_W - 80, 1};
    SDL_Rect l2{40, 206, VIEW_W - 80, 1};
    SDL_RenderFillRect(ren, &l1);
    SDL_RenderFillRect(ren, &l2);

    pixfont::drawTextCentered(ren, VIEW_W / 2, 26, "НАСТРОЙКИ", 2,
                              SDL_Color{255, 222, 130, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 44, "УПРАВЛЕНИЕ", 1,
                              SDL_Color{196, 176, 148, 255});

    // Заголовки двух колонок с клавишами.
    pixfont::drawText(ren, 386 - pixfont::textWidth("КНОПКА 1", 1), 56,
                      "КНОПКА 1", 1, SDL_Color{170, 158, 138, 255});
    pixfont::drawText(ren, 452 - pixfont::textWidth("КНОПКА 2", 1), 56,
                      "КНОПКА 2", 1, SDL_Color{170, 158, 138, 255});

    const int rows = CA_COUNT + 1;
    const int slotRight[2] = {386, 452};
    for (int i = 0; i < rows; ++i) {
        int y = 72 + i * 17;
        bool sel = (i == settingsIndex);
        bool waiting = (i == bindTarget);
        if (sel) {
            if (waiting) SDL_SetRenderDrawColor(ren, 84, 44, 30, 215);
            else         SDL_SetRenderDrawColor(ren, 56, 44, 24, 215);
            SDL_Rect bg{44, y - 3, 412, 13};
            SDL_RenderFillRect(ren, &bg);
            if (waiting) SDL_SetRenderDrawColor(ren, 230, 120, 70, 230);
            else         SDL_SetRenderDrawColor(ren, 214, 174, 92, 230);
            SDL_Rect mark{44, y - 3, 2, 13};
            SDL_RenderFillRect(ren, &mark);
        }

        const char* label = (i < CA_COUNT) ? controlName(i) : "НАЗАД";
        SDL_Color lc = sel ? SDL_Color{246, 236, 214, 255}
                           : SDL_Color{190, 180, 160, 255};
        pixfont::drawText(ren, 56, y, label, 1, lc);

        if (i < CA_COUNT) {
            for (int s = 0; s < 2; ++s) {
                bool slotSel = sel && (s == settingsSlot);
                bool slotWaiting = waiting && (s == bindSlot);
                std::string key = slotWaiting ? "?"
                                              : keyLabel(controls.keys[i][s]);
                int tw = std::max(12, pixfont::textWidth(key, 1));
                if (slotSel) {
                    if (slotWaiting) SDL_SetRenderDrawColor(ren, 96, 46, 28, 235);
                    else             SDL_SetRenderDrawColor(ren, 96, 74, 38, 235);
                    SDL_Rect kr{slotRight[s] - tw - 4, y - 2, tw + 8, 11};
                    SDL_RenderFillRect(ren, &kr);
                }
                SDL_Color kc;
                if (slotWaiting)     kc = SDL_Color{255, 200, 130, 255};
                else if (slotSel)    kc = SDL_Color{255, 232, 160, 255};
                else if (controls.keys[i][s] == SDL_SCANCODE_UNKNOWN)
                                     kc = SDL_Color{120, 110, 100, 255};
                else                 kc = SDL_Color{212, 196, 170, 255};
                pixfont::drawText(ren, slotRight[s] - pixfont::textWidth(key, 1),
                                  y, key, 1, kc);
            }
        } else {
            const char* backKey = touch.enabled ? "МЕНЮ" : "ESC";
            pixfont::drawText(ren, 452 - pixfont::textWidth(backKey, 1), y,
                              backKey, 1, sel ? SDL_Color{255, 226, 150, 255}
                                              : SDL_Color{160, 150, 138, 255});
        }
    }

    if (bindTarget >= 0) {
        pixfont::drawTextCentered(ren, VIEW_W / 2, 216,
                                  touch.enabled
                                      ? "НАЖМИТЕ КЛАВИШУ   МЕНЮ - ОТМЕНА"
                                      : "НАЖМИТЕ КЛАВИШУ   ESC - ОТМЕНА", 1,
                                  SDL_Color{235, 180, 120, 255});
    } else {
        pixfont::drawTextCentered(ren, VIEW_W / 2, 216,
                                  touch.enabled
                                      ? "↑/↓ - ВЫБОР   ←/→ - КНОПКА 1 / 2"
                                      : "W/S - ВЫБОР   A/D - КНОПКА 1 / 2", 1,
                                  SDL_Color{196, 176, 148, 255});
        pixfont::drawTextCentered(ren, VIEW_W / 2, 232,
                                  touch.enabled
                                      ? "ОК - СМЕНИТЬ   МЕНЮ - НАЗАД"
                                      : "ENTER - СМЕНИТЬ   ESC - НАЗАД", 1,
                                  SDL_Color{150, 140, 128, 255});
    }
}

void Game::drawSlideScreen()
{
    SDL_SetRenderDrawColor(ren, 6, 4, 8, 238);
    SDL_RenderFillRect(ren, nullptr);

    SDL_SetRenderDrawColor(ren, 214, 174, 92, 150);
    SDL_Rect l1{54, 54, VIEW_W - 108, 1};
    SDL_Rect l2{54, 212, VIEW_W - 108, 1};
    SDL_RenderFillRect(ren, &l1);
    SDL_RenderFillRect(ren, &l2);

    pixfont::drawTextCentered(ren, VIEW_W / 2, 34, "МЕЖДУ ЭТАЖАМИ", 1,
                              SDL_Color{196, 176, 148, 255});

    if (slideWallScene) {
        drawSlideWall();
    } else if (slideIndex >= 0 && slideIndex < static_cast<int>(slideTexts.size())) {
        std::vector<std::string> lines =
            pixfont::wrapText(slideTexts[slideIndex], 32);
        int lineH = 22;
        int top = (VIEW_H - static_cast<int>(lines.size()) * lineH) / 2 - 4;
        for (size_t i = 0; i < lines.size(); ++i) {
            pixfont::drawTextCentered(ren, VIEW_W / 2,
                                      top + static_cast<int>(i) * lineH,
                                      lines[i], 2,
                                      SDL_Color{246, 232, 198, 255});
        }
    }

    std::string counter = "СЛАЙД " + std::to_string(slideIndex + 1) + " / " +
                          std::to_string(static_cast<int>(slideTexts.size()));
    pixfont::drawTextCentered(ren, VIEW_W / 2, 222, counter, 1,
                              SDL_Color{150, 140, 128, 255});

    if (std::fmod(titleBlink, 1.15f) < 0.78f) {
        pixfont::drawTextCentered(ren, VIEW_W / 2, 240,
                                  touch.enabled ? "ОК - ДАЛЬШЕ"
                                             : "ENTER - ДАЛЬШЕ", 1,
                                  SDL_Color{235, 210, 160, 255});
    }
}

void Game::drawSlideWall()
{
    // Кадр-«фотография»: бетонная стена у лифта, на которой Лёха оставил
    // надпись баллончиком. Рисуем её вместо текста слайда.
    const int px = 56;
    const int py = 58;
    const int pw = VIEW_W - 112;
    const int ph = 146;
    const int floorY = py + ph - 16;

    // Бетон: вертикальные полосы и мелкие пятна.
    for (int y = 0; y < ph; ++y) {
        uint32_t h = hash2(px, py + y);
        int v = 44 + static_cast<int>((h >> 4) % 9);
        SDL_SetRenderDrawColor(ren, static_cast<Uint8>(v),
                               static_cast<Uint8>(v + 1),
                               static_cast<Uint8>(v + 1), 255);
        SDL_Rect band{px, py + y, pw, 1};
        SDL_RenderFillRect(ren, &band);
    }
    for (int bx = 0; bx < pw; bx += 46) {
        SDL_SetRenderDrawColor(ren, 34, 35, 35, 255);
        SDL_Rect seam{px + bx, py, 1, floorY - py};
        SDL_RenderFillRect(ren, &seam);
        SDL_SetRenderDrawColor(ren, 62, 64, 64, 90);
        SDL_Rect seamHi{px + bx + 1, py, 1, floorY - py};
        SDL_RenderFillRect(ren, &seamHi);
        for (int by = py + 18; by < floorY; by += 34) {
            uint32_t h = hash2(px + bx, by);
            SDL_SetRenderDrawColor(ren, 30, 31, 31, 160);
            SDL_Rect blot{px + bx + 3 + static_cast<int>(h % 20),
                          by + static_cast<int>((h >> 5) % 12),
                          6 + static_cast<int>((h >> 9) % 18), 2};
            SDL_RenderFillRect(ren, &blot);
            SDL_SetRenderDrawColor(ren, 88, 90, 88, 110);
            SDL_Rect bolt{px + bx + 2, by, 2, 2};
            SDL_RenderFillRect(ren, &bolt);
        }
    }
    // Горизонтальные швы плит.
    for (int by = py + 36; by < floorY; by += 38) {
        SDL_SetRenderDrawColor(ren, 26, 27, 27, 255);
        SDL_Rect seam{px, by, pw, 1};
        SDL_RenderFillRect(ren, &seam);
        SDL_SetRenderDrawColor(ren, 70, 72, 70, 70);
        SDL_Rect seamLo{px, by + 1, pw, 1};
        SDL_RenderFillRect(ren, &seamLo);
    }

    // Кабельный лоток под потолком кадра и свисающая петля.
    SDL_SetRenderDrawColor(ren, 30, 31, 32, 255);
    SDL_Rect tray{px, py + 2, pw, 5};
    SDL_RenderFillRect(ren, &tray);
    SDL_SetRenderDrawColor(ren, 58, 60, 60, 255);
    SDL_Rect trayHi{px, py + 2, pw, 1};
    SDL_RenderFillRect(ren, &trayHi);
    SDL_SetRenderDrawColor(ren, 20, 20, 22, 255);
    for (int i = 0; i < 3; ++i) {
        SDL_Rect cable{px + 12 + i * 5, py + 7, 2, 8 + i * 3};
        SDL_RenderFillRect(ren, &cable);
    }
    SDL_SetRenderDrawColor(ren, 22, 22, 24, 255);
    SDL_RenderDrawLine(ren, px + 306, py + 7, px + 312, py + 16);
    SDL_RenderDrawLine(ren, px + 312, py + 16, px + 320, py + 8);
    SDL_RenderDrawLine(ren, px + 312, py + 16, px + 306, py + 30);

    // Аварийная лампа в углу: мигает, как на техническом этаже.
    const float blink = std::fmod(titleBlink, 2.7f);
    const bool lit = blink < 2.10f;
    if (lit) {
        fillTriangle(px + 22, py + 12, px + 4, floorY, px + 46, floorY,
                     SDL_Color{255, 196, 96, 16});
        fillTriangle(px + 22, py + 12, px + 10, floorY - 22, px + 38,
                     floorY - 22, SDL_Color{255, 214, 128, 12});
    }
    SDL_SetRenderDrawColor(ren, 26, 24, 22, 255);
    SDL_Rect lampArm{px + 16, py + 9, 12, 2};
    SDL_RenderFillRect(ren, &lampArm);
    SDL_SetRenderDrawColor(ren, lit ? 255 : 132, lit ? 214 : 118,
                           lit ? 132 : 74, 255);
    SDL_Rect bulb{px + 19, py + 11, 8, 4};
    SDL_RenderFillRect(ren, &bulb);
    SDL_SetRenderDrawColor(ren, 46, 38, 30, 200);
    SDL_Rect guard{px + 19, py + 9, 8, 1};
    SDL_RenderFillRect(ren, &guard);

    // Пол коридора с грязной лужицей.
    SDL_SetRenderDrawColor(ren, 30, 29, 31, 255);
    SDL_Rect floor{px, floorY, pw, ph - (floorY - py)};
    SDL_RenderFillRect(ren, &floor);
    SDL_SetRenderDrawColor(ren, 54, 52, 54, 255);
    SDL_Rect floorEdge{px, floorY, pw, 1};
    SDL_RenderFillRect(ren, &floorEdge);
    SDL_SetRenderDrawColor(ren, 22, 22, 24, 255);
    SDL_Rect puddle{px + 96, floorY + 5, 46, 4};
    SDL_RenderFillRect(ren, &puddle);

    // Сама надпись: два слоя, будто краска легла неровно.
    const SDL_Color paint{208, 204, 190, 255};
    const SDL_Color paintLo{98, 94, 88, 160};
    const SDL_Color red{168, 54, 50, 230};
    const std::string line1 = "ЛЁХА БЫЛ";
    const std::string line2 = "ЗДЕСЬ";
    int x1 = VIEW_W / 2 - pixfont::textWidth(line1, 3) / 2;
    int x2 = VIEW_W / 2 - pixfont::textWidth(line2, 3) / 2;
    int y1 = 96;
    int y2 = 128;

    // Тёмный подслой и светлая краска сверху; вокруг — редкие брызги.
    pixfont::drawText(ren, x1 + 1, y1 + 1, line1, 3, paintLo, false);
    pixfont::drawText(ren, x1 + 2, y2 + 1, line2, 3, paintLo, false);
    pixfont::drawText(ren, x1, y1, line1, 3, paint, false);
    pixfont::drawText(ren, x2, y2, line2, 3, paint, false);
    // В шрифте 5x7 у «Ё» нет диакритики: дорисовываем две точки над второй
    // буквой, чтобы на стене читалось именно «ЛЁХА».
    {
        int ex = x1 + 18;
        SDL_SetRenderDrawColor(ren, paintLo.r, paintLo.g, paintLo.b, 180);
        SDL_Rect s1{ex + 4, y1 - 4, 3, 2};
        SDL_Rect s2{ex + 10, y1 - 4, 3, 2};
        SDL_RenderFillRect(ren, &s1);
        SDL_RenderFillRect(ren, &s2);
        SDL_SetRenderDrawColor(ren, paint.r, paint.g, paint.b, 255);
        SDL_Rect d1{ex + 4, y1 - 5, 3, 2};
        SDL_Rect d2{ex + 10, y1 - 5, 3, 2};
        SDL_RenderFillRect(ren, &d1);
        SDL_RenderFillRect(ren, &d2);
    }
    SDL_SetRenderDrawColor(ren, paint.r, paint.g, paint.b, 170);
    for (int i = 0; i < 22; ++i) {
        uint32_t h = hash2(i * 13 + 5, 77);
        int sx2 = x1 - 12 + static_cast<int>(h % (pixfont::textWidth(line1, 3) + 24));
        int sy2 = y1 - 8 + static_cast<int>((h >> 8) % 62);
        SDL_Rect speck{sx2, sy2, 1 + static_cast<int>((h >> 16) % 2), 1};
        SDL_RenderFillRect(ren, &speck);
    }

    // Потёки краски под буквами.
    auto drip = [&](int x, int top, int len) {
        SDL_SetRenderDrawColor(ren, paint.r, paint.g, paint.b, 220);
        SDL_Rect stream{x, top, 1, len};
        SDL_RenderFillRect(ren, &stream);
        SDL_SetRenderDrawColor(ren, paint.r, paint.g, paint.b, 140);
        SDL_Rect drop{x - 1, top + len, 3, 1};
        SDL_RenderFillRect(ren, &drop);
    };
    drip(x1 + 18, y1 + 21, 6);
    drip(x1 + 62, y1 + 21, 9);
    drip(x1 + 104, y1 + 21, 4);
    drip(x2 + 12, y2 + 21, 8);
    drip(x2 + 54, y2 + 21, 11);

    // Красный росчерк и стрелка в сторону лифта.
    SDL_SetRenderDrawColor(ren, red.r, red.g, red.b, 210);
    SDL_Rect tagLine{x1 - 4, y2 + 30, pixfont::textWidth(line1, 3) + 14, 1};
    SDL_RenderFillRect(ren, &tagLine);
    SDL_RenderDrawLine(ren, px + 268, py + 108, px + 322, py + 108);
    SDL_RenderDrawLine(ren, px + 322, py + 108, px + 312, py + 103);
    SDL_RenderDrawLine(ren, px + 322, py + 108, px + 312, py + 113);
}

void Game::drawLevelSelectScreen()
{
    // DEV: экран выбора уровня за секретной дверью.
    SDL_SetRenderDrawColor(ren, 6, 4, 8, 238);
    SDL_RenderFillRect(ren, nullptr);

    SDL_SetRenderDrawColor(ren, 90, 220, 240, 160);
    SDL_Rect l1{54, 54, VIEW_W - 108, 1};
    SDL_Rect l2{54, 212, VIEW_W - 108, 1};
    SDL_RenderFillRect(ren, &l1);
    SDL_RenderFillRect(ren, &l2);

    pixfont::drawTextCentered(ren, VIEW_W / 2, 28, "DEV: ВЫБОР УРОВНЯ", 2,
                              SDL_Color{140, 236, 250, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 48,
                              "СЕКРЕТНАЯ ДВЕРЬ — ТОЛЬКО ДЛЯ РАЗРАБОТКИ", 1,
                              SDL_Color{150, 140, 128, 255});

    static const char* const levelNames[LEVEL_COUNT] = {
        "ОСОБНЯК", "ТЕХЭТАЖ", "ВЕСТИБЮЛЬ", "ПАРКОВКА", "ЛЕСОПИЛКА"
    };
    for (int i = 0; i < LEVEL_COUNT; ++i) {
        bool sel = (i == levelSelectIndex);
        int y = 72 + i * 26;

        if (sel) {
            SDL_SetRenderDrawColor(ren, 30, 74, 88, 220);
            SDL_Rect hl{VIEW_W / 2 - 150, y - 6, 300, 23};
            SDL_RenderFillRect(ren, &hl);
            SDL_SetRenderDrawColor(ren, 100, 224, 240, 255);
            SDL_Rect hlTop{VIEW_W / 2 - 150, y - 6, 300, 1};
            SDL_Rect hlBot{VIEW_W / 2 - 150, y + 16, 300, 1};
            SDL_RenderFillRect(ren, &hlTop);
            SDL_RenderFillRect(ren, &hlBot);
        }

        std::string label = "УРОВЕНЬ " + std::to_string(i + 1) + " — " +
                            ((i >= 0 && i < LEVEL_COUNT) ? levelNames[i] : "");
        pixfont::drawTextCentered(ren, VIEW_W / 2, y,
                                  label, sel ? 2 : 1,
                                  sel ? SDL_Color{255, 236, 170, 255}
                                      : SDL_Color{176, 166, 150, 255});
    }

    pixfont::drawTextCentered(ren, VIEW_W / 2, 196,
                              "W/S ИЛИ ↑/↓ - ВЫБОР", 1,
                              SDL_Color{196, 176, 148, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 226,
                              "ENTER - ЗАГРУЗИТЬ   ESC - НАЗАД В ИГРУ", 1,
                              SDL_Color{150, 140, 128, 255});

    pixfont::drawText(ren, 8, 8, "DEV", 1, SDL_Color{90, 220, 240, 255});
}

void Game::drawWinScreen()
{
    SDL_SetRenderDrawColor(ren, 8, 10, 26, 150);
    SDL_RenderFillRect(ren, nullptr);
    drawParticles();

    SDL_SetRenderDrawColor(ren, 220, 184, 96, 160);
    SDL_Rect l1{64, 64, VIEW_W - 128, 1};
    SDL_Rect l2{64, 168, VIEW_W - 128, 1};
    SDL_RenderFillRect(ren, &l1);
    SDL_RenderFillRect(ren, &l2);

    const bool bossWin = (levelIndex == 3);
    const bool sawmillWin = (levelIndex == 4);
    pixfont::drawTextCentered(ren, VIEW_W / 2, 80,
                              sawmillWin ? "ЛЁХА ОТВЕТИЛ ЗА ВСЁ!"
                              : bossWin    ? "БОСС ПОВЕРЖЕН!"
                                           : "КОЛЯН ВЫБРАЛСЯ!", 3,
                              SDL_Color{255, 226, 140, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 124,
                              sawmillWin ? "ЗАБРОШЕННАЯ ЛЕСОПИЛКА"
                              : bossWin  ? "КОЛЯН УЕХАЛ НА ДЖИПЕ"
                                         : "ИЗ АПАРТАМЕНТОВ «ДАЙМОНД»", 2,
                              SDL_Color{230, 220, 200, 255});

    if (std::fmod(titleBlink, 1.15f) < 0.78f) {
        pixfont::drawTextCentered(ren, VIEW_W / 2, 192,
                                  touch.enabled ? "ЗАНОВО - НОВАЯ ИГРА"
                                             : "R - ИГРАТЬ СНОВА", 2,
                                  SDL_Color{255, 232, 170, 255});
    }
    std::string stats = bossWin
        ? "ОЧКИ: " + std::to_string(score) +
              "   МОНСТРОВ: " + std::to_string(kills)
        : "КУБКИ 3/3   ОЧКИ: " + std::to_string(score) +
              "   МОНСТРОВ: " + std::to_string(kills);
    pixfont::drawTextCentered(ren, VIEW_W / 2, 226, stats, 1,
                              SDL_Color{190, 180, 160, 255});
}

void Game::drawDeathOverlay()
{
    const bool spider = (deathCause == DEATH_SPIDER);
    const bool bullet = (deathCause == DEATH_BULLET);
    const bool dog = (deathCause == DEATH_DOBERMAN);
    const bool fall = (deathCause == DEATH_FALL);
    const bool boss = (deathCause == DEATH_BOSS);
    const bool blast = boss || (deathCause == DEATH_GRENADE);
    float t = deathTimer;

    SDL_SetRenderDrawColor(ren, 6, 3, 8, 170);
    SDL_RenderFillRect(ren, nullptr);

    // маленькое окно по центру — без единой надписи, всего на несколько
    // пикселей больше самого Коляна в кадре
    SDL_Rect panel{VIEW_W / 2 - 50, (VIEW_H - 100) / 2, 100, 100};
    SDL_SetRenderDrawColor(ren, 20, 14, 20, 242);
    SDL_RenderFillRect(ren, &panel);

    SDL_Color edge = spider ? SDL_Color{178, 86, 140, 255}
                            : bullet ? SDL_Color{236, 196, 96, 255}
                            : dog ? SDL_Color{196, 52, 52, 255}
                            : boss ? SDL_Color{152, 42, 56, 255}
                            : (deathCause == DEATH_GRENADE)
                                  ? SDL_Color{214, 132, 58, 255}
                            : fall ? SDL_Color{132, 118, 104, 255}
                                  : SDL_Color{168, 112, 62, 255};
    SDL_SetRenderDrawColor(ren, edge.r, edge.g, edge.b, 255);
    SDL_RenderDrawRect(ren, &panel);
    SDL_SetRenderDrawColor(ren, static_cast<Uint8>(edge.r / 2),
                           static_cast<Uint8>(edge.g / 2),
                           static_cast<Uint8>(edge.b / 2), 255);
    SDL_Rect inner{panel.x + 3, panel.y + 3, panel.w - 6, panel.h - 6};
    SDL_RenderDrawRect(ren, &inner);

    SDL_RenderSetClipRect(ren, &panel);

    auto box = [&](int bx, int by, int bw, int bh, SDL_Color c) {
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
        SDL_Rect r{bx, by, bw, bh};
        SDL_RenderFillRect(ren, &r);
    };
    auto limb = [&](int x0, int y0, int x1, int y1, int th, SDL_Color c) {
        int dx = x1 - x0, dy = y1 - y0;
        int steps = std::max(1, std::max(std::abs(dx), std::abs(dy)) / 2);
        for (int i = 0; i <= steps; ++i) {
            int lx = x0 + dx * i / steps;
            int ly = y0 + dy * i / steps;
            box(lx - th / 2, ly - th / 2, th, th, c);
        }
    };

    if (spider) {
        // Смерть от паука: он спускается на нити и кусает.
        int px = panel.x + 24;
        int py = panel.y + 20;
        float k = clampf((t - 0.20f) / 1.05f, 0.0f, 1.0f);
        float bite = clampf((t - 0.80f) / 0.55f, 0.0f, 1.0f);
        SDL_Color badSkin{158, 86, 198, 255};
        SDL_Color skin{
            static_cast<Uint8>(238 + (badSkin.r - 238) * bite),
            static_cast<Uint8>(190 + (badSkin.g - 190) * bite),
            static_cast<Uint8>(150 + (badSkin.b - 150) * bite), 255};

        box(px + 1, py + 38, 46, 16, SDL_Color{52, 84, 148, 255});
        box(px + 1, py + 38, 46, 3, SDL_Color{190, 58, 62, 255});
        box(px + 7, py + 8, 32, 32, skin);
        box(px + 10, py + 40, 26, 3, skin);
        box(px + 5, py + 2, 36, 10, SDL_Color{62, 40, 30, 255});
        box(px + 4, py + 8, 4, 12, SDL_Color{62, 40, 30, 255});
        box(px + 38, py + 8, 4, 12, SDL_Color{62, 40, 30, 255});

        if (bite < 0.55f) {
            box(px + 13, py + 20, 8, 6, SDL_Color{248, 246, 240, 255});
            box(px + 26, py + 20, 8, 6, SDL_Color{248, 246, 240, 255});
            int look = (bite > 0.25f) ? 2 : 0;
            box(px + 15 + look, py + 22, 3, 3, SDL_Color{30, 22, 24, 255});
            box(px + 28 + look, py + 22, 3, 3, SDL_Color{30, 22, 24, 255});
        } else {
            SDL_SetRenderDrawColor(ren, 56, 16, 24, 255);
            for (int e = 0; e < 2; ++e) {
                int ex = px + (e == 0 ? 13 : 26);
                int ey = py + 20;
                SDL_RenderDrawLine(ren, ex, ey, ex + 7, ey + 6);
                SDL_RenderDrawLine(ren, ex + 7, ey, ex, ey + 6);
            }
        }
        if (bite > 0.35f && bite < 0.95f) {
            box(px + 19, py + 32, 9, 6, SDL_Color{90, 20, 28, 255});
            box(px + 22, py + 32, 3, 2, SDL_Color{240, 236, 226, 255});
        } else {
            box(px + 20, py + 33, 8, 2, SDL_Color{150, 72, 72, 255});
        }

        int jitter = (bite > 0.0f && bite < 1.0f)
                         ? static_cast<int>(std::sin(t * 42.0f) * 2.0f) : 0;
        int mx = px + 39 + jitter;
        int my = static_cast<int>(panel.y + 8 + k * (py + 22 - (panel.y + 8)));
        SDL_SetRenderDrawColor(ren, 210, 210, 220, 120);
        SDL_RenderDrawLine(ren, mx, panel.y + 4, mx, my - 4);
        SDL_SetRenderDrawColor(ren, 38, 24, 34, 255);
        for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < 4; ++i) {
                int lx = mx + side * (6 + i);
                int ly = my + 1 - i * 2;
                SDL_RenderDrawLine(ren, mx, my, lx, ly - 2);
                SDL_RenderDrawLine(ren, lx, ly - 2, lx + side * 3, ly + 3);
            }
        }
        fillCircle(mx, my, 5, SDL_Color{82, 44, 62, 255});
        fillCircle(mx, my - 2, 3, SDL_Color{112, 64, 86, 255});
        box(mx - 2, my - 4, 2, 2, SDL_Color{255, 70, 60, 255});
        box(mx + 1, my - 4, 2, 2, SDL_Color{255, 70, 60, 255});
        box(mx - 2, my + 5, 5, 2, SDL_Color{30, 18, 26, 255});
    } else if (bullet) {
        // Смерть от пули: она входит в глаз, голова — строго в профиль.
        float u = clampf((t - 0.12f) / 0.60f, 0.0f, 1.0f);
        float impact = clampf((t - 0.72f) / 0.12f, 0.0f, 1.0f);
        float jt = clampf((t - 0.72f) / 0.32f, 0.0f, 1.0f);
        float jolt = -std::sin(jt * 3.14159265f) * 5.0f;

        int px = panel.x + 24 + static_cast<int>(jolt);
        int py = panel.y + 14;

        const SDL_Color skin{236, 188, 148, 255};
        const SDL_Color skinShade{204, 154, 120, 255};
        const SDL_Color hair{58, 38, 28, 255};

        // плечи, шарф, шея
        box(px - 14, py + 56, 82, 20, SDL_Color{52, 84, 148, 255});
        box(px - 14, py + 56, 82, 4, SDL_Color{190, 58, 62, 255});
        box(px + 18, py + 52, 32, 6, SDL_Color{190, 58, 62, 255});
        box(px + 26, py + 44, 20, 12, skinShade);

        // голова в профиль, лицом влево
        box(px + 2, py, 50, 48, skin);
        box(px + 44, py + 8, 10, 38, skinShade);
        // нос и подбородок
        fillTriangle(px + 2, py + 20, px - 6, py + 28, px + 2, py + 34, skinShade);
        box(px + 4, py + 44, 24, 6, skinShade);
        // волосы: затылок, макушка, чёлка
        box(px - 1, py - 5, 54, 12, hair);
        box(px + 40, py + 2, 15, 40, hair);
        box(px + 34, py + 8, 7, 13, hair);
        box(px + 3, py + 5, 12, 4, hair);
        // ухо поверх волос — сразу читается профиль
        box(px + 38, py + 23, 11, 13, skinShade);
        box(px + 41, py + 26, 4, 7, hair);

        // один глаз в профиль: пуля войдёт прямо в него
        int eyeY = py + 17;
        if (impact < 0.5f) {
            box(px + 11, eyeY, 13, 8, SDL_Color{248, 246, 240, 255});
            box(px + 14, eyeY + 2, 4, 4, SDL_Color{30, 22, 24, 255});
            box(px + 10, eyeY - 4, 17, 3, hair);
        } else {
            SDL_SetRenderDrawColor(ren, 56, 16, 24, 255);
            SDL_RenderDrawLine(ren, px + 11, eyeY + 4, px + 23, eyeY + 4);
            SDL_RenderDrawLine(ren, px + 11, eyeY + 2, px + 23, eyeY + 6);
            SDL_RenderDrawLine(ren, px + 11, eyeY + 6, px + 23, eyeY + 2);
        }
        // рот
        box(px + 5, py + 40, 12, 2, SDL_Color{150, 72, 72, 255});

        // цель пули — глаз
        float targetX = static_cast<float>(px + 23);
        float targetY = static_cast<float>(eyeY + 4);
        float startX = static_cast<float>(panel.x - 20);
        float tipX = startX + (targetX - startX) * u;

        // след полёта
        if (u > 0.05f && u < 1.0f) {
            SDL_SetRenderDrawColor(ren, 255, 214, 130, 95);
            SDL_RenderDrawLine(ren, static_cast<int>(tipX) - 34,
                               static_cast<int>(targetY),
                               static_cast<int>(tipX) - 8,
                               static_cast<int>(targetY));
            SDL_SetRenderDrawColor(ren, 255, 240, 190, 140);
            SDL_RenderDrawLine(ren, static_cast<int>(tipX) - 24,
                               static_cast<int>(targetY) - 1,
                               static_cast<int>(tipX) - 9,
                               static_cast<int>(targetY) - 1);
        }

        // крупная пуля: медный корпус, острая головка и яркий сердечник
        int hy = static_cast<int>(targetY);
        int tip = static_cast<int>(tipX);
        int tail = tip - 15;
        box(tail, hy - 3, 10, 6, SDL_Color{188, 138, 54, 255});
        box(tail, hy - 3, 2, 6, SDL_Color{120, 84, 34, 255});
        fillTriangle(tip - 5, hy - 4, tip - 5, hy + 4, tip, hy,
                     SDL_Color{236, 194, 92, 255});
        fillTriangle(tip - 5, hy - 4, tip - 5, hy, tip, hy,
                     SDL_Color{252, 226, 150, 255});
        box(tip - 12, hy - 2, 4, 2, SDL_Color{255, 250, 214, 235});

        // удар: кровь из глаза
        if (impact > 0.0f) {
            int cx = px + 18;
            int cy = eyeY + 4;
            box(cx - 3, cy - 2, 7, 5, SDL_Color{150, 30, 40, 235});
            box(cx - 1, cy + 3, 2, 7, SDL_Color{128, 24, 34, 210});
            box(cx + 3, cy + 5, 1, 5, SDL_Color{120, 22, 32, 190});
            float ia = clampf(1.0f - (t - 0.80f) / 0.5f, 0.0f, 1.0f);
            SDL_SetRenderDrawColor(ren, 200, 40, 50,
                                   static_cast<Uint8>(210 * ia));
            for (int i = 0; i < 7; ++i) {
                float a = 0.45f + i * 0.38f;
                int len = 6 + static_cast<int>(9.0f * impact);
                SDL_RenderDrawLine(ren, cx, cy,
                                   cx + static_cast<int>(std::cos(a) * len),
                                   cy - static_cast<int>(std::sin(a) * len) + 2);
            }

            // красные брызги из затылка: пуля вышла с другой стороны
            int bx = px + 52;
            int by = py + 16;
            float fade = clampf(1.0f - (t - 0.86f) / 0.95f, 0.0f, 1.0f);
            for (int i = 0; i < 9; ++i) {
                float a = -0.55f + i * 0.15f;
                int len = 8 + static_cast<int>(16.0f * impact) +
                          static_cast<int>(3.0f * std::sin(t * 31.0f + i));
                SDL_SetRenderDrawColor(ren, 206, 32, 44,
                                       static_cast<Uint8>(225 * fade));
                SDL_RenderDrawLine(ren, bx, by,
                                   bx + static_cast<int>(std::cos(a) * len),
                                   by + static_cast<int>(std::sin(a) * len));
            }
            for (int i = 0; i < 6; ++i) {
                int dx = 4 + static_cast<int>(std::fmod(t * 58.0f + i * 11.0f, 28.0f));
                int dy = -8 + ((i * 9) % 22);
                SDL_SetRenderDrawColor(ren, 176, 26, 36,
                                       static_cast<Uint8>(215 * fade));
                SDL_Rect drop{bx + dx, by + dy, 2, 2};
                SDL_RenderFillRect(ren, &drop);
            }
        }

        // рука Коляна дёрнулась к лицу
        limb(px + 10, py + 62, px + 5, py + 42, 6, skinShade);
        box(px + 1, py + 37, 8, 6, skin);
    } else if (fall) {
        // Падение в шахту: Колян кувыркается в темноте, мимо летят полосы.
        float u = clampf(t / 1.20f, 0.0f, 1.0f);
        int cx = panel.x + 50;
        int cy = panel.y + 16 + static_cast<int>(u * 66.0f);
        int spin = static_cast<int>(u * 14.0f);

        SDL_SetRenderDrawColor(ren, 112, 98, 86, 120);
        for (int i = 0; i < 4; ++i) {
            int lx = cx - 20 + i * 13;
            SDL_RenderDrawLine(ren, lx, panel.y + 4, lx, panel.y + 44);
        }

        box(cx - 9 + spin / 4, cy, 18, 24, SDL_Color{52, 84, 148, 255});
        box(cx - 9 + spin / 4, cy, 18, 3, SDL_Color{190, 58, 62, 255});
        box(cx - 7 + spin / 3, cy - 14, 14, 15, SDL_Color{238, 190, 150, 255});
        box(cx - 9 + spin / 3, cy - 16, 18, 5, SDL_Color{62, 40, 30, 255});
        box(cx - 5 + spin / 3, cy + 23, 4, 13, SDL_Color{40, 52, 74, 255});
        box(cx + 1 + spin / 3, cy + 23, 4, 13, SDL_Color{40, 52, 74, 255});
        if (t > 1.05f) {
            box(cx - 12, cy + 36, 24, 3, SDL_Color{150, 44, 50, 220});
            box(cx - 4, cy + 34, 8, 2, SDL_Color{182, 60, 62, 200});
        }
    } else if (deathCause == DEATH_BAT || blast) {
        // Смерть от братка с битой, гиганта на парковке или взрыва гранаты:
        // удар сзади сносит Коляну голову.
        float u = clampf((t - 0.10f) / 0.85f, 0.0f, 1.0f);
        float impact = clampf((t - 0.95f) / 0.12f, 0.0f, 1.0f);
        float jt = clampf((t - 0.95f) / 0.30f, 0.0f, 1.0f);
        float jolt = -std::sin(jt * 3.14159265f) * 6.0f;
        float detach = clampf((t - 0.95f) / 0.70f, 0.0f, 1.0f);
        bool hit = (t >= 0.95f);
        // голова остаётся в кадре, а тело оседает вниз — между шеей и головой
        // появляется хорошо читаемый провал.
        int bodyDrop = static_cast<int>(18.0f * detach);

        int px = panel.x + 23 + static_cast<int>(jolt);
        int py = panel.y + 14;
        int headX = px - static_cast<int>(12.0f * detach);
        int headY = py - static_cast<int>(6.0f * detach);

        const SDL_Color skin{238, 190, 150, 255};
        const SDL_Color skinShade{206, 156, 122, 255};
        const SDL_Color hair{62, 40, 30, 255};

        // плечи и шарф — после удара тело оседает
        box(px - 14, py + 56 + bodyDrop, 82, 20, SDL_Color{52, 84, 148, 255});
        box(px - 14, py + 56 + bodyDrop, 82, 4, SDL_Color{190, 58, 62, 255});
        box(px + 18, py + 54 + bodyDrop, 32, 6, SDL_Color{190, 58, 62, 255});

        // шея: до удара целая, после — кровавый обрубок
        if (!hit) {
            box(px + 30, py + 46, 16, 12, skinShade);
        } else {
            box(px + 30, py + 42 + bodyDrop, 16, 12, SDL_Color{126, 24, 34, 255});
            box(px + 32, py + 42 + bodyDrop, 12, 3,
                SDL_Color{232, 220, 200, 255});
            box(px + 33, py + 45 + bodyDrop, 3, 2,
                SDL_Color{180, 40, 48, 255});
            box(px + 41, py + 45 + bodyDrop, 3, 2,
                SDL_Color{180, 40, 48, 255});
            // кровь стекает по обрубку
            box(px + 31, py + 50 + bodyDrop, 2, 4, SDL_Color{150, 28, 38, 220});
            box(px + 43, py + 49 + bodyDrop, 1, 3, SDL_Color{140, 26, 36, 200});
        }

        // голова в профиль: после удара отлетает влево-вверх
        box(headX, headY, 52, 50, skin);
        box(headX + 3, headY + 42, 46, 9, skin);
        fillTriangle(headX, headY + 24, headX - 7, headY + 31,
                     headX, headY + 35, skinShade);
        box(headX - 3, headY - 5, 58, 14, hair);
        box(headX + 38, headY - 2, 16, 44, hair);
        box(headX + 34, headY + 4, 6, 12, hair);
        box(headX + 40, headY + 22, 9, 11, skinShade);
        box(headX + 42, headY + 25, 4, 5, hair);
        box(headX + 7, headY + 15, 13, 2, hair);

        if (impact < 0.5f) {
            box(headX + 8, headY + 19, 11, 5, SDL_Color{248, 246, 240, 255});
            box(headX + 10, headY + 20, 3, 3, SDL_Color{30, 22, 24, 255});
        } else {
            SDL_SetRenderDrawColor(ren, 56, 16, 24, 255);
            SDL_RenderDrawLine(ren, headX + 8, headY + 18, headX + 19, headY + 24);
            SDL_RenderDrawLine(ren, headX + 19, headY + 18, headX + 8, headY + 24);
            SDL_RenderDrawLine(ren, headX + 8, headY + 19, headX + 18, headY + 24);
            SDL_RenderDrawLine(ren, headX + 9, headY + 18, headX + 19, headY + 23);
        }

        if (impact > 0.2f) {
            box(headX + 4, headY + 38, 16, 7, SDL_Color{90, 20, 28, 255});
            box(headX + 6, headY + 38, 3, 2, SDL_Color{240, 236, 226, 255});
            box(headX + 11, headY + 38, 2, 2, SDL_Color{240, 236, 226, 255});
        } else {
            box(headX + 6, headY + 41, 12, 2, SDL_Color{150, 72, 72, 255});
        }

        // рваный срез шеи на отлетевшей голове
        if (hit) {
            box(headX + 24, headY + 47, 20, 4, SDL_Color{126, 24, 34, 255});
            box(headX + 30, headY + 47, 7, 2, SDL_Color{232, 220, 200, 255});
            box(headX + 26, headY + 50, 2, 5, SDL_Color{128, 24, 34, 210});
            box(headX + 38, headY + 50, 1, 4, SDL_Color{120, 22, 32, 190});
        }

        // кровь хлещет из обрубка шеи
        if (hit) {
            int nx = px + 38;
            int ny = py + 42 + bodyDrop;
            float fade = clampf(1.0f - (t - 1.10f) / 1.20f, 0.0f, 1.0f);
            for (int i = 0; i < 9; ++i) {
                float a = 3.35f + i * 0.22f;
                int len = 8 + static_cast<int>(13.0f * impact) +
                          static_cast<int>(2.0f * std::sin(t * 24.0f + i));
                SDL_SetRenderDrawColor(ren, 196, 34, 44,
                                       static_cast<Uint8>(220 * fade));
                SDL_RenderDrawLine(ren, nx, ny,
                                   nx + static_cast<int>(std::cos(a) * len),
                                   ny + static_cast<int>(std::sin(a) * len));
            }
            for (int i = 0; i < 5; ++i) {
                int dx = ((i * 7 + 3) % 25) - 12;
                int dy = static_cast<int>(std::fmod(t * 46.0f + i * 9.0f, 26.0f));
                SDL_SetRenderDrawColor(ren, 170, 28, 38,
                                       static_cast<Uint8>(210 * fade));
                SDL_Rect drop{nx + dx, ny + dy, 2, 3};
                SDL_RenderFillRect(ren, &drop);
            }
        }

        // бита сама летит по дуге от правого верхнего угла к затылку
        int startX = panel.x + 112, startY = panel.y - 12;
        int ctrlX = panel.x + 94, ctrlY = panel.y - 14;
        int endX = px + 46, endY = py + 10;
        float inv = 1.0f - u;
        int hx = static_cast<int>(inv * inv * startX + 2.0f * inv * u * ctrlX +
                                  u * u * endX);
        int hy = static_cast<int>(inv * inv * startY + 2.0f * inv * u * ctrlY +
                                  u * u * endY);
        // после удара бита отлетает, выпуская голову
        float retract = clampf((t - 1.02f) / 0.45f, 0.0f, 1.0f);
        hx += static_cast<int>(18.0f * retract);
        hy -= static_cast<int>(6.0f * retract);
        if (u >= 1.0f && retract <= 0.0f) {
            hx += static_cast<int>(std::sin(t * 55.0f));
            hy += static_cast<int>(std::cos(t * 47.0f));
        }

        // бита крупным планом: тонкая рукоять с набалдашником и толстый боёк
        int batMidX = hx - 13, batMidY = hy + 1;
        int batTipX = hx - 42, batTipY = hy + 6;
        box(hx - 1, hy - 3, 4, 7, SDL_Color{72, 44, 22, 255});
        limb(hx, hy, batMidX, batMidY, 7, SDL_Color{92, 56, 28, 255});
        limb(batMidX, batMidY, batTipX, batTipY, 13, SDL_Color{158, 102, 50, 255});
        limb(batMidX, batMidY - 1, batTipX + 4, batTipY - 2, 4,
             SDL_Color{198, 140, 74, 255});
        box(batTipX - 2, batTipY - 7, 4, 14, SDL_Color{118, 74, 36, 255});

        if (impact > 0.0f) {
            float ia = clampf(1.0f - (t - 1.05f) / 0.35f, 0.0f, 1.0f);
            // кровь на бойке
            box(batTipX + 3, batTipY - 6, 10, 8, SDL_Color{150, 30, 40, 235});
            box(batTipX + 5, batTipY - 8, 5, 2, SDL_Color{170, 36, 46, 235});
            int cx = px + 46, cy = py + 8;
            SDL_SetRenderDrawColor(ren, 255, 238, 150,
                                   static_cast<Uint8>(220 * ia));
            for (int i = 0; i < 8; ++i) {
                float a = i * 0.785398f;
                int len = 6 + static_cast<int>(10.0f * impact);
                SDL_RenderDrawLine(ren, cx, cy,
                                   cx + static_cast<int>(std::cos(a) * len),
                                   cy + static_cast<int>(std::sin(a) * len));
            }
        } else if (u > 0.25f) {
            // след движения биты
            SDL_SetRenderDrawColor(ren, 230, 210, 180, 55);
            SDL_RenderDrawLine(ren, hx - 32, hy - 24, hx - 12, hy - 3);
            SDL_RenderDrawLine(ren, hx - 20, hy - 6, hx - 3, hy + 6);
        }
    } else {
        // Смерть от добермана: пёс сбивает верхнюю часть Коляна,
        // а внутренняя рамка постепенно заливается кровью по частям.
        float u = clampf((t - 0.10f) / 0.80f, 0.0f, 1.0f);
        float impact = clampf((t - 0.90f) / 0.12f, 0.0f, 1.0f);
        float detach = clampf((t - 0.90f) / 0.65f, 0.0f, 1.0f);
        bool hit = (t >= 0.90f);
        float jt = clampf((t - 0.90f) / 0.30f, 0.0f, 1.0f);
        int jolt = static_cast<int>(-std::sin(jt * 3.14159265f) * 5.0f);

        int px = panel.x + 22 + jolt;
        int py = panel.y + 18;

        const SDL_Color skin{238, 190, 150, 255};
        const SDL_Color skinShade{206, 156, 122, 255};
        const SDL_Color hair{62, 40, 30, 255};

        // нижняя часть Коляна остаётся стоять
        box(px - 6, py + 56, 52, 22, SDL_Color{52, 84, 148, 255});
        box(px - 6, py + 56, 52, 4, SDL_Color{190, 58, 62, 255});
        if (hit) {
            box(px + 14, py + 44, 20, 12, SDL_Color{126, 24, 34, 255});
            box(px + 16, py + 44, 16, 3, SDL_Color{232, 220, 200, 255});
            box(px + 18, py + 47, 3, 2, SDL_Color{180, 40, 48, 255});
            box(px + 27, py + 47, 3, 2, SDL_Color{180, 40, 48, 255});
            box(px + 15, py + 50, 2, 6, SDL_Color{150, 28, 38, 220});
            box(px + 31, py + 49, 1, 5, SDL_Color{140, 26, 36, 200});
        } else {
            box(px + 14, py + 44, 20, 12, skinShade);
        }

        // верхняя часть: до удара на месте, после — отлетает влево-вверх
        int ux = px - static_cast<int>(30.0f * detach);
        int uy = py - static_cast<int>(22.0f * detach);
        if (!hit) {
            ux = px;
            uy = py;
        }
        box(ux - 2, uy + 38, 44, 16, SDL_Color{52, 84, 148, 255});
        box(ux - 2, uy + 38, 44, 4, SDL_Color{190, 58, 62, 255});
        box(ux + 4, uy + 8, 30, 30, skin);
        box(ux + 4, uy + 34, 26, 6, skinShade);
        fillTriangle(ux + 4, uy + 18, ux - 3, uy + 24, ux + 4, uy + 28,
                     skinShade);
        box(ux + 1, uy + 3, 34, 8, hair);
        box(ux + 30, uy + 8, 8, 24, hair);
        box(ux + 3, uy + 6, 8, 3, hair);
        if (impact < 0.5f) {
            box(ux + 9, uy + 15, 8, 5, SDL_Color{248, 246, 240, 255});
            box(ux + 11, uy + 16, 3, 3, SDL_Color{30, 22, 24, 255});
        } else {
            SDL_SetRenderDrawColor(ren, 56, 16, 24, 255);
            SDL_RenderDrawLine(ren, ux + 9, uy + 15, ux + 16, uy + 20);
            SDL_RenderDrawLine(ren, ux + 16, uy + 15, ux + 9, uy + 20);
        }
        if (hit) {
            box(ux + 18, uy + 44, 18, 4, SDL_Color{126, 24, 34, 255});
            box(ux + 22, uy + 44, 8, 2, SDL_Color{232, 220, 200, 255});
        }

        // доберман бросается справа и сбивает верхнюю часть
        int dogX = panel.x + 104 - static_cast<int>(u * 30.0f);
        int dogY = panel.y + 40;
        const SDL_Color fur{34, 28, 30, 255};
        const SDL_Color furLight{62, 50, 48, 255};
        const SDL_Color tan{126, 86, 52, 255};

        box(dogX, dogY, 30, 18, fur);
        box(dogX + 4, dogY + 12, 22, 6, furLight);
        box(dogX + 3, dogY + 18, 5, 12, fur);
        box(dogX + 11, dogY + 18, 5, 12, fur);
        box(dogX + 21, dogY + 18, 5, 12, fur);
        box(dogX + 28, dogY - 8, 5, 12, fur);
        box(dogX + 29, dogY - 10, 4, 3, furLight);
        box(dogX - 12, dogY - 2, 16, 14, fur);
        box(dogX - 9, dogY - 8, 4, 7, fur);
        box(dogX - 2, dogY - 8, 4, 7, fur);
        box(dogX - 11, dogY - 1, 14, 3, tan);

        int jaw = hit ? 4 : (6 + static_cast<int>(std::sin(t * 30.0f)));
        box(dogX - 16, dogY + 2, 6, 4, fur);
        box(dogX - 16, dogY + 2 + jaw, 6, 4, fur);
        box(dogX - 15, dogY + 5, 4, 1, SDL_Color{240, 236, 226, 255});
        box(dogX - 15, dogY + 1 + jaw, 4, 1, SDL_Color{240, 236, 226, 255});
        box(dogX - 6, dogY + 1, 2, 2, SDL_Color{240, 60, 52, 255});

        if (impact > 0.0f) {
            int nx = px + 22;
            int ny = py + 42;
            float fade = clampf(1.0f - (t - 1.05f) / 0.90f, 0.0f, 1.0f);
            for (int i = 0; i < 8; ++i) {
                float a = -0.4f + i * 0.75f;
                int len = 8 + static_cast<int>(14.0f * impact);
                SDL_SetRenderDrawColor(ren, 198, 32, 42,
                                       static_cast<Uint8>(220 * fade));
                SDL_RenderDrawLine(ren, nx, ny,
                                   nx + static_cast<int>(std::cos(a) * len),
                                   ny - static_cast<int>(std::sin(a) * len));
            }
        }

        // рамка изнутри краснеет по частям
        if (hit) {
            float stain = clampf((t - 0.95f) / 1.30f, 0.0f, 1.0f);
            static const int spots[][4] = {
                {5, 5, 22, 4},  {32, 5, 26, 4},  {63, 5, 27, 4},
                {5, 14, 4, 24}, {5, 44, 4, 22},  {5, 72, 4, 19},
                {86, 12, 4, 26}, {86, 44, 4, 24}, {86, 74, 4, 18},
                {12, 84, 24, 4}, {42, 84, 20, 4}, {68, 84, 22, 4},
                {16, 34, 9, 7}, {66, 52, 12, 8},  {28, 60, 8, 6},
                {50, 26, 7, 9}
            };
            const int total = static_cast<int>(sizeof(spots) / sizeof(spots[0]));
            for (int i = 0; i < total; ++i) {
                if (stain < static_cast<float>(i + 1) / total) continue;
                Uint8 a = static_cast<Uint8>(120 + 90 * ((i % 3) / 2.0f));
                box(panel.x + spots[i][0], panel.y + spots[i][1],
                    spots[i][2], spots[i][3], SDL_Color{116, 18, 28, a});
            }
        }
    }

    SDL_RenderSetClipRect(ren, nullptr);
}

void Game::drawGameOverScreen()
{
    SDL_SetRenderDrawColor(ren, 12, 3, 7, 195);
    SDL_RenderFillRect(ren, nullptr);

    SDL_SetRenderDrawColor(ren, 168, 62, 68, 150);
    SDL_Rect l1{64, 62, VIEW_W - 128, 1};
    SDL_Rect l2{64, 198, VIEW_W - 128, 1};
    SDL_RenderFillRect(ren, &l1);
    SDL_RenderFillRect(ren, &l2);

    pixfont::drawTextCentered(ren, VIEW_W / 2, 76, "ИГРА ОКОНЧЕНА", 3,
                              SDL_Color{232, 96, 92, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 120,
                              "КОЛЯН ПОГИБ В АПАРТАМЕНТАХ «ДАЙМОНД»", 1,
                              SDL_Color{218, 200, 190, 255});
    const char* causeText =
        (deathCause == DEATH_SPIDER)   ? "ПОСЛЕДНЯЯ ПРИЧИНА: ПАУК"
        : (deathCause == DEATH_BULLET) ? "ПОСЛЕДНЯЯ ПРИЧИНА: ПУЛЯ"
        : (deathCause == DEATH_DOBERMAN) ? "ПОСЛЕДНЯЯ ПРИЧИНА: ДОБЕРМАН"
        : (deathCause == DEATH_BOSS)   ? "ПОСЛЕДНЯЯ ПРИЧИНА: ГИГАНТ С ПАРКОВКИ"
        : (deathCause == DEATH_GRENADE) ? "ПОСЛЕДНЯЯ ПРИЧИНА: ГРАНАТА"
        : (deathCause == DEATH_FALL)   ? "ПОСЛЕДНЯЯ ПРИЧИНА: ПАДЕНИЕ В ПРОПАСТЬ"
                                        : "ПОСЛЕДНЯЯ ПРИЧИНА: БРАТОК С БИТОЙ";
    SDL_Color causeCol =
        (deathCause == DEATH_SPIDER)   ? SDL_Color{196, 110, 166, 255}
        : (deathCause == DEATH_BULLET) ? SDL_Color{236, 196, 96, 255}
        : (deathCause == DEATH_DOBERMAN) ? SDL_Color{226, 90, 76, 255}
        : (deathCause == DEATH_BOSS)   ? SDL_Color{236, 118, 108, 255}
        : (deathCause == DEATH_GRENADE) ? SDL_Color{244, 168, 86, 255}
        : (deathCause == DEATH_FALL)   ? SDL_Color{150, 136, 120, 255}
                                         : SDL_Color{206, 154, 96, 255};
    pixfont::drawTextCentered(ren, VIEW_W / 2, 138, causeText, 1, causeCol);

    std::string stats = "МОНСТРОВ ПОВЕРЖЕНО: " + std::to_string(kills) +
                        "   ОЧКИ: " + std::to_string(score);
    pixfont::drawTextCentered(ren, VIEW_W / 2, 160, stats, 1,
                              SDL_Color{180, 168, 152, 255});

    if (std::fmod(titleBlink, 1.15f) < 0.78f) {
        pixfont::drawTextCentered(ren, VIEW_W / 2, 210,
                                  touch.enabled ? "ЗАНОВО - СНАЧАЛА"
                                             : "R - ЗАНОВО", 2,
                                  SDL_Color{255, 226, 140, 255});
    }
}

} // namespace kolek
