#include "game/game.h"
#include "core/math.h"
#include "data/levels.h"

namespace kolek {

void Game::checkPickups()
{
    // Кубки: +1000 очков и один замок на двери.
    for (CupItem& c : cups) {
        if (c.taken) continue;
        RectF cr{c.tx * 16.0f + 2.0f, c.ty * 16.0f + 3.0f, 12.0f, 11.0f};
        if (overlapRect(player.x, player.y, player.w, player.h,
                        cr.x, cr.y, cr.w, cr.h)) {
            c.taken = true;
            ++cupsGot;
            score += CUP_SCORE;
            addScorePopup(CUP_SCORE, SDL_Color{255, 226, 140, 255});
            lvl.rows[c.ty][c.tx] = '.';
            spawnSparkle(cr.x + 6, cr.y + 6, 18, SDL_Color{255, 220, 110, 255});
            spawnSparkle(cr.x + 6, cr.y + 6, 8, SDL_Color{255, 250, 220, 255});

            // Чекпоинт — место взятого кубка. Ищем пол под кубком, чтобы
            // Колян после смерти встал на твёрдую поверхность.
            int floorRow = c.ty + 1;
            while (floorRow < lvl.h && !lvl.solid(c.tx, floorRow)) ++floorRow;
            float cpx = c.tx * TILE + 3.0f;
            float cpy = ((floorRow < lvl.h) ? floorRow * TILE
                                            : (c.ty + 1) * TILE) - PLAYER_H;
            checkpoint = RectF{cpx, cpy, player.w, player.h};
        }
    }

    // Украшения: 200, 400 или 600 очков.
    for (Jewel& jw : jewelry) {
        if (jw.taken) continue;
        if (overlapRect(player.x, player.y, player.w, player.h,
                        jw.x - 2.0f, jw.y - 2.0f, 15.0f, 14.0f)) {
            jw.taken = true;
            score += JEWEL_SCORE[jw.type];
            SDL_Color glint = (jw.type == 0) ? SDL_Color{170, 220, 255, 255}
                            : (jw.type == 1) ? SDL_Color{140, 255, 170, 255}
                                             : SDL_Color{255, 215, 120, 255};
            addScorePopup(JEWEL_SCORE[jw.type], glint);
            spawnSparkle(jw.x + 5.0f, jw.y + 5.0f, 14, glint);
        }
    }
    jewelry.erase(std::remove_if(jewelry.begin(), jewelry.end(),
                                 [](const Jewel& jw) { return jw.taken; }),
                  jewelry.end());
}

void Game::checkDoor(float dt)
{
    if (door.w <= 0) return;
    bool inDoor = overlapRect(player.x, player.y, player.w, player.h,
                              door.x + 4.0f, door.y, door.w - 8.0f, door.h);
    if (!inDoor) {
        winDelay = std::max(0.0f, winDelay - dt * 2.0f);
        return;
    }
    if (cupsGot < static_cast<int>(cups.size())) {
        winDelay = 0;
        return;
    }
    winDelay += dt;
    if (winDelay > 0.8f) {
        winDelay = 0;
        advanceLevel();
    }
}

void Game::advanceLevel()
{
    if (levelIndex + 1 < LEVEL_COUNT) {
        beginSlides(levelIndex + 1);
    } else {
        spawnSparkle(door.x + door.w * 0.5f, door.y + door.h * 0.5f, 40,
                     SDL_Color{255, 225, 130, 255});
        state = ST_WIN;
        spawnConfetti();
    }
}

void Game::beginSlides(int nextLevel)
{
    slideTexts.clear();
    slideIndex = 0;
    slideNextLevel = nextLevel;
    slideWallScene = false;

    // Между первым и вторым этажом — короткая сцена с Лёхой.
    if (levelIndex == 0 && nextLevel == 1) {
        slideTexts.push_back("КОЛЯН. ЛЁХА ТЕБЯ КИНУЛ");
        slideTexts.push_back("ЭТО СОМНИТЕЛЬНЫЕ АППАРТАМЕНТЫ, НАЙДИ ВЫХОД НАРУЖУ");
    }

    // После технического этажа — стена у лифта, на которой Лёха оставил
    // свой след. Слайд рисуется как кадр с надписью, а не как текст.
    if (levelIndex == 1 && nextLevel == 2) {
        slideWallScene = true;
        slideTexts.push_back("ЛЁХА БЫЛ ЗДЕСЬ");
    }

    if (slideTexts.empty()) {
        // Для переходов без слайдов — сразу следующий уровень.
        int next = slideNextLevel;
        slideNextLevel = -1;
        resetWorld(next);
        spawnSparkle(player.x + player.w * 0.5f, player.y + player.h * 0.5f, 26,
                     SDL_Color{255, 220, 130, 255});
        shake = 0.25f;
    } else {
        state = ST_SLIDE;
    }
}

void Game::toggleSecretDoor()
{
    if (!DEV_SECRET_LEVELS) return;

    // Повторное Ctrl+R убирает дверь.
    if (secretDoorActive) {
        secretDoorActive = false;
        return;
    }

    auto support = [&](int tx, int ty) {
        return lvl.solid(tx, ty) || lvl.stepTile(tx, ty);
    };

    int dir = (player.facing >= 0) ? 1 : -1;
    int baseTx = static_cast<int>((player.x + player.w * 0.5f) / TILE);
    int feetRow = static_cast<int>((player.y + player.h + 1.0f) / TILE);
    feetRow = std::max(0, std::min(feetRow, lvl.h - 1));

    bool placed = false;
    for (int d = 2; d <= 6 && !placed; ++d) {
        int tx = baseTx + dir * d;
        if (tx <= 0 || tx >= lvl.w - 1) continue;

        int fy = feetRow;
        while (fy < lvl.h && !support(tx, fy)) ++fy;
        if (fy >= lvl.h) continue;

        bool clear = true;
        for (int k = 1; k <= 3; ++k)
            if (fy - k < 0 || support(tx, fy - k)) clear = false;
        if (!clear) continue;

        secretDoor = RectF{static_cast<float>(tx * TILE),
                           static_cast<float>((fy - 3) * TILE),
                           static_cast<float>(TILE), static_cast<float>(3 * TILE)};
        placed = true;
    }

    if (!placed) {
        // Если впереди стена или обрыв — ставим дверь прямо перед Коляном.
        float x = player.x + player.w * 0.5f + dir * 28.0f - 8.0f;
        float y = player.y + player.h - 3 * TILE;
        secretDoor = RectF{x, y, TILE, 3 * TILE};
    }

    secretDoorActive = true;
    spawnSparkle(secretDoor.x + secretDoor.w * 0.5f,
                 secretDoor.y + secretDoor.h * 0.5f, 18,
                 SDL_Color{90, 230, 245, 255});
    shake = std::max(shake, 0.12f);
    SDL_Log("devdoor: spawned at %.0f,%.0f level=%d", secretDoor.x,
            secretDoor.y, levelIndex + 1);
}

void Game::respawnPlayer()
{
    player.x = checkpoint.x;
    player.y = checkpoint.y;
    player.vx = player.vy = 0;
    player.climbing = false;
    player.grounded = false;
    player.invuln = 1.6f;
    player.hurt = 0;
    player.airJumps = 1;
    player.lookY = 0;
    snapCamera();
}

void Game::killPlayer(int cause)
{
    if (state != ST_PLAY || player.invuln > 0) return;

    --lives;
    deathCause = cause;
    deathTimer = 0;
    state = ST_DYING;

    player.vx = player.vy = 0;
    shake = 0.8f;
    hurtFlash = 0.9f;
    SDL_Color blood = (cause == DEATH_DOBERMAN) ? SDL_Color{130, 26, 32, 255}
                    : (cause == DEATH_FALL)     ? SDL_Color{120, 108, 96, 255}
                                                : SDL_Color{210, 70, 70, 255};
    spawnSparkle(player.x + player.w * 0.5f, player.y + player.h * 0.5f, 22, blood);
    bullets.clear();
}

void Game::updateDeath(float dt)
{
    deathTimer += dt;
    if (deathTimer < DEATH_TIME) return;

    if (lives > 0) {
        respawnPlayer();
        mag = MAG_SIZE;
        reloading = false;
        shootCd = standStill = muzzle = gunKick = 0;
        reloadTimer = 0;
        reloadDuration = RELOAD_FULL;
        prevSpacePressed = false;
        jumpEdge = false;
        introTimer = 20.0f;
        state = ST_PLAY;
        spawnSparkle(player.x + player.w * 0.5f, player.y + player.h * 0.5f, 14,
                     SDL_Color{130, 200, 255, 255});
    } else {
        state = ST_GAMEOVER;
        spawnSparkle(player.x + player.w * 0.5f, player.y + player.h * 0.5f, 20,
                     SDL_Color{90, 20, 30, 255});
    }
}

} // namespace kolek
