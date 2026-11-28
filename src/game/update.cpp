#include "game/game.h"
#include "core/math.h"
#include "data/levels.h"

namespace kolek {

void Game::requestExit(ExitAction action)
{
    if (exitAction == action) return;
    exitAction = action;
    exitAccepted = false;
}

void Game::update(float dt, const Input& in)
{
    if (exitAction != EXIT_NONE) {
        // Мир и все таймеры стоят, пока игрок принимает решение.
        if (in.back) {
            exitAction = EXIT_NONE;
        } else {
            if (in.menuUp || in.menuDown || in.menuLeft || in.menuRight)
                exitAccepted = !exitAccepted;
            if (in.confirm) {
                if (exitAccepted) {
                    if (exitAction == EXIT_GAME) {
                        quitRequested = true;
                    } else {
                        state = ST_TITLE;
                        menuIndex = 0;
                        titleBlink = 0;
                    }
                }
                exitAction = EXIT_NONE;
            }
        }
        // Пробел подтверждения не должен превращаться в прыжок после отмены.
        jumpEdge = false;
        prevSpacePressed = in.spacePressed;
        return;
    }

    // Нажатие прыжка считаем один раз за реальное нажатие: главный цикл
    // переиспользует один Input для нескольких подшагов физики.
    jumpEdge = in.spacePressed && !prevSpacePressed;
    prevSpacePressed = in.spacePressed;

    timeSec += dt;
    titleBlink += dt;
    updateParticles(dt);
    updateScorePopups(dt);
    updateTracers(dt);
    shake = std::max(0.0f, shake - dt * 1.9f);
    hurtFlash = std::max(0.0f, hurtFlash - dt * 2.4f);

    if (state == ST_TITLE) {
        if (in.back) {
            requestExit(EXIT_GAME);
            return;
        }
        if (in.menuUp)
            menuIndex = (menuIndex + 2) % 3;
        if (in.menuDown)
            menuIndex = (menuIndex + 1) % 3;
        if (in.confirm) {
            if (menuIndex == 0) {
                startGame();
                // Пробел, которым начали игру, не должен сразу превращаться в прыжок.
                jumpEdge = false;
                prevSpacePressed = true;
            } else if (menuIndex == 1) {
                state = ST_SETTINGS;
                settingsIndex = 0;
                settingsSlot = 0;
                bindTarget = -1;
            } else {
                requestExit(EXIT_GAME);
            }
        }
        return;
    }
    if (state == ST_SETTINGS) {
        const int rows = CA_COUNT + 1;
        if (bindTarget >= 0) return;
        if (in.menuUp)
            settingsIndex = (settingsIndex + rows - 1) % rows;
        if (in.menuDown)
            settingsIndex = (settingsIndex + 1) % rows;
        if (settingsIndex < CA_COUNT) {
            if (in.menuLeft) settingsSlot = 0;
            if (in.menuRight) settingsSlot = 1;
        }
        if (in.back) {
            state = ST_TITLE;
            menuIndex = 1;
            return;
        }
        if (in.confirm) {
            if (settingsIndex < CA_COUNT) {
                bindTarget = settingsIndex;
                bindSlot = settingsSlot;
            } else {
                state = ST_TITLE;
                menuIndex = 1;
            }
        }
        return;
    }
    if (state == ST_LEVEL_SELECT) {
        // DEV: выбор уровня из секретной двери.
        if (in.menuUp)
            levelSelectIndex = (levelSelectIndex + LEVEL_COUNT - 1) % LEVEL_COUNT;
        if (in.menuDown)
            levelSelectIndex = (levelSelectIndex + 1) % LEVEL_COUNT;
        if (in.back) {
            secretDoorActive = false;
            state = ST_PLAY;
            return;
        }
        if (in.confirm) {
            secretDoorActive = false;
            kills = 0;
            score = 0;
            lives = START_LIVES;
            deathTimer = 0;
            resetWorld(levelSelectIndex);
            state = ST_PLAY;
            return;
        }
        return;
    }
    if (state == ST_SLIDE) {
        if (in.confirm || in.back) {
            ++slideIndex;
            if (slideIndex >= static_cast<int>(slideTexts.size())) {
                int next = slideNextLevel;
                slideNextLevel = -1;
                slideIndex = 0;
                slideTexts.clear();
                slideWallScene = false;
                resetWorld(next);
                spawnSparkle(player.x + player.w * 0.5f,
                             player.y + player.h * 0.5f, 26,
                             SDL_Color{255, 220, 130, 255});
                shake = 0.25f;
            }
        }
        return;
    }
    if (state == ST_WIN || state == ST_GAMEOVER) {
        if (in.back) {
            requestExit(EXIT_LEVEL);
            return;
        }
        if (in.restart) startGame();
        return;
    }
    if (state == ST_DYING) {
        if (in.back) {
            requestExit(EXIT_LEVEL);
            return;
        }
        updateDeath(dt);
        return;
    }
    if (in.back) {
        requestExit(EXIT_LEVEL);
        return;
    }
    if (DEV_SECRET_LEVELS && in.secretDoor)
        toggleSecretDoor();
    if (in.restart) {
        startGame();
        return;
    }

    updatePlayer(dt, in);
    updateWeapon(dt, in);
    updateMonsters(dt);
    updateBullets(dt);
    updateGrenades(dt);
    checkPickups();
    checkDoor(dt);

    // Парковка: как только босс повержен, короткая пауза — и победа.
    if (levelIndex == 3 && bossDefeated && state == ST_PLAY) {
        bossWinTimer += dt;
        if (bossWinTimer > BOSS_WIN_DELAY) {
            bossWinTimer = 0;
            state = ST_WIN;
            spawnConfetti();
        }
    }

    // В вестибюле под полом — тёмная шахта: упал в пропасть, значит погиб.
    if (levelIndex == 2 && player.y > (lvl.h - 4) * TILE) {
        player.invuln = 0;
        killPlayer(DEATH_FALL);
    }
    updateCamera(dt);

    // DEV: вход в секретную дверь открывает выбор уровня.
    if (DEV_SECRET_LEVELS && secretDoorActive &&
        overlapRect(player.x, player.y, player.w, player.h,
                    secretDoor.x, secretDoor.y, secretDoor.w, secretDoor.h)) {
        secretDoorActive = false;
        levelSelectIndex = levelIndex;
        state = ST_LEVEL_SELECT;
        return;
    }

    introTimer += dt;
}

} // namespace kolek
