#include "game/game.h"

namespace kolek {

void Game::screenshot(const char* path)
{
    int ow = 0, oh = 0;
    SDL_GetRendererOutputSize(ren, &ow, &oh);
    if (ow <= 0 || oh <= 0) return;
    std::vector<uint32_t> px(static_cast<size_t>(ow) * oh, 0);
    if (SDL_RenderReadPixels(ren, nullptr, SDL_PIXELFORMAT_ARGB8888,
                             px.data(), ow * 4) != 0) {
        return;
    }
    SDL_Surface* s = SDL_CreateRGBSurfaceWithFormatFrom(
        px.data(), ow, oh, 32, ow * 4, SDL_PIXELFORMAT_ARGB8888);
    if (s) {
        SDL_SaveBMP(s, path);
        SDL_FreeSurface(s);
    }
}

void Game::makeScenarios()
{
    auto takeCup = [&](int tx, int ty) {
        for (CupItem& c : cups) {
            if (c.tx == tx && c.ty == ty && !c.taken) {
                c.taken = true;
                lvl.rows[c.ty][c.tx] = '.';
                ++cupsGot;
                score += CUP_SCORE;
            }
        }
    };

    // 0 — титульный экран
    resetWorld(0);
    state = ST_TITLE;
    render();
    screenshot("shot_title.bmp");
    SDL_RenderPresent(ren);

    requestExit(EXIT_GAME);
    render();
    screenshot("shot_exit_game.bmp");
    SDL_RenderPresent(ren);
    exitAction = EXIT_NONE;

    // 0b — настройки управления
    resetWorld(0);
    state = ST_SETTINGS;
    menuIndex = 1;
    settingsIndex = 4;
    settingsSlot = 1;
    render();
    screenshot("shot_settings.bmp");
    SDL_RenderPresent(ren);

    // 1 — самое начало
    resetWorld(0);
    state = ST_PLAY;
    introTimer = 1.5f;
    render();
    screenshot("shot_start.bmp");
    SDL_RenderPresent(ren);

    requestExit(EXIT_LEVEL);
    render();
    screenshot("shot_exit_level.bmp");
    SDL_RenderPresent(ren);
    exitAction = EXIT_NONE;

    // 1b — подвал: браток с пистолетом и пуля в полёте
    resetWorld(0);
    state = ST_PLAY;
    {
        Monster thrower;
        bool found = false;
        for (const Monster& m : monsters) {
            if (m.type == 2) { thrower = m; found = true; break; }
        }
        monsters.clear();
        if (found) {
            thrower.b.x = 120 * 16.0f;
            thrower.b.y = 42 * 16.0f - thrower.b.h;
            thrower.homeX = thrower.b.x;
            thrower.homeY = thrower.b.y;
            thrower.b.facing = -1;
            thrower.throwCd = 0.0f;
            monsters.push_back(thrower);
        }
    }
    player.x = 110 * 16.0f;
    player.y = 42 * 16.0f - PLAYER_H;
    player.facing = 1;
    player.vx = player.vy = 0;
    player.grounded = true;
    snapCamera();
    introTimer = 20.0f;
    for (int i = 0; i < 120; ++i) update(1.0f / 120.0f, Input{});
    snapCamera();
    render();
    screenshot("shot_basement.bmp");
    SDL_RenderPresent(ren);

    // 1c — ступени-зигзаг
    resetWorld(0);
    state = ST_PLAY;
    monsters.clear();
    player.x = 48 * 16.0f + 19.0f;
    player.y = 41 * 16.0f - PLAYER_H;
    player.vx = player.vy = 0;
    player.grounded = true;
    player.facing = 1;
    snapCamera();
    introTimer = 20.0f;
    render();
    screenshot("shot_stairs.bmp");
    SDL_RenderPresent(ren);

    // 1d — паук на потолке и спуск на паутине
    resetWorld(0);
    state = ST_PLAY;
    monsters.clear();
    {
        Monster m;
        m.type = 0;
        m.b.w = 16;
        m.b.h = 18;
        m.b.x = 83 * 16.0f;
        m.b.y = 420.0f;          // потолок нижней галереи (ряд 25)
        m.ceilingY = m.b.y;
        m.b.facing = 1;
        m.hp = m.maxHp = 3;
        monsters.push_back(m);
    }
    player.x = 83 * 16.0f + 3.0f;
    player.y = 32 * 16.0f - PLAYER_H;
    player.vx = player.vy = 0;
    player.grounded = true;
    player.facing = 1;
    player.invuln = 0;
    snapCamera();
    for (int i = 0; i < 4; ++i) update(1.0f / 120.0f, Input{});
    player.invuln = 99.0f;
    for (int i = 0; i < 200 && (monsters.empty() ||
         monsters.front().webState != 2); ++i)
        update(1.0f / 120.0f, Input{});
    player.invuln = 0.0f;         // иначе Колян мигает и может не попасть в кадр
    camY = 380.0f;
    introTimer = 20.0f;
    render();
    screenshot("shot_spider.bmp");
    SDL_RenderPresent(ren);

    // 2 — бальный зал нижнего этажа
    resetWorld(0);
    state = ST_PLAY;
    takeCup(10, 7);
    player.x = 62 * 16.0f;
    player.y = 482.0f;
    player.facing = 1;
    snapCamera();
    addScorePopup(CUP_SCORE, SDL_Color{255, 226, 140, 255});
    introTimer = 20.0f;
    render();
    screenshot("shot_ballroom.bmp");
    SDL_RenderPresent(ren);

    // 3 — бой: диагональный выстрел вверх по братку
    resetWorld(0);
    state = ST_PLAY;
    monsters.clear();
    player.x = 17 * 16.0f;
    player.y = 482.0f;
    player.facing = 1;
    player.lookY = -1;
    player.grounded = true;
    {
        Monster m;
        m.type = 1;
        m.b.w = 14;
        m.b.h = 26;
        m.b.x = player.x + 45.0f;
        m.b.y = player.y - 20.0f;
        m.b.facing = -1;
        m.hp = m.maxHp = 20;
        m.homeX = m.b.x;
        m.homeY = m.b.y;
        m.seed = 1.7f;
        monsters.push_back(m);
    }
    snapCamera();
    introTimer = 20.0f;
    {
        Input in;
        in.lookUp = true;
        in.shootPressed = true;
        update(1.0f / 120.0f, in);
        // Даём вспышке чуть погаснуть, но держим прицел вверх: видны трассеры.
        for (int i = 0; i < 8; ++i) {
            Input hold;
            hold.lookUp = true;
            update(1.0f / 120.0f, hold);
        }
    }
    snapCamera();
    render();
    screenshot("shot_combat.bmp");
    SDL_RenderPresent(ren);

    // 4 — верхний этаж
    resetWorld(0);
    state = ST_PLAY;
    takeCup(10, 7);
    takeCup(125, 24);
    player.x = 8 * 16.0f;
    player.y = 98.0f;
    player.facing = 1;
    snapCamera();
    introTimer = 20.0f;
    render();
    screenshot("shot_upper.bmp");
    SDL_RenderPresent(ren);

    // 5 — парадная дверь в подвале
    resetWorld(0);
    state = ST_PLAY;
    for (CupItem& c : cups) {
        if (!c.taken) {
            c.taken = true;
            lvl.rows[c.ty][c.tx] = '.';
            ++cupsGot;
            score += CUP_SCORE;
        }
    }
    player.x = 150 * 16.0f;
    player.y = 42 * 16.0f - PLAYER_H;
    player.vx = player.vy = 0;
    player.grounded = true;
    player.facing = 1;
    snapCamera();
    introTimer = 20.0f;
    render();
    screenshot("shot_door.bmp");
    SDL_RenderPresent(ren);

    // 5b — слайды между первым и вторым уровнем
    resetWorld(0);
    beginSlides(1);
    render();
    screenshot("shot_slide1.bmp");
    SDL_RenderPresent(ren);

    slideIndex = 1;
    render();
    screenshot("shot_slide2.bmp");
    SDL_RenderPresent(ren);

    // 5b2 — слайд после второго уровня: стена с надписью «ЛЁХА БЫЛ ЗДЕСЬ»
    resetWorld(1);
    beginSlides(2);
    render();
    screenshot("shot_slide3.bmp");
    SDL_RenderPresent(ren);

    // 5b3 — слайд перед парковкой: джип Лёхиного водилы
    resetWorld(2);
    beginSlides(3);
    render();
    screenshot("shot_slide_parking.bmp");
    SDL_RenderPresent(ren);

    // 5c — DEV: секретная дверь и экран выбора уровня
    resetWorld(0);
    state = ST_PLAY;
    player.facing = 1;
    toggleSecretDoor();
    introTimer = 20.0f;
    snapCamera();
    render();
    screenshot("shot_devdoor.bmp");
    SDL_RenderPresent(ren);

    state = ST_LEVEL_SELECT;
    levelSelectIndex = 1;
    render();
    screenshot("shot_levelselect.bmp");
    SDL_RenderPresent(ren);

    // 6 — второй уровень: винтовая шахта-зигзаг на нижнем этаже
    resetWorld(1);
    state = ST_PLAY;
    player.x = 40 * 16.0f;
    player.y = 482.0f;
    player.facing = 1;
    takeCup(12, 7);
    snapCamera();
    introTimer = 20.0f;
    render();
    screenshot("shot_level2.bmp");
    SDL_RenderPresent(ren);

    // 6b — доберман: низкий хитбокс и погоня
    resetWorld(1);
    state = ST_PLAY;
    monsters.clear();
    player.x = 40 * 16.0f;
    player.y = 482.0f;
    player.facing = 1;
    player.grounded = true;
    {
        Monster d;
        d.type = 3;
        d.b.w = 24;
        d.b.h = 14;
        d.b.x = 45 * 16.0f;
        d.b.y = 32 * 16.0f - d.b.h;
        d.b.facing = -1;
        d.hp = d.maxHp = 2;
        d.homeX = d.b.x;
        d.homeY = d.b.y;
        d.anim = 2.4f;
        monsters.push_back(d);
    }
    snapCamera();
    introTimer = 20.0f;
    render();
    screenshot("shot_doberman.bmp");
    SDL_RenderPresent(ren);

    // 6c — выход со второго уровня: дверь-лифт с собранными кубками
    resetWorld(1);
    state = ST_PLAY;
    for (CupItem& c : cups) c.taken = true;
    cupsGot = static_cast<int>(cups.size());
    player.x = 150 * 16.0f + 3.0f;
    player.y = 32 * 16.0f - PLAYER_H;
    player.facing = 1;
    player.vx = player.vy = 0;
    player.grounded = true;
    snapCamera();
    introTimer = 20.0f;
    render();
    screenshot("shot_elevator.bmp");
    SDL_RenderPresent(ren);

    // 6d — третий уровень: мраморный вестибюль, платформы и пропасть
    resetWorld(2);
    state = ST_PLAY;
    player.x = 36 * 16.0f + 3.0f;
    player.y = (lvl.h - 6) * TILE - PLAYER_H;
    player.facing = 1;
    player.vx = player.vy = 0;
    player.grounded = true;
    takeCup(43, 7);
    snapCamera();
    introTimer = 20.0f;
    render();
    screenshot("shot_level3.bmp");
    SDL_RenderPresent(ren);

    // 6e — четвёртый уровень: ночная парковка, бетонные блоки и босс у джипа
    resetWorld(3);
    state = ST_PLAY;
    player.x = 7 * 16.0f + 3.0f;
    player.y = 17 * 16.0f - PLAYER_H;
    player.facing = 1;
    player.vx = player.vy = 0;
    player.grounded = true;
    for (Monster& m : monsters) {
        if (m.type == 4) {
            m.anim = 1.3f;
            m.hp = 42;
        }
    }
    snapCamera();
    introTimer = 20.0f;
    render();
    screenshot("shot_parking.bmp");
    SDL_RenderPresent(ren);

    // 7 — победа над боссом парковки
    resetWorld(3);
    monsters.clear();
    state = ST_WIN;
    kills = 9;
    score = 24000;
    spawnConfetti();
    spawnConfetti();
    for (int i = 0; i < 75; ++i) updateParticles(1.0f / 60.0f);
    render();
    screenshot("shot_win.bmp");
    SDL_RenderPresent(ren);

    // 8 — анимация смерти от паука
    resetWorld(0);
    state = ST_PLAY;
    player.x = 17 * 16.0f;
    player.y = 482.0f;
    player.facing = 1;
    player.grounded = true;
    snapCamera();
    introTimer = 20.0f;
    lives = 2;
    deathCause = DEATH_SPIDER;
    deathTimer = 1.45f;
    titleBlink = 0.25f;
    state = ST_DYING;
    render();
    screenshot("shot_death.bmp");
    SDL_RenderPresent(ren);

    // 8b — браток с битой сносит Коляну голову
    deathCause = DEATH_BAT;
    deathTimer = 1.60f;
    render();
    screenshot("shot_death_bat.bmp");
    SDL_RenderPresent(ren);

    // 8c — пуля между глаз (вид в профиль)
    resetWorld(0);
    state = ST_PLAY;
    player.x = 17 * 16.0f;
    player.y = 482.0f;
    player.facing = 1;
    player.grounded = true;
    snapCamera();
    introTimer = 20.0f;
    lives = 2;
    deathCause = DEATH_BULLET;
    deathTimer = 1.20f;
    titleBlink = 0.25f;
    state = ST_DYING;
    render();
    screenshot("shot_death_bullet.bmp");
    SDL_RenderPresent(ren);

    // 8d — доберман сбивает верхнюю часть Коляна
    deathCause = DEATH_DOBERMAN;
    deathTimer = 1.65f;
    render();
    screenshot("shot_death_dog.bmp");
    SDL_RenderPresent(ren);

    // 9 — экран поражения
    resetWorld(0);
    state = ST_PLAY;
    kills = 7;
    lives = 0;
    deathCause = DEATH_BAT;
    deathTimer = 0;
    titleBlink = 0.25f;
    state = ST_GAMEOVER;
    render();
    screenshot("shot_gameover.bmp");
    SDL_RenderPresent(ren);
}

} // namespace kolek
