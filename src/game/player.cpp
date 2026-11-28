#include "game/game.h"
#include "core/math.h"
#include "core/physics.h"

namespace kolek {

void Game::updatePlayer(float dt, const Input& in)
{
    if (player.invuln > 0) player.invuln -= dt;
    if (player.ladderLock > 0) player.ladderLock -= dt;

    // Прицел привязан к направлению движения: вверх/вниз — по W/S и ↑/↓,
    // горизонталь — по A/D. На лестнице эти же клавиши означают лазание.
    bool onLadder = ladderProbe(lvl, player);
    bool wantUp = in.up || in.lookUp;
    bool wantDown = in.down || in.lookDown;
    bool aimUp = (in.lookUp || in.up) && !onLadder;
    bool aimDown = (in.lookDown || in.down) && !onLadder;
    player.lookY = aimUp ? -1 : (aimDown ? 1 : 0);

    float ix = (in.right ? 1.0f : 0.0f) - (in.left ? 1.0f : 0.0f);
    if (reloading) ix = 0.0f;   // во время перезарядки Колян стоит на месте
    if (ix != 0) player.facing = (ix > 0) ? 1 : -1;

    if (!reloading && !player.climbing && player.ladderLock <= 0) {
        if (wantUp && onLadder) {
            player.climbing = true;
            player.vy = 0;
        } else if (wantDown && onLadder) {
            player.climbing = true;
            player.vy = 0;
        } else if (!player.grounded && player.vy > 20 && onLadder) {
            player.climbing = true;
            player.vy = 0;
        }
    }
    if (player.climbing) player.lookY = 0;

    if (!reloading && jumpEdge) player.jumpBuf = 0.12f;
    else player.jumpBuf = std::max(0.0f, player.jumpBuf - dt);

    if (player.grounded) {
        player.coyote = 0.09f;
        player.airJumps = 1;
    } else {
        player.coyote = std::max(0.0f, player.coyote - dt);
    }

    float accel = player.grounded ? 1200.0f : 820.0f;
    float speed = RUN_SPEED;
    if (player.climbing) {
        accel = 760.0f;
        speed = 56.0f;
    }
    if (ix != 0) player.vx = approach(player.vx, ix * speed, accel * dt);
    else player.vx = approach(player.vx, 0.0f,
                              (player.grounded ? 1400.0f : 600.0f) * dt);

    if (player.climbing) {
        float climbTarget = wantUp ? -CLIMB_SPEED : (wantDown ? CLIMB_SPEED + 16.0f : 0.0f);
        player.vy = approach(player.vy, climbTarget, 520.0f * dt);
        if (jumpEdge) {
            player.climbing = false;
            player.vy = -245.0f;
            player.ladderLock = 0.28f;
            player.grounded = false;
            player.coyote = 0;
            player.jumpBuf = 0;
            player.airJumps = 1;
            spawnDust(player.x + player.w / 2, player.y + player.h, 4,
                      SDL_Color{220, 220, 200, 180});
        }
    } else {
        player.vy = std::min(player.vy + GRAVITY * dt, MAX_FALL);
        if (player.jumpBuf > 0 && player.coyote > 0) {
            player.vy = -JUMP_SPEED;
            player.jumpBuf = 0;
            player.coyote = 0;
            player.grounded = false;
            player.airJumps = 1;
            spawnDust(player.x + player.w / 2, player.y + player.h, 5,
                      SDL_Color{210, 200, 180, 160});
        } else if (player.jumpBuf > 0 && player.airJumps > 0) {
            // Двойной прыжок: второй толчок прямо в воздухе.
            player.airJumps--;
            player.jumpBuf = 0;
            player.vy = -JUMP_SPEED * 0.90f;
            player.grounded = false;
            spawnSparkle(player.x + player.w / 2, player.y + player.h, 10,
                         SDL_Color{150, 220, 255, 230});
            spawnDust(player.x + player.w / 2, player.y + player.h, 5,
                      SDL_Color{210, 225, 255, 140});
            shake = std::max(shake, 0.06f);
        }
        if (!in.jumpHeld && player.vy < -125.0f) player.vy = -125.0f;
    }

    bool wasGrounded = player.grounded;
    float fallSpeed = player.vy;
    moveX(lvl, player, player.vx * dt);
    moveY(lvl, player, player.vy * dt);

    if (!wasGrounded && player.grounded && fallSpeed > 190.0f) {
        spawnDust(player.x + player.w / 2, player.y + player.h, 6,
                  SDL_Color{205, 195, 175, 170});
        shake = std::max(shake, 0.07f);
    }

    if (player.climbing) {
        int ctx = static_cast<int>((player.x + player.w * 0.5f) / TILE);
        int topRow = -1;
        for (int ty = 0; ty < lvl.h; ++ty) {
            if (lvl.at(ctx, ty) == 'L') {
                topRow = ty;
                break;
            }
        }
        if (topRow >= 0) {
            float topY = topRow * TILE;
            if (player.y + player.h < topY) {
                player.y = topY - player.h;
                player.vy = 0;
            }
        }
        bool still = ladderProbe(lvl, player);
        if (!still && !wantUp) player.climbing = false;
        if (player.grounded && !wantUp) player.climbing = false;
    }

    if (player.grounded && std::fabs(player.vx) > 6.0f && !player.climbing) {
        player.anim += std::fabs(player.vx) * dt;
    } else if (player.climbing && std::fabs(player.vy) > 6.0f) {
        player.anim += 22.0f * dt;
    }
}

void Game::updateWeapon(float dt, const Input& in)
{
    if (shootCd > 0) shootCd -= dt;
    if (muzzle > 0) muzzle -= dt;
    if (gunKick > 0) gunKick = std::max(0.0f, gunKick - dt);

    bool idle = player.hurt <= 0 && player.grounded && !player.climbing &&
                std::fabs(player.vx) < 8.0f && std::fabs(player.vy) < 8.0f &&
                !in.left && !in.right;
    if (idle && mag < MAG_SIZE) {
        standStill += dt;
        if (!reloading && standStill >= RELOAD_WAIT) {
            reloading = true;
            reloadDuration = RELOAD_FULL *
                             static_cast<float>(MAG_SIZE - mag) /
                             static_cast<float>(MAG_SIZE);
            reloadTimer = reloadDuration;
            standStill = 0;
        }
    } else if (!reloading) {
        standStill = 0;
    }

    if (reloading) {
        reloadTimer -= dt;
        if (reloadTimer <= 0) {
            reloading = false;
            mag = MAG_SIZE;
            standStill = 0;
            spawnSparkle(player.x + player.w * 0.5f, player.y + 8.0f, 8,
                         SDL_Color{255, 220, 130, 255});
        }
    }

    if (in.shootPressed && !reloading && shootCd <= 0.0f &&
        player.hurt <= 0 && !player.climbing && player.grounded) {
        fireShot(in);
    }
}

void Game::fireShot(const Input& in)
{
    if (mag <= 0) {
        if (!reloading && player.grounded && std::fabs(player.vx) < 8.0f &&
            !in.left && !in.right) {
            reloading = true;
            reloadDuration = RELOAD_FULL * static_cast<float>(MAG_SIZE - mag) /
                             static_cast<float>(MAG_SIZE);
            reloadTimer = reloadDuration;
            standStill = 0;
        }
        return;
    }

    --mag;
    shootCd = SHOT_COOLDOWN;
    muzzle = 0.10f;
    gunKick = 0.14f;

    // Мгновенное попадание: дробь летит лучами на всю длину экрана.
    // Прицел вверх/вниз поворачивает дробовик на 30 градусов,
    // а каждая дробина уходит под своим углом внутри конуса разброса.
    float aimAng = player.lookY * AIM_DIAG;
    float dirX = player.facing * std::cos(aimAng);
    float dirY = std::sin(aimAng);
    float shoulderX = player.x + player.w * 0.5f + player.facing * 1.0f;
    float shoulderY = player.y + 12.0f;
    float ox = shoulderX + dirX * 9.0f;
    float oy = shoulderY + dirY * 9.0f;
    float baseAng = std::atan2(dirY, dirX);

    for (int i = 0; i < PELLET_COUNT; ++i) {
        // Веер: средняя дробина летит прямо, крайние — в стороны.
        float fan = (i - (PELLET_COUNT - 1) * 0.5f) * SHOT_SPREAD;
        float ang = baseAng + fan + rndf(rngState, -0.006f, 0.006f);
        float dx = std::cos(ang);
        float dy = std::sin(ang);
        Monster* hit = nullptr;
        float ex = ox + dx * SHOT_RANGE;
        float ey = oy + dy * SHOT_RANGE;
        for (float t = 0.0f; t <= SHOT_RANGE; t += 2.5f) {
            float px = ox + dx * t;
            float py = oy + dy * t;
            if (lvl.solidRect(px - 1.0f, py - 1.0f, 2.0f, 2.0f)) {
                ex = px;
                ey = py;
                spawnSparkle(px, py, 2, SDL_Color{220, 200, 160, 200});
                break;
            }
            for (Monster& m : monsters) {
                if (m.dead) continue;
                if (px > m.b.x && px < m.b.x + m.b.w &&
                    py > m.b.y && py < m.b.y + m.b.h) {
                    hit = &m;
                    ex = px;
                    ey = py;
                    break;
                }
            }
            if (hit) break;
        }
        if (hit) damageMonster(*hit, 1, player.facing);

        // Угасающий трассер: от дула до точки попадания или до конца экрана.
        Tracer tr;
        tr.x0 = ox;
        tr.y0 = oy;
        tr.x1 = ex;
        tr.y1 = ey;
        tr.life = tr.maxLife = rndf(rngState, 0.18f, 0.24f);
        int warm = static_cast<int>(rndf(rngState, -18.0f, 12.0f));
        tr.color = SDL_Color{255, static_cast<Uint8>(226 + warm),
                             static_cast<Uint8>(150 + warm / 2), 255};
        tracers.push_back(tr);
    }

    for (int i = 0; i < PELLET_COUNT; ++i) {
        float ang = baseAng + rndf(rngState, -0.42f, 0.42f);
        float spd = rndf(rngState, 25.0f, 95.0f);
        Particle p;
        p.x = ox;
        p.y = oy;
        p.vx = std::cos(ang) * spd;
        p.vy = std::sin(ang) * spd;
        p.life = p.maxLife = rndf(rngState, 0.12f, 0.30f);
        p.size = rndf(rngState, 1.0f, 3.0f);
        p.grav = -40;
        p.color = SDL_Color{210, 205, 190, 170};
        parts.push_back(p);
    }
    {
        Particle c;
        c.x = player.x + player.w * 0.5f;
        c.y = player.y + 10.0f;
        c.vx = -player.facing * rndf(rngState, 40.0f, 90.0f);
        c.vy = rndf(rngState, -110.0f, -60.0f);
        c.life = c.maxLife = 0.6f;
        c.size = 2;
        c.grav = 600;
        c.color = SDL_Color{220, 170, 70, 255};
        parts.push_back(c);
    }

    // Отдача: горизонтальный выстрел толкает Коляна назад.
    // При прицеле вверх/вниз отдачу не учитываем.
    if (player.lookY == 0) player.vx -= player.facing * RECOIL_KICK;
    shake = std::max(shake, 0.16f);
}


void Game::damageMonster(Monster& m, int dmg, float dirX)
{
    if (m.dead) return;
    m.hp -= dmg;
    m.hurtFlash = 0.12f;
    // Гиганта с места не сдвинуть дробью.
    if (m.type != 4)
        m.b.vx += (dirX >= 0.0f ? 1.0f : -1.0f) * 24.0f;

    if (m.hp <= 0) {
        m.dead = true;
        ++kills;
        score += KILL_SCORE[m.type];
        addScorePopup(KILL_SCORE[m.type], SDL_Color{255, 226, 150, 255});
        shake = std::max(shake, m.type == 4 ? 0.55f : 0.30f);
        if (m.type == 4) bossDefeated = true;
        SDL_Color blood = (m.type == 0) ? SDL_Color{150, 40, 45, 255}
                        : (m.type == 3) ? SDL_Color{120, 26, 34, 255}
                        : (m.type == 4) ? SDL_Color{110, 22, 36, 255}
                                        : SDL_Color{142, 40, 42, 255};
        spawnSparkle(m.b.x + m.b.w * 0.5f, m.b.y + m.b.h * 0.5f,
                     m.type == 4 ? 42 : 18, blood);
        spawnSparkle(m.b.x + m.b.w * 0.5f, m.b.y + m.b.h * 0.5f,
                     m.type == 4 ? 20 : 8, SDL_Color{255, 200, 90, 255});
    } else {
        spawnSparkle(m.b.x + m.b.w * 0.5f, m.b.y + m.b.h * 0.5f, 5,
                     (m.type == 0) ? SDL_Color{180, 60, 60, 255}
                                   : SDL_Color{160, 60, 52, 255});
    }
}

} // namespace kolek
