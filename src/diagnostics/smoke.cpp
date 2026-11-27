#include "game/game.h"
#include "core/math.h"
#include "core/physics.h"
#include "data/levels.h"

namespace kolek {

void Game::runSmoke()
{
    resetWorld(0);
    state = ST_PLAY;
    monsters.clear();

    bool startFirstFloor = std::fabs((player.y + player.h) - 32 * 16.0f) < 1.0f;
    bool doorBasement = (door.w > 0 && door.y >= 39 * 16.0f - 0.1f &&
                         door.y + door.h >= 42 * 16.0f - 0.1f &&
                         door.x > 150 * 16.0f);
    SDL_Log("start: feet=%.1f firstfloor=%d; door: x=%.0f y=%.0f basement=%d",
            player.y + player.h, startFirstFloor ? 1 : 0, door.x, door.y,
            doorBasement ? 1 : 0);

    // 1. Обычный бег вправо с прыжками (проверка физики и столкновений).
    for (int i = 0; i < 600; ++i) {
        Input in;
        in.right = i < 520;
        in.spacePressed = (i % 70 == 20);
        in.jumpHeld = (i % 70 >= 20 && i % 70 < 34);
        update(1.0f / 120.0f, in);
    }
    SDL_Log("run: x=%.1f y=%.1f cups=%d", player.x, player.y, cupsGot);

    // 2. Подъём по ступеням-зигзагу из подвала до верхней галереи.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 48 * 16.0f + 3.0f;
    player.y = 42 * 16.0f - PLAYER_H;
    player.facing = 1;
    snapCamera();

    bool climbed = false;
    for (int i = 0; i < 4000; ++i) {
        Input in;
        in.jumpHeld = true;
        if (player.grounded && player.jumpBuf <= 0.0f) in.spacePressed = true;
        update(1.0f / 120.0f, in);
        if (player.y + player.h <= 130.0f) {
            climbed = true;
            break;
        }
    }
    bool camOk = (camY < 80.0f);
    SDL_Log("stairs: y=%.1f climbed=%d camY=%.1f", player.y, climbed ? 1 : 0, camY);

    // 2a. Ступень держит: со стрелкой вниз Колян не проваливается сквозь неё.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 48 * 16.0f + 3.0f;
    player.y = 39 * 16.0f - PLAYER_H;
    player.facing = 1;
    player.grounded = true;
    snapCamera();
    float startFeet = player.y + player.h;
    for (int i = 0; i < 150; ++i) {
        Input in;
        in.down = true;
        in.lookDown = true;
        update(1.0f / 120.0f, in);
    }
    bool stepHeld = (player.grounded &&
                     std::fabs((player.y + player.h) - startFeet) < 1.5f);
    SDL_Log("stephold: feet=%.1f stayed=%d", player.y + player.h,
            stepHeld ? 1 : 0);

    // 2a2. Зигзаг ступеней: шагнув с верхней ступени, Колян падает
    // на следующую нижнюю, а не сквозь неё.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 1474.0f;          // над ступенью ряда 21, мимо ряда 19
    player.y = 19 * 16.0f - PLAYER_H;
    player.facing = 1;
    snapCamera();
    float fallFrom = player.y + player.h;
    for (int i = 0; i < 120; ++i) update(1.0f / 120.0f, Input{});
    bool stairFall = (player.grounded &&
                      std::fabs((player.y + player.h) - 21 * 16.0f) < 1.5f);
    SDL_Log("stairfall: from=%.0f to=%.0f caught=%d", fallFrom,
            player.y + player.h, stairFall ? 1 : 0);

    // 2a3. Паук ползёт по потолку и спускается на паутине над Коляном.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 5 * 16.0f + 3.0f;
    player.y = 482.0f;
    player.facing = 1;
    player.grounded = true;
    player.invuln = 99.0f;
    monsters.clear();
    {
        Monster m;
        m.type = 0;
        m.b.w = 16;
        m.b.h = 18;
        m.b.x = 80.0f;
        m.b.y = 420.0f;          // потолок стартового зала (ряд 25)
        m.ceilingY = m.b.y;
        m.b.facing = 1;
        m.hp = m.maxHp = 3;
        monsters.push_back(m);
    }
    float spiderLow = 420.0f;
    bool spiderWebbed = false;
    player.invuln = 0.0f;
    for (int i = 0; i < 6; ++i) {
        update(1.0f / 120.0f, Input{});
        if (!monsters.empty() && monsters.front().webState != 0)
            spiderWebbed = true;
    }
    // Отбегаем: паук должен спуститься, повисеть и вернуться на потолок.
    player.x += 150.0f;
    player.invuln = 99.0f;
    for (int i = 0; i < 300; ++i) {
        update(1.0f / 120.0f, Input{});
        if (!monsters.empty()) {
            if (monsters.front().webState != 0) spiderWebbed = true;
            spiderLow = std::max(spiderLow, monsters.front().b.y);
        }
    }
    bool spiderBack = (!monsters.empty() && monsters.front().webState == 0 &&
                       monsters.front().b.y <= 421.0f);
    bool spiderOk = spiderWebbed && spiderLow > 460.0f && spiderBack;
    SDL_Log("spider: webbed=%d low=%.0f back=%d ok=%d", spiderWebbed ? 1 : 0,
            spiderLow, spiderBack ? 1 : 0, spiderOk ? 1 : 0);
    monsters.clear();

    // 2a4. Паук ползёт по потолку, пока Колян далеко и паутину не тянет.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 5 * 16.0f + 3.0f;
    player.y = 482.0f;
    player.grounded = true;
    player.invuln = 99.0f;
    monsters.clear();
    {
        Monster m;
        m.type = 0;
        m.b.w = 16;
        m.b.h = 18;
        m.b.x = 100 * 16.0f;
        m.b.y = 420.0f;          // потолок нижней галереи (ряд 25)
        m.ceilingY = m.b.y;
        m.b.facing = 1;
        m.hp = m.maxHp = 3;
        monsters.push_back(m);
    }
    float crawlFrom = monsters.front().b.x;
    for (int i = 0; i < 120; ++i) update(1.0f / 120.0f, Input{});
    float crawlTo = monsters.empty() ? crawlFrom : monsters.front().b.x;
    bool spiderCrawl = (!monsters.empty() && monsters.front().webState == 0 &&
                        std::fabs(crawlTo - crawlFrom) > 20.0f &&
                        std::fabs(monsters.front().b.y - 420.0f) < 0.5f);
    SDL_Log("spidercrawl: from=%.0f to=%.0f crawl=%d", crawlFrom, crawlTo,
            spiderCrawl ? 1 : 0);
    monsters.clear();

    // 2b. Браток с пистолетом: ускоренная пуля долетает и убивает,
    //     но остаётся медленнее Коляна.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 5 * 16.0f + 3.0f;
    player.y = 482.0f;
    player.facing = 1;
    player.grounded = true;
    player.invuln = 0;
    snapCamera();
    monsters.clear();
    bullets.clear();
    {
        Monster m;
        m.type = 2;
        m.b.w = 14;
        m.b.h = 26;
        m.b.x = player.x + 90.0f;
        m.b.y = 482.0f + 4.0f;
        m.b.facing = -1;
        m.hp = m.maxHp = 3;
        m.homeX = m.b.x;
        m.homeY = m.b.y;
        m.throwCd = 0.0f;
        monsters.push_back(m);
    }
    bool bulletThrown = false;
    float bulletVx = 0.0f;
    for (int i = 0; i < 40 && !bulletThrown; ++i) {
        update(1.0f / 120.0f, Input{});
        if (!bullets.empty()) {
            bulletThrown = true;
            bulletVx = bullets.front().vx;
        }
    }
    bool bulletSlowerThanPlayer = (BULLET_SPEED < RUN_SPEED);
    bool bulletSpeedOk = bulletThrown &&
                         std::fabs(std::fabs(bulletVx) - BULLET_SPEED) < 0.01f &&
                         bulletSlowerThanPlayer;
    for (int i = 0; i < 700 && state == ST_PLAY; ++i)
        update(1.0f / 120.0f, Input{});
    bool bulletKill = (state == ST_DYING && deathCause == DEATH_BULLET);
    SDL_Log("bullet: thrown=%d speed=%.2f/%.2f slower=%d kill=%d cause=%d",
            bulletThrown ? 1 : 0, std::fabs(bulletVx), BULLET_SPEED,
            bulletSlowerThanPlayer ? 1 : 0, bulletKill ? 1 : 0, deathCause);
    state = ST_PLAY;

    // 2b2. Браток с пистолетом не разворачивается к Коляну на одной линии
    //      и стреляет не чаще одного раза в две секунды.
    monsters.clear();
    bullets.clear();
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 96 * 16.0f - 60.0f;
    player.y = 482.0f;
    player.facing = 1;
    player.grounded = true;
    player.invuln = 99.0f;
    snapCamera();
    {
        Monster m;
        m.type = 2;
        m.b.w = 14;
        m.b.h = 26;
        m.b.x = 96 * 16.0f;
        m.b.y = 482.0f + 4.0f;
        m.b.facing = 1;
        m.hp = m.maxHp = 3;
        m.homeX = m.b.x;
        m.homeY = m.b.y;
        m.throwCd = 0.0f;
        monsters.push_back(m);
    }
    // Колян стоит позади стрелка на той же высоте: тот не должен
    // развернуться в его сторону.
    for (int i = 0; i < 90; ++i) update(1.0f / 120.0f, Input{});
    bool pistolNoTurn = !monsters.empty() && monsters.front().b.facing == 1;
    SDL_Log("pistolface: facing=%d ok=%d",
            monsters.empty() ? 0 : monsters.front().b.facing,
            pistolNoTurn ? 1 : 0);

    // Теперь Колян перед стрелком: замеряем паузу между выстрелами.
    // Колян стоит низко и неуязвим для пули, иначе смерть прервёт замер.
    bullets.clear();
    if (!monsters.empty()) {
        player.x = monsters.front().b.x + 60.0f;
        player.y = 510.0f;
        player.h = 2.0f;
        player.grounded = true;
        player.invuln = 0.0f;
        monsters.front().throwCd = 0.0f;
    }
    update(1.0f / 120.0f, Input{});
    bool firstShot = !bullets.empty();
    float firstCd = monsters.empty() ? 0.0f : monsters.front().throwCd;
    bullets.clear();
    bool earlyShot = false;
    float prevCd = monsters.empty() ? 0.0f : monsters.front().throwCd;
    for (int i = 0; i < 200; ++i) {   // 1.67 с — второй пули быть не должно
        update(1.0f / 120.0f, Input{});
        float cd = monsters.empty() ? 0.0f : monsters.front().throwCd;
        if (cd > prevCd + 0.5f) earlyShot = true;
        prevCd = cd;
    }
    bool onTimeShot = false;
    for (int i = 0; i < 100; ++i) {   // ещё 0.83 с — здесь выстрел уже обязан быть
        update(1.0f / 120.0f, Input{});
        float cd = monsters.empty() ? 0.0f : monsters.front().throwCd;
        if (cd > prevCd + 0.5f) onTimeShot = true;
        prevCd = cd;
    }
    bool pistolRateOk = firstShot && std::fabs(firstCd - 2.0f) < 0.02f &&
                        !earlyShot && onTimeShot;
    SDL_Log("pistolrate: first=%d cd=%.2f early=%d ontime=%d ok=%d",
            firstShot ? 1 : 0, firstCd, earlyShot ? 1 : 0,
            onTimeShot ? 1 : 0, pistolRateOk ? 1 : 0);

    // 2b3. Патруль братка с пистолетом — ровно на 10% медленнее пули.
    monsters.clear();
    bullets.clear();
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 5 * 16.0f + 3.0f;
    player.y = 482.0f;
    player.grounded = true;
    player.invuln = 99.0f;
    {
        Monster m;
        m.type = 2;
        m.b.w = 14;
        m.b.h = 26;
        m.b.x = 96 * 16.0f;
        m.b.y = 482.0f + 4.0f;
        m.b.facing = 1;
        m.hp = m.maxHp = 3;
        m.homeX = m.b.x;
        m.homeY = m.b.y;
        m.throwCd = 99.0f;
        monsters.push_back(m);
    }
    for (int i = 0; i < 30; ++i) update(1.0f / 120.0f, Input{});
    float patrolX1 = monsters.empty() ? 0.0f : monsters.front().b.x;
    for (int i = 0; i < 60; ++i) update(1.0f / 120.0f, Input{});
    float patrolX2 = monsters.empty() ? patrolX1 : monsters.front().b.x;
    float patrolSpeed = std::fabs(patrolX2 - patrolX1) * 2.0f;
    bool pistolPatrolOk = !monsters.empty() &&
                          std::fabs(patrolSpeed - PISTOL_THUG_SPEED) < 2.0f &&
                          patrolSpeed < BULLET_SPEED;
    SDL_Log("pistolpatrol: speed=%.2f want=%.2f ok=%d", patrolSpeed,
            PISTOL_THUG_SPEED, pistolPatrolOk ? 1 : 0);
    monsters.clear();
    bullets.clear();

    // 2c. Доберман: есть на втором уровне, хитбокс занижен.
    resetWorld(1);
    state = ST_PLAY;
    bool dogOnMap = false;
    bool dogLowHitbox = false;
    for (const Monster& m : monsters) {
        if (m.type == 3) {
            dogOnMap = true;
            dogLowHitbox = (m.b.h <= 16.0f && m.b.w >= 20.0f);
        }
    }
    SDL_Log("doberman: onmap=%d low=%d", dogOnMap ? 1 : 0, dogLowHitbox ? 1 : 0);

    // 2d. Доберман кусает — Колян погибает именно от собаки.
    monsters.clear();
    bullets.clear();
    lives = START_LIVES;
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 50 * 16.0f + 3.0f;
    player.y = 32 * 16.0f - PLAYER_H;
    player.facing = 1;
    player.grounded = true;
    player.invuln = 0;
    snapCamera();
    {
        Monster d;
        d.type = 3;
        d.b.w = 24;
        d.b.h = 14;
        d.b.x = player.x + 40.0f;
        d.b.y = 32 * 16.0f - d.b.h;
        d.b.facing = -1;
        d.hp = d.maxHp = 2;
        d.homeX = d.b.x;
        d.homeY = d.b.y;
        monsters.push_back(d);
    }
    for (int i = 0; i < 300 && state == ST_PLAY; ++i)
        update(1.0f / 120.0f, Input{});
    bool dogKill = (state == ST_DYING && deathCause == DEATH_DOBERMAN);
    SDL_Log("dogkill: state=%d cause=%d ok=%d", state, deathCause,
            dogKill ? 1 : 0);

    resetWorld(0);
    state = ST_PLAY;
    monsters.clear();
    bullets.clear();
    lives = START_LIVES;

    // 3. Подбор кубка на верхней галерее: +1000 очков.
    monsters.clear();
    bullets.clear();
    int beforeCups = cupsGot;
    int beforeScore = score;
    player.x = 10 * 16.0f + 2.0f;
    player.y = 98.0f;
    player.vx = player.vy = 0;
    player.grounded = true;
    for (int i = 0; i < 12; ++i) {
        Input in;
        update(1.0f / 120.0f, in);
    }
    bool gotCup = (cupsGot == beforeCups + 1 &&
                   score == beforeScore + CUP_SCORE);
    SDL_Log("cup: got=%d total=%d score=%d", gotCup ? 1 : 0, cupsGot, score);

    // 3a. Чекпоинт — последний взятый кубок; без кубков — старт уровня.
    resetWorld(0);
    state = ST_PLAY;
    monsters.clear();
    lives = START_LIVES;
    bool startCheckpoint = (std::fabs(checkpoint.x - player.x) < 0.5f &&
                            std::fabs(checkpoint.y - player.y) < 0.5f);
    CupItem& firstCup = cups.front();
    int cupFloor = firstCup.ty + 1;
    while (cupFloor < lvl.h && !lvl.solid(firstCup.tx, cupFloor)) ++cupFloor;
    float cupPX = firstCup.tx * TILE + 3.0f;
    float cupPY = cupFloor * TILE - PLAYER_H;
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = cupPX;
    player.y = cupPY;
    player.facing = 1;
    player.grounded = true;
    checkPickups();
    bool cupCheckpoint = (cupsGot == 1 &&
                          std::fabs(checkpoint.x - cupPX) < 0.5f &&
                          std::fabs(checkpoint.y - cupPY) < 0.5f);
    player.invuln = 0;
    killPlayer(DEATH_SPIDER);
    deathTimer = DEATH_TIME;
    updateDeath(0.001f);
    bool cupRespawn = (state == ST_PLAY && lives == START_LIVES - 1 &&
                       std::fabs(player.x - cupPX) < 0.5f &&
                       std::fabs(player.y - cupPY) < 0.5f);
    bool cupCheckpointOk = startCheckpoint && cupCheckpoint && cupRespawn;
    SDL_Log("cupcheckpoint: start=%d set=%d respawn=%d ok=%d",
            startCheckpoint ? 1 : 0, cupCheckpoint ? 1 : 0,
            cupRespawn ? 1 : 0, cupCheckpointOk ? 1 : 0);

    // 3b. Очки за убийство: браток с битой 100, паук 200,
    //     браток с пистолетом 300, доберман 250.
    score = 0;
    size_t popupBefore = popups.size();
    {
        Monster z;
        z.type = 1;
        z.b.x = 200.0f;
        z.b.y = 400.0f;
        z.hp = 1;
        damageMonster(z, 5, 1.0f);
        Monster sp;
        sp.type = 0;
        sp.b.x = 220.0f;
        sp.b.y = 400.0f;
        sp.hp = 1;
        damageMonster(sp, 5, 1.0f);
        Monster th;
        th.type = 2;
        th.b.x = 240.0f;
        th.b.y = 400.0f;
        th.hp = 1;
        damageMonster(th, 5, 1.0f);
        Monster dg;
        dg.type = 3;
        dg.b.x = 260.0f;
        dg.b.y = 400.0f;
        dg.hp = 1;
        damageMonster(dg, 5, 1.0f);
    }
    bool killScore = (score == 100 + 200 + 300 + 250);
    bool popupOk = (popups.size() == popupBefore + 4 &&
                    popups[popupBefore].value == 100 &&
                    popups[popupBefore + 1].value == 200 &&
                    popups[popupBefore + 2].value == 300 &&
                    popups[popupBefore + 3].value == 250);
    SDL_Log("killscore: total=%d popups=%d ok=%d", score,
            static_cast<int>(popups.size()), (killScore && popupOk) ? 1 : 0);

    // 3c. Украшения: 200 / 400 / 600 очков.
    score = 0;
    jewelry.clear();
    for (int t = 0; t < 3; ++t) {
        Jewel jw;
        jw.x = player.x;
        jw.y = player.y;
        jw.type = t;
        jewelry.push_back(jw);
    }
    checkPickups();
    bool jewelScore = (score == 200 + 400 + 600 && jewelry.empty());
    SDL_Log("jewel: score=%d left=%d ok=%d", score,
            static_cast<int>(jewelry.size()), jewelScore ? 1 : 0);

    // 3d. Всплывающие очки поднимаются и вовремя исчезают.
    size_t popupCount = popups.size();
    float popupY = popups.empty() ? 0.0f : popups.back().y;
    for (int i = 0; i < 10; ++i) updateScorePopups(1.0f / 120.0f);
    bool popupRise = (popups.size() == popupCount && !popups.empty() &&
                      popups.back().y < popupY - 1.0f);
    for (int i = 0; i < 200; ++i) updateScorePopups(1.0f / 120.0f);
    bool popupGone = popups.empty();
    SDL_Log("popup: rise=%d gone=%d", popupRise ? 1 : 0, popupGone ? 1 : 0);

    // 4. Двойной прыжок: ровно один прыжок на нажатие, максимум два.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 3 * 16.0f + 3.0f;
    player.y = 482.0f;
    player.facing = 1;
    for (int i = 0; i < 6; ++i) update(1.0f / 120.0f, Input{});
    bool oneJumpOnly = false;
    {
        Input in;
        in.spacePressed = true;
        in.jumpHeld = true;
        update(1.0f / 120.0f, in);
        oneJumpOnly = (player.airJumps == 1 && player.vy < 0.0f);
        // Повтор того же Input в следующем подшаге не должен тратить второй прыжок.
        update(1.0f / 120.0f, in);
        oneJumpOnly = oneJumpOnly && (player.airJumps == 1);
        if (!oneJumpOnly) SDL_Log("doublejump: duplicate edge consumed a jump");
    }
    for (int i = 0; i < 14; ++i) {
        Input in;
        in.jumpHeld = true;
        update(1.0f / 120.0f, in);
    }
    {
        Input in;
        in.spacePressed = true;
        in.jumpHeld = true;
        update(1.0f / 120.0f, in);
    }
    bool doubleJump = (player.airJumps == 0 && !player.grounded);
    SDL_Log("doublejump: edge=%d used=%d y=%.1f", oneJumpOnly ? 1 : 0,
            doubleJump ? 1 : 0, player.y);
    for (int i = 0; i < 160; ++i) update(1.0f / 120.0f, Input{});
    bool jumpReset = (player.grounded && player.airJumps == 1);
    SDL_Log("doublejump: reset=%d grounded=%d airJumps=%d", jumpReset ? 1 : 0,
            player.grounded ? 1 : 0, player.airJumps);

    // 4b. Удержание W не должно съедать отдельное нажатие пробела.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 3 * 16.0f + 3.0f;
    player.y = 482.0f;
    player.facing = 1;
    for (int i = 0; i < 6; ++i) update(1.0f / 120.0f, Input{});
    {
        Input in;
        in.up = true;
        in.spacePressed = true;
        in.jumpHeld = true;
        update(1.0f / 120.0f, in);
    }
    {
        Input in;
        in.up = true;
        in.jumpHeld = true;
        update(1.0f / 120.0f, in);
    }
    {
        Input in;
        in.up = true;
        in.spacePressed = true;
        in.jumpHeld = true;
        update(1.0f / 120.0f, in);
    }
    bool spaceWhileHoldingW = (player.airJumps == 0 && !player.grounded);
    SDL_Log("doublejump: space_with_w=%d airJumps=%d",
            spaceWhileHoldingW ? 1 : 0, player.airJumps);

    // 5. Мгновенный выстрел дробовика по братку с битой.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 5 * 16.0f + 3.0f;
    player.y = 482.0f;
    player.facing = 1;
    player.grounded = true;
    player.invuln = 2.0f;
    snapCamera();
    monsters.clear();
    mag = MAG_SIZE;
    shootCd = 0;
    reloading = false;
    {
        Monster m;
        m.type = 1;
        m.b.w = 14;
        m.b.h = 26;
        m.b.x = player.x + 45.0f;
        m.b.y = player.y + 4.0f;
        m.b.facing = -1;
        m.hp = m.maxHp = 20;
        m.homeX = m.b.x;
        m.homeY = m.b.y;
        monsters.push_back(m);
    }
    {
        Input in;
        in.shootPressed = true;
        update(1.0f / 120.0f, in);
    }
    int hpAfter = monsters.empty() ? 0 : monsters.front().hp;
    bool shotOk = (mag == MAG_SIZE - 1 && hpAfter < 20);
    bool tracerOk = (static_cast<int>(tracers.size()) == PELLET_COUNT);
    bool recoilOk = (player.vx <= -RECOIL_KICK * 0.7f);
    SDL_Log("shot: mag=%d monsterHp=%d instant=%d tracers=%d recoil=%d",
            mag, hpAfter, shotOk ? 1 : 0, static_cast<int>(tracers.size()),
            recoilOk ? 1 : 0);
    for (int i = 0; i < 40; ++i) update(1.0f / 120.0f, Input{});

    // 5b. Взгляд вверх/вниз стрелками вне лестницы.
    {
        Input in;
        in.lookUp = true;
        update(1.0f / 120.0f, in);
    }
    bool lookUpOk = (player.lookY == -1);
    {
        Input in;
        in.lookDown = true;
        update(1.0f / 120.0f, in);
    }
    bool lookDownOk = (player.lookY == 1);
    update(1.0f / 120.0f, Input{});
    bool lookForward = (player.lookY == 0);
    SDL_Log("look: up=%d down=%d forward=%d", lookUpOk ? 1 : 0,
            lookDownOk ? 1 : 0, lookForward ? 1 : 0);

    // 5c. Диагональный выстрел вверх: браток висит над лучом.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 5 * 16.0f + 3.0f;
    player.y = 482.0f;
    player.facing = 1;
    player.grounded = true;
    player.invuln = 2.0f;
    player.vx = 0;
    monsters.clear();
    mag = MAG_SIZE;
    shootCd = 0;
    reloading = false;
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
        monsters.push_back(m);
    }
    {
        Input in;
        in.lookUp = true;
        in.shootPressed = true;
        update(1.0f / 120.0f, in);
    }
    int hpUp = monsters.empty() ? 0 : monsters.front().hp;
    bool diagUpOk = (mag == MAG_SIZE - 1 && hpUp < 20);
    bool recoilUpOk = (std::fabs(player.vx) < 5.0f);
    SDL_Log("diag-up: mag=%d monsterHp=%d recoil=%d ok=%d", mag, hpUp,
            recoilUpOk ? 1 : 0, diagUpOk ? 1 : 0);

    // 5d. Диагональный выстрел вниз из главного зала.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 5 * 16.0f + 3.0f;
    player.y = 150.0f;
    player.facing = 1;
    player.grounded = false;
    player.invuln = 2.0f;
    monsters.clear();
    mag = MAG_SIZE;
    shootCd = 0;
    reloading = false;
    {
        Monster m;
        m.type = 1;
        m.b.w = 14;
        m.b.h = 26;
        m.b.x = player.x + 50.0f;
        m.b.y = player.y + 30.0f;
        m.b.facing = -1;
        m.hp = m.maxHp = 20;
        m.homeX = m.b.x;
        m.homeY = m.b.y;
        monsters.push_back(m);
    }
    {
        Input in;
        in.lookDown = true;
        update(1.0f / 120.0f, in);
    }
    player.vx = 0;
    fireShot(Input{});
    int hpDown = monsters.empty() ? 0 : monsters.front().hp;
    bool diagDownOk = (mag == MAG_SIZE - 1 && hpDown < 20);
    bool recoilDownOk = (std::fabs(player.vx) < 0.1f);
    SDL_Log("diag-down: mag=%d monsterHp=%d recoil=%d ok=%d", mag, hpDown,
            recoilDownOk ? 1 : 0, diagDownOk ? 1 : 0);

    // 5e. В прыжке стрелять нельзя.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 5 * 16.0f + 3.0f;
    player.y = 150.0f;
    player.facing = 1;
    player.grounded = false;
    player.invuln = 2.0f;
    monsters.clear();
    mag = MAG_SIZE;
    shootCd = 0;
    reloading = false;
    {
        Input in;
        in.shootPressed = true;
        update(1.0f / 120.0f, in);
    }
    bool noAirShot = (mag == MAG_SIZE);
    SDL_Log("airshot: mag=%d blocked=%d", mag, noAirShot ? 1 : 0);

    // 6. Перезарядка на месте.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 5 * 16.0f + 3.0f;
    player.y = 482.0f;
    player.facing = 1;
    player.grounded = true;
    player.invuln = 2.0f;
    monsters.clear();
    mag = 1;
    shootCd = 0;
    reloading = false;
    {
        Input in;
        in.shootPressed = true;
        update(1.0f / 120.0f, in);
    }
    // Первую секунду неподвижности перезарядка ещё не начинается.
    for (int i = 0; i < 100; ++i) update(1.0f / 120.0f, Input{});
    bool reloadWaitHeld = !reloading;
    for (int i = 0; i < 40; ++i) update(1.0f / 120.0f, Input{});
    bool reloadStarted = reloading;
    for (int i = 0; i < 300; ++i) update(1.0f / 120.0f, Input{});
    bool reloadOk = (mag == MAG_SIZE && !reloading &&
                     std::fabs(reloadDuration - RELOAD_FULL) < 0.01f);
    SDL_Log("reload: wait1s=%d started=%d mag=%d dur=%.2f ok=%d",
            reloadWaitHeld ? 1 : 0, reloadStarted ? 1 : 0, mag, reloadDuration,
            reloadOk ? 1 : 0);

    // 6b. Перезарядка пропорциональна недостающим патронам и блокирует движение.
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 5 * 16.0f + 3.0f;
    player.y = 482.0f;
    player.facing = 1;
    player.grounded = true;
    player.invuln = 2.0f;
    monsters.clear();
    mag = 4;
    shootCd = 0;
    reloading = false;
    reloadTimer = 0;
    reloadDuration = 0;
    standStill = RELOAD_WAIT;
    update(1.0f / 120.0f, Input{});
    bool reloadProportional = reloading &&
                              std::fabs(reloadDuration - 1.0f) < 0.01f &&
                              std::fabs(reloadTimer - 1.0f) < 0.02f;
    float lockX = player.x;
    for (int i = 0; i < 30; ++i) {
        Input in;
        in.right = true;
        update(1.0f / 120.0f, in);
    }
    bool reloadLocked = std::fabs(player.x - lockX) < 0.5f;
    for (int i = 0; i < 150; ++i) update(1.0f / 120.0f, Input{});
    bool reloadDone = (mag == MAG_SIZE && !reloading);
    SDL_Log("reload4: dur=%.2f proportional=%d locked=%d done=%d",
            reloadDuration, reloadProportional ? 1 : 0, reloadLocked ? 1 : 0,
            reloadDone ? 1 : 0);

    // 7. Смерть от братка с битой, три жизни и экран поражения.
    resetWorld(0);
    state = ST_PLAY;
    monsters.clear();
    lives = START_LIVES;
    checkpoint = RectF{player.x, player.y, player.w, player.h};
    player.invuln = 0;
    {
        Monster m;
        m.type = 1;
        m.b.w = 14;
        m.b.h = 26;
        m.b.x = player.x - 2.0f;
        m.b.y = player.y + 4.0f;
        m.b.facing = 1;
        m.hp = m.maxHp = 4;
        m.homeX = m.b.x;
        m.homeY = m.b.y;
        monsters.push_back(m);
    }
    update(1.0f / 120.0f, Input{});
    bool dying = (state == ST_DYING && lives == START_LIVES - 1 &&
                  deathCause == DEATH_BAT);
    SDL_Log("death: state=%d lives=%d cause=%d", state, lives, deathCause);

    monsters.clear();
    for (int i = 0; i < 400; ++i) update(1.0f / 120.0f, Input{});
    bool respawned = (state == ST_PLAY && lives == START_LIVES - 1);
    SDL_Log("respawn: state=%d lives=%d", state, lives);

    for (int round = 0; round < 2; ++round) {
        state = ST_PLAY;
        player.invuln = 0;
        killPlayer(DEATH_SPIDER);
        for (int i = 0; i < 400; ++i) update(1.0f / 120.0f, Input{});
    }
    bool gameOver = (state == ST_GAMEOVER && lives == 0);
    SDL_Log("gameover: state=%d lives=%d", state, lives);

    // 8. Переход на второй уровень: сначала два слайда, затем сам уровень.
    resetWorld(0);
    monsters.clear();
    for (CupItem& c : cups) {
        c.taken = true;
        lvl.rows[c.ty][c.tx] = '.';
    }
    cupsGot = static_cast<int>(cups.size());
    player.x = 2496.0f;
    player.y = 42 * 16.0f - PLAYER_H;   // дверь теперь в подвале, на полу row 42
    player.vx = player.vy = 0;
    player.grounded = true;
    player.facing = 1;
    for (int i = 0; i < 200; ++i) update(1.0f / 120.0f, Input{});
    bool slideShown = (state == ST_SLIDE && levelIndex == 0 &&
                       static_cast<int>(slideTexts.size()) == 2);

    // Первый слайд: подтверждение перелистывает ко второму.
    {
        Input in;
        in.confirm = true;
        update(1.0f / 120.0f, in);
    }
    bool slideSecond = (state == ST_SLIDE && slideIndex == 1);

    // Второй слайд: подтверждение загружает второй уровень.
    {
        Input in;
        in.confirm = true;
        update(1.0f / 120.0f, in);
    }
    bool level2 = (levelIndex == 1 && state == ST_PLAY);
    bool slidesOk = slideShown && slideSecond && level2;
    SDL_Log("slides: shown=%d second=%d level=%d state=%d ok=%d",
            slideShown ? 1 : 0, slideSecond ? 1 : 0, levelIndex, state,
            slidesOk ? 1 : 0);

    // Технический этаж: аварийные лампы и стремянки на своих местах.
    int techLamps = 0, techLadders = 0;
    for (const Decor& d : decor) {
        if (d.type == DEC_EMERGENCY) ++techLamps;
        if (d.type == DEC_STEPLADDER) ++techLadders;
    }
    bool techDecorOk = (techLamps >= 4 && techLadders >= 2);
    SDL_Log("techdecor: lamps=%d ladders=%d ok=%d", techLamps, techLadders,
            techDecorOk ? 1 : 0);

    if (level2) {
        monsters.clear();
        for (CupItem& c : cups) {
            c.taken = true;
            lvl.rows[c.ty][c.tx] = '.';
        }
        cupsGot = static_cast<int>(cups.size());
        player.x = 2496.0f;
        player.y = 482.0f;
        player.vx = player.vy = 0;
        player.grounded = true;
        player.facing = 1;
        for (int i = 0; i < 200; ++i) update(1.0f / 120.0f, Input{});
    }

    // Слайд после технического этажа: стена с надписью «ЛЁХА БЫЛ ЗДЕСЬ».
    bool wallSlide = (state == ST_SLIDE && levelIndex == 1 && slideWallScene &&
                      static_cast<int>(slideTexts.size()) == 1);
    if (wallSlide) {
        Input in;
        in.confirm = true;
        update(1.0f / 120.0f, in);
    }
    SDL_Log("slide3: shown=%d wall=%d level=%d state=%d ok=%d",
            wallSlide ? 1 : 0, wallSlide ? 1 : 0, levelIndex, state,
            (wallSlide && levelIndex == 2 && state == ST_PLAY) ? 1 : 0);

    // Второй лифт ведёт в длинный одноэтажный вестибюль.
    bool level3 = (levelIndex == 2 && state == ST_PLAY);
    int bats3 = 0, dogs3 = 0, pistols3 = 0, pistolsNearDoor = 0;
    for (const Monster& m : monsters) {
        if (m.type == 1) ++bats3;
        if (m.type == 3) ++dogs3;
        if (m.type == 2) {
            ++pistols3;
            if (m.b.x > door.x - 200.0f) ++pistolsNearDoor;
        }
    }
    int highPlats = 0, pitTiles = 0, highCups = 0, highJewels = 0;
    for (int ty = 0; ty < lvl.h; ++ty)
        for (int tx = 0; tx < lvl.w; ++tx) {
            char c = lvl.at(tx, ty);
            if (c == 'S' && ty < lvl.h - 6) ++highPlats;
            if (c == '.' && ty == lvl.h - 6) ++pitTiles;
        }
    for (const CupItem& c : cups)
        if (c.ty <= lvl.h - 8) ++highCups;
    for (const Jewel& jw : jewelry)
        if (jw.y < (lvl.h - 7) * TILE) ++highJewels;
    bool singleFloor = (lvl.h == 22);
    bool lobbyOk = singleFloor && lvl.w >= 220 && highPlats >= 12 &&
                   pitTiles >= 12 && bats3 >= 3 && dogs3 >= 2 &&
                   pistols3 == 1 && pistolsNearDoor == 1;
    bool lobbyBonuses = (highCups >= 2 && highJewels >= 2);
    SDL_Log("lobby: h=%d w=%d plats=%d pits=%d bat=%d dog=%d gun=%d/%d "
            "highcup=%d highjewel=%d ok=%d",
            lvl.h, lvl.w, highPlats, pitTiles, bats3, dogs3, pistols3,
            pistolsNearDoor, highCups, highJewels,
            (lobbyOk && lobbyBonuses) ? 1 : 0);

    // Пропасть: падение отнимает жизнь и возвращает на чекпоинт.
    bool fallDies = false, fallRespawns = false;
    if (level3) {
        lives = START_LIVES;
        int livesBefore = lives;
        player.x = 50 * 16.0f + 3.0f;
        player.y = (lvl.h - 8) * 16.0f;
        player.vx = player.vy = 0;
        player.grounded = false;
        player.invuln = 0;
        for (int i = 0; i < 120; ++i) update(1.0f / 120.0f, Input{});
        fallDies = (state == ST_DYING && deathCause == DEATH_FALL &&
                    lives == livesBefore - 1);
        for (int i = 0; i < 400; ++i) update(1.0f / 120.0f, Input{});
        fallRespawns = (state == ST_PLAY &&
                        std::fabs(player.x - checkpoint.x) < 1.0f);
        SDL_Log("fall: dies=%d respawn=%d cause=%d lives=%d ok=%d",
                fallDies ? 1 : 0, fallRespawns ? 1 : 0, deathCause, lives,
                (fallDies && fallRespawns) ? 1 : 0);
    }

    // Кубки вестибюля собраны — дверь на другом конце зала открывается
    // и ведёт на ночную парковку с боссом.
    if (level3) {
        monsters.clear();
        for (CupItem& c : cups) {
            c.taken = true;
            lvl.rows[c.ty][c.tx] = '.';
        }
        cupsGot = static_cast<int>(cups.size());
        player.x = door.x + 8.0f;
        player.y = door.y + door.h - PLAYER_H;
        player.vx = player.vy = 0;
        player.grounded = true;
        player.facing = 1;
        player.invuln = 2.0f;
        for (int i = 0; i < 200; ++i) update(1.0f / 120.0f, Input{});
    }
    bool level4 = (levelIndex == 3 && state == ST_PLAY);
    bool parkingSize = (lvl.w == 40 && lvl.h == 19);
    bool parkingBare = (cups.empty() && door.w <= 0.0f);
    int bossCount = 0;
    bool bossSize = false, bossHpOk = false, bossNearJeep = false;
    bool bossStandsOnFloor = false, bossFacesPlayer = false;
    for (const Monster& m : monsters) {
        if (m.type != 4) continue;
        ++bossCount;
        bossSize = (std::fabs(m.b.w - BOSS_W) < 0.1f &&
                    std::fabs(m.b.h - BOSS_H) < 0.1f);
        bossHpOk = (m.hp == BOSS_HP && m.maxHp == BOSS_HP);
        // Джип нарисован примерно на x=396..474, босс стоит вплотную слева.
        bossNearJeep = (m.b.x >= 280.0f && m.b.x + m.b.w <= 400.0f);
        bossStandsOnFloor = (std::fabs((m.b.y + m.b.h) - 17 * TILE) < 1.0f);
        bossFacesPlayer = (m.b.facing == -1);
    }
    int parkingPanels = 0;
    for (int ty = 0; ty < lvl.h; ++ty)
        for (int tx = 0; tx < lvl.w; ++tx)
            if (lvl.at(tx, ty) == 't' && ty >= lvl.h - 4) ++parkingPanels;
    bool bossOnMap = (level4 && parkingSize && parkingBare && bossCount == 1 &&
                      bossSize && bossHpOk && bossNearJeep &&
                      bossStandsOnFloor && bossFacesPlayer &&
                      parkingPanels == 8);
    SDL_Log("parking: level=%d size=%dx%d boss=%d %dx%d hp=%d/%d nearjeep=%d "
            "floor=%d face=%d panels=%d ok=%d",
            levelIndex, lvl.w, lvl.h, bossCount,
            bossCount ? static_cast<int>(monsters.back().b.w) : 0,
            bossCount ? static_cast<int>(monsters.back().b.h) : 0,
            bossCount ? monsters.back().hp : 0,
            bossCount ? monsters.back().maxHp : 0,
            bossNearJeep ? 1 : 0, bossStandsOnFloor ? 1 : 0,
            bossFacesPlayer ? 1 : 0, parkingPanels, bossOnMap ? 1 : 0);

    // Босс упирается в бетонный блок и в правую стену — и разворачивается.
    bool bossPanelTurn = false, bossWallTurn = false;
    int bossIdx = -1;
    for (size_t i = 0; i < monsters.size(); ++i)
        if (monsters[i].type == 4) { bossIdx = static_cast<int>(i); break; }
    if (bossIdx >= 0) {
        player.x = 4 * 16.0f + 3.0f;
        player.y = 17 * 16.0f - PLAYER_H;
        player.vx = player.vy = 0;
        player.grounded = true;
        player.invuln = 5.0f;
        Monster& bm = monsters[bossIdx];
        bm.b.x = 242.0f;             // вплотную к правому краю блока (240)
        bm.b.y = 17 * 16.0f - BOSS_H;
        bm.b.vx = -26.0f;
        bm.b.facing = -1;
        for (int i = 0; i < 90; ++i) updateMonsters(1.0f / 120.0f);
        bossPanelTurn = (bm.b.facing == 1 && bm.b.x >= 239.0f);

        bm.b.x = 520.0f;             // недалеко от правой стены (624)
        bm.b.y = 17 * 16.0f - BOSS_H;
        bm.b.vx = 26.0f;
        bm.b.facing = 1;
        for (int i = 0; i < 120; ++i) updateMonsters(1.0f / 120.0f);
        bossWallTurn = (bm.b.facing == -1 && bm.b.x <= 545.0f);
    }
    SDL_Log("bossturn: panel=%d wall=%d x=%.0f",
            bossPanelTurn ? 1 : 0, bossWallTurn ? 1 : 0,
            bossIdx >= 0 ? monsters[bossIdx].b.x : -1.0f);

    // Гигант убивает касанием: отдельная причина смерти.
    resetWorld(3);
    state = ST_PLAY;
    lives = START_LIVES;
    player.x = 19 * 16.0f + 4.0f;
    player.y = 17 * 16.0f - PLAYER_H;
    player.vx = player.vy = 0;
    player.grounded = true;
    player.invuln = 0;
    update(1.0f / 120.0f, Input{});
    bool bossContact = (state == ST_DYING && deathCause == DEATH_BOSS &&
                        lives == START_LIVES - 1);
    SDL_Log("bosscontact: state=%d cause=%d lives=%d ok=%d",
            state, deathCause, lives, bossContact ? 1 : 0);

    // 70 дробин = 10 залпов по 7; 69-я ещё не убивает.
    resetWorld(3);
    state = ST_PLAY;
    bool bossPellets = false, bossVictory = false;
    int bossIdx2 = -1;
    for (size_t i = 0; i < monsters.size(); ++i)
        if (monsters[i].type == 4) { bossIdx2 = static_cast<int>(i); break; }
    bool bossMath = (BOSS_HP == PELLET_COUNT * 10);
    if (bossIdx2 >= 0) {
        Monster& bm = monsters[bossIdx2];
        int scoreBefore = score;
        for (int i = 0; i < BOSS_HP - 1; ++i) damageMonster(bm, 1, 1.0f);
        bool aliveAtOne = (!bm.dead && bm.hp == 1);
        damageMonster(bm, 1, 1.0f);
        bossPellets = (aliveAtOne && bm.dead && bossDefeated &&
                       score == scoreBefore + BOSS_SCORE);
        for (int i = 0; i < 240; ++i) update(1.0f / 120.0f, Input{});
        bossVictory = (state == ST_WIN);
    }
    SDL_Log("bossfight: math=%d pellets=%d victory=%d state=%d kills=%d",
            bossMath ? 1 : 0, bossPellets ? 1 : 0, bossVictory ? 1 : 0,
            state, kills);

    // 9. Лестницы второго уровня — тот же зигзаг из двух тайлов, что и в первом.
    bool level2Stairs = false;
    if (level2) {
        resetWorld(1);
        state = ST_PLAY;
        monsters.clear();
        player = Body{};
        player.w = 10;
        player.h = PLAYER_H;
        player.x = 40 * 16.0f + 3.0f;          // в полосе перекрытия ступеней
        player.y = 32 * 16.0f - PLAYER_H;      // нижний этаж
        player.facing = 1;
        snapCamera();
        bool reached = false;
        for (int i = 0; i < 6000; ++i) {
            Input in;
            in.jumpHeld = true;
            if (player.grounded && player.jumpBuf <= 0.0f) in.spacePressed = true;
            update(1.0f / 120.0f, in);
            if (player.grounded && player.y + player.h <= 113.0f) {
                reached = true;
                break;
            }
        }
        if (reached) {
            // Отпускаем прыжок: Колян должен остаться стоять на верхней ступени.
            for (int i = 0; i < 120; ++i) update(1.0f / 120.0f, Input{});
            level2Stairs = (player.grounded && player.y + player.h <= 114.0f);
        }
        SDL_Log("stairs2: x=%.1f feet=%.1f climbed=%d", player.x,
                player.y + player.h, level2Stairs ? 1 : 0);
    }

    // 10. Главное меню, настройки и переназначение клавиш.
    resetWorld(0);
    state = ST_TITLE;
    menuIndex = 0;
    controls.reset();
    {
        Input in;
        in.confirm = true;
        update(1.0f / 120.0f, in);
    }
    bool menuStart = (state == ST_PLAY);

    state = ST_TITLE;
    menuIndex = 1;
    {
        Input in;
        in.confirm = true;
        update(1.0f / 120.0f, in);
    }
    bool menuSettings = (state == ST_SETTINGS && settingsIndex == 0);

    {
        Input in;
        in.confirm = true;
        update(1.0f / 120.0f, in);
    }
    bool bindWaiting = (bindTarget == CA_LEFT && bindSlot == 0);
    assignBind(SDL_SCANCODE_Q);
    bool menuRebind = (controls.keys[CA_LEFT][0] == SDL_SCANCODE_Q &&
                       bindTarget < 0);

    // Вторая клавиша того же действия.
    settingsSlot = 1;
    {
        Input in;
        in.confirm = true;
        update(1.0f / 120.0f, in);
    }
    bool bindWaiting2 = (bindTarget == CA_LEFT && bindSlot == 1);
    assignBind(SDL_SCANCODE_E);
    bool menuRebind2 = (controls.keys[CA_LEFT][1] == SDL_SCANCODE_E);

    {
        Input in;
        in.back = true;
        update(1.0f / 120.0f, in);
    }
    bool settingsBack = (state == ST_TITLE && menuIndex == 1);

    state = ST_PLAY;
    const float beforeExitX = player.x;
    const float beforeExitY = player.y;
    const float beforeExitTime = timeSec;
    {
        Input in;
        in.back = true;
        update(1.0f / 120.0f, in);
    }
    bool exitPrompt = (state == ST_PLAY && exitAction == EXIT_LEVEL && !exitAccepted);
    {
        Input in;
        in.right = in.shootPressed = in.restart = true;
        update(1.0f, in);
    }
    bool exitPaused = (state == ST_PLAY && player.x == beforeExitX &&
                       player.y == beforeExitY && timeSec == beforeExitTime + 1.0f / 120.0f);
    {
        Input in;
        in.confirm = true; // выбранное по умолчанию «Нет»
        update(1.0f / 120.0f, in);
    }
    bool exitCancelled = (state == ST_PLAY && exitAction == EXIT_NONE);
    requestExit(EXIT_LEVEL);
    {
        Input in;
        in.back = true;
        update(1.0f / 120.0f, in);
    }
    bool exitBack = (state == ST_PLAY && exitAction == EXIT_NONE);
    requestExit(EXIT_LEVEL);
    {
        Input in;
        in.menuDown = true;
        update(1.0f / 120.0f, in);
        in = Input{};
        in.confirm = true;
        update(1.0f / 120.0f, in);
    }
    bool escMenu = (state == ST_TITLE && exitAction == EXIT_NONE);

    // Esc в главном меню открывает тот же диалог, что и «Выход».
    {
        Input in;
        in.back = true;
        update(1.0f / 120.0f, in);
    }
    bool escQuitPrompt = (state == ST_TITLE && !quitRequested &&
                          exitAction == EXIT_GAME && !exitAccepted);
    {
        Input in;
        in.back = true;
        update(1.0f / 120.0f, in);
    }
    bool escQuitCancel = (state == ST_TITLE && !quitRequested && exitAction == EXIT_NONE);

    // «Выход» открывает подтверждение, отмена оставляет игру открытой.
    menuIndex = 2;
    {
        Input in;
        in.confirm = true;
        update(1.0f / 120.0f, in);
    }
    bool quitPrompt = (!quitRequested && exitAction == EXIT_GAME && !exitAccepted);
    {
        Input in;
        in.confirm = true;
        update(1.0f / 120.0f, in);
    }
    bool quitCancelled = (!quitRequested && exitAction == EXIT_NONE && state == ST_TITLE);
    // Тот же запрос используется для SDL_QUIT (крестик окна, Alt+F4).
    requestExit(EXIT_GAME);
    {
        Input in;
        in.back = true;
        update(1.0f / 120.0f, in);
    }
    bool quitBack = (!quitRequested && exitAction == EXIT_NONE);
    requestExit(EXIT_GAME);
    {
        Input in;
        in.menuDown = true;
        update(1.0f / 120.0f, in);
        requestExit(EXIT_GAME); // повторный запрос не сбрасывает выбор
        in = Input{};
        in.confirm = true;
        update(1.0f / 120.0f, in);
    }
    bool exitQuits = quitRequested && exitAction == EXIT_NONE;
    quitRequested = false;
    menuIndex = 0;
    controls.reset();

    // Прицел следует за направлением движения: вверх/вниз теми же клавишами.
    resetWorld(0);
    state = ST_PLAY;
    monsters.clear();
    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = 5 * 16.0f + 3.0f;
    player.y = 482.0f;
    player.facing = 1;
    player.grounded = true;
    {
        Input in;
        in.up = true;
        update(1.0f / 120.0f, in);
    }
    bool moveAimUp = (player.lookY == -1);
    {
        Input in;
        in.down = true;
        update(1.0f / 120.0f, in);
    }
    bool moveAimDown = (player.lookY == 1);
    update(1.0f / 120.0f, Input{});
    bool moveAimFwd = (player.lookY == 0);
    bool moveAimOk = moveAimUp && moveAimDown && moveAimFwd;

    bool menuOk = menuStart && menuSettings && bindWaiting && menuRebind &&
                  bindWaiting2 && menuRebind2 && settingsBack && escMenu &&
                  escQuitPrompt && escQuitCancel && exitQuits && moveAimOk && exitPrompt && exitPaused &&
                  exitCancelled && exitBack && quitPrompt && quitCancelled && quitBack;
    SDL_Log("exitconfirm: level=%d pause=%d cancel=%d back=%d quit=%d cancel=%d back=%d",
            exitPrompt, exitPaused, exitCancelled, exitBack,
            quitPrompt, quitCancelled, quitBack);
    SDL_Log("menu: start=%d settings=%d bind=%d/%d rebind=%d/%d "
            "back=%d esc=%d escquit=%d esccancel=%d exit=%d moveaim=%d/%d/%d ok=%d",
            menuStart ? 1 : 0, menuSettings ? 1 : 0, bindWaiting ? 1 : 0,
            bindWaiting2 ? 1 : 0, menuRebind ? 1 : 0, menuRebind2 ? 1 : 0,
            settingsBack ? 1 : 0, escMenu ? 1 : 0, escQuitPrompt ? 1 : 0,
            escQuitCancel ? 1 : 0,
            exitQuits ? 1 : 0, moveAimUp ? 1 : 0,
            moveAimDown ? 1 : 0, moveAimFwd ? 1 : 0, menuOk ? 1 : 0);

    // DEV-чит: Ctrl+R создаёт дверь, вход в неё открывает выбор уровня,
    // подтверждение загружает выбранный уровень.
    resetWorld(0);
    state = ST_PLAY;
    secretDoorActive = false;
    toggleSecretDoor();
    bool devDoorSpawned = secretDoorActive && secretDoor.w > 0.0f;

    player.x = secretDoor.x + 2.0f;
    player.y = secretDoor.y + secretDoor.h - player.h;
    player.vx = player.vy = 0;
    player.grounded = true;
    update(1.0f / 120.0f, Input{});
    bool devDoorOpens = (state == ST_LEVEL_SELECT);

    levelSelectIndex = 1;
    {
        Input in;
        in.confirm = true;
        update(1.0f / 120.0f, in);
    }
    bool devSelectLoads = (state == ST_PLAY && levelIndex == 1);
    bool devSecretOk = devDoorSpawned && devDoorOpens && devSelectLoads;
    SDL_Log("devsecret: door=%d open=%d load=%d ok=%d",
            devDoorSpawned ? 1 : 0, devDoorOpens ? 1 : 0,
            devSelectLoads ? 1 : 0, devSecretOk ? 1 : 0);

    bool ok = startFirstFloor && doorBasement && climbed && camOk && stepHeld &&
              stairFall && spiderOk && spiderCrawl &&
              gotCup && killScore && popupOk && jewelScore &&
              cupCheckpointOk &&
              popupRise && popupGone &&
              oneJumpOnly && doubleJump && jumpReset &&
              spaceWhileHoldingW && shotOk && recoilOk &&
              tracerOk && bulletThrown && bulletSpeedOk && bulletKill &&
              pistolNoTurn && pistolRateOk && pistolPatrolOk && dogOnMap &&
              dogLowHitbox && dogKill &&
              diagUpOk && diagDownOk && recoilUpOk && recoilDownOk &&
              noAirShot && lookUpOk && lookDownOk &&
              lookForward && reloadOk && reloadWaitHeld && reloadStarted &&
              reloadProportional && reloadLocked &&
              reloadDone && dying && respawned && gameOver && slidesOk &&
              level2 && level2Stairs && techDecorOk && wallSlide && level3 &&
              lobbyOk && lobbyBonuses && fallDies && fallRespawns &&
              level4 && bossOnMap && bossPanelTurn && bossWallTurn &&
              bossContact && bossMath && bossPellets && bossVictory &&
              menuOk && devSecretOk;
    SDL_Log("smoke %s", ok ? "OK" : "FAIL");
}

} // namespace kolek
