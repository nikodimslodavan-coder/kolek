#include "game/game.h"
#include "core/math.h"
#include "core/physics.h"

namespace kolek {

void Game::updateMonsters(float dt)
{
    float pcx = player.x + player.w * 0.5f;
    float pcy = player.y + player.h * 0.5f;

    for (Monster& m : monsters) {
        if (m.dead) continue;
        if (m.hurtFlash > 0) m.hurtFlash -= dt;

        Body& b = m.b;
        float dxp = pcx - (b.x + b.w * 0.5f);
        float dyp = pcy - (b.y + b.h * 0.5f);

        if (m.type == 0) {
            updateSpider(m, dt, dxp);
            if (state == ST_PLAY && player.invuln <= 0 &&
                overlapRect(player.x + 1.0f, player.y + 3.0f,
                            player.w - 2.0f, player.h - 4.0f,
                            m.b.x + 1.0f, m.b.y + 1.0f,
                            m.b.w - 2.0f, m.b.h - 2.0f)) {
                killPlayer(DEATH_SPIDER);
                break;
            }
            continue;
        }

        b.vy = std::min(b.vy + GRAVITY * dt, MAX_FALL);
        if (m.throwCd > 0.0f) m.throwCd -= dt;

        int dir = b.facing;
        float targetSpeed = 0.0f;
        bool holding = false;    // браток с пистолетом стоит и целится

        if (m.type == 2) {
            // Браток с пистолетом: стреляет, когда Колян на одной с ним высоте
            // и стоит перед ним. Колян может зайти сзади — стрелок не
            // разворачивается, а продолжает свой патруль.
            bool aligned = state == ST_PLAY && player.invuln <= 0 &&
                           std::fabs(dyp) < 26.0f && std::fabs(dxp) < 300.0f;
            bool inFront = aligned && ((dxp > 0.0f) == (b.facing > 0));
            if (inFront) {
                targetSpeed = 0.0f;
                holding = true;
                m.anim += dt * 2.0f;
                if (m.throwCd <= 0.0f) {
                    // Пуля летит по прямой, но всё равно медленнее Коляна.
                    Bullet k;
                    k.dir = dir;
                    k.x = b.x + b.w * 0.5f + dir * 12.0f;
                    k.y = b.y + 9.0f;
                    k.vx = dir * BULLET_SPEED;
                    k.spin = rndf(rngState, 0.0f, 6.28f);
                    bullets.push_back(k);
                    // Не чаще одного выстрела в две секунды.
                    m.throwCd = 2.0f;
                    m.anim += 1.8f;
                    spawnSparkle(k.x, k.y, 3, SDL_Color{255, 226, 150, 200});
                }
            } else {
                // Быстрый патруль: ровно на 10% медленнее своей пули.
                float off = b.x - m.homeX;
                if (off > 80.0f) dir = -1;
                else if (off < -80.0f) dir = 1;
                targetSpeed = PISTOL_THUG_SPEED;
                m.anim += dt * 4.0f;
            }
        } else {
            const bool dog = (m.type == 3);
            float chaseRadius = dog ? CHASE_RADIUS * 1.6f : CHASE_RADIUS;
            bool chase = state == ST_PLAY && player.invuln <= 0 &&
                         std::fabs(dxp) <= chaseRadius &&
                         std::fabs(dyp) < (dog ? 22.0f : 30.0f);
            if (chase) {
                dir = (dxp > 0.0f) ? 1 : -1;
                // Браток с битой стал на 10% быстрее прежнего зомби (56).
                targetSpeed = dog ? 118.0f : 61.6f;
                m.anim += dt * (dog ? 16.0f : 7.7f);
            } else {
                // Патруль вокруг точки появления.
                float off = b.x - m.homeX;
                float range = dog ? 90.0f : 48.0f;
                if (off > range) dir = -1;
                else if (off < -range) dir = 1;
                targetSpeed = dog ? 42.0f : 22.0f;
                m.anim += dt * (dog ? 7.0f : 3.3f);
            }
        }
        b.facing = dir;
        b.vx = approach(b.vx, dir * targetSpeed, 900.0f * dt);

        int frontX = static_cast<int>(std::floor(
            (dir > 0 ? b.x + b.w + 3.0f : b.x - 3.0f) / TILE));
        int footY = static_cast<int>(std::floor((b.y + b.h + 2.0f) / TILE));
        int midY = static_cast<int>(std::floor((b.y + b.h - 2.0f) / TILE));
        bool edge = b.grounded && !lvl.solid(frontX, footY) && !lvl.stepTile(frontX, footY);
        bool wall = lvl.solid(frontX, midY);
        if ((edge || wall) && !holding) {
            b.facing = -dir;
            b.vx = b.facing * 26.0f;
        }

        moveX(lvl, b, b.vx * dt);
        moveY(lvl, b, b.vy * dt);
        if (b.blockedX) b.vx = -b.vx;
        if (b.grounded && std::fabs(b.vx) > 4.0f) m.anim += dt * 4.0f;

        if (state == ST_PLAY && player.invuln <= 0 &&
            overlapRect(player.x + 1.0f, player.y + 3.0f, player.w - 2.0f, player.h - 4.0f,
                        m.b.x + 1.0f, m.b.y + 1.0f, m.b.w - 2.0f, m.b.h - 2.0f)) {
            killPlayer(m.type == 0 ? DEATH_SPIDER
                                   : (m.type == 1) ? DEATH_BAT
                                   : (m.type == 2) ? DEATH_BULLET
                                                   : DEATH_DOBERMAN);
            break;
        }
    }

    monsters.erase(std::remove_if(monsters.begin(), monsters.end(),
                                  [](const Monster& m) { return m.dead; }),
                   monsters.end());
}

void Game::updateSpider(Monster& m, float dt, float dxp)
{
    Body& b = m.b;
    b.vy = 0;
    b.grounded = false;

    // Есть ли между пауком и Коляном свободный проход по вертикали:
    // спускаемся только в той же комнате, а не через перекрытие.
    float top = b.y + b.h;
    bool clear = false;
    if (player.y > top + 6.0f) {
        clear = !lvl.solidRect(b.x + 7.0f, top + 1.0f, b.w - 14.0f,
                               player.y - top - 2.0f);
    }

    if (m.webState == 0) {
        // Ползём по потолку, подбираясь к Коляну.
        m.anim += dt * 6.0f;
        bool above = state == ST_PLAY && player.invuln <= 0 && clear &&
                     std::fabs(dxp) < 24.0f;
        if (above) {
            m.webState = 1;
            m.webTimer = 0;
            m.webAnchorX = b.x + b.w * 0.5f;
            m.webAnchorY = b.y;
            m.webTargetY = player.y - b.h + 8.0f;
            if (m.webTargetY < b.y + 1.0f) m.webTargetY = b.y + 1.0f;
            return;
        }

        int dir = b.facing;
        if (clear && std::fabs(dxp) > 10.0f) dir = (dxp > 0.0f) ? 1 : -1;
        float speed = clear ? 52.0f : 32.0f;
        float nx = b.x + dir * speed * dt;
        int ceilTX = static_cast<int>(std::floor((nx + b.w * 0.5f) / TILE));
        // Тело висит в 4 px под перекрытием: смотрим на 5 px выше, иначе
        // проверка попадает уже в пустую клетку и паук дёргается на месте.
        int ceilTY = static_cast<int>(std::floor((b.y - 5.0f) / TILE));
        int frontX = static_cast<int>(std::floor(
            (dir > 0 ? nx + b.w + 1.0f : nx - 1.0f) / TILE));
        int bodyTY = static_cast<int>(std::floor((b.y + b.h * 0.5f) / TILE));
        if (!lvl.solid(ceilTX, ceilTY) || lvl.solid(frontX, bodyTY)) {
            b.facing = -dir;
        } else {
            b.facing = dir;
            b.x = nx;
        }
    } else if (m.webState == 1) {
        // Спуск на паутине прямо над Коляном.
        m.webTimer += dt;
        b.y += 115.0f * dt;
        b.x = m.webAnchorX - b.w * 0.5f;
        if (b.y >= m.webTargetY) {
            b.y = m.webTargetY;
            m.webState = 2;
            m.webTimer = 0;
        }
    } else if (m.webState == 2) {
        // Повис, покачался — и обратно на потолок.
        m.webTimer += dt;
        b.x = m.webAnchorX - b.w * 0.5f +
              std::sin(m.webTimer * 7.0f) * 2.0f;
        if (m.webTimer > 0.7f) {
            m.webState = 3;
            m.webTimer = 0;
        }
    } else {
        b.y -= 135.0f * dt;
        b.x = m.webAnchorX - b.w * 0.5f;
        if (b.y <= m.ceilingY) {
            b.y = m.ceilingY;
            m.webState = 0;
            b.facing = (dxp > 0.0f) ? 1 : -1;
        }
    }
}

void Game::updateBullets(float dt)
{
    std::vector<Bullet> keep;
    keep.reserve(bullets.size());
    for (Bullet& k : bullets) {
        k.x += k.vx * dt;
        k.spin += dt * 13.0f;

        // Пуля врезается в стену (или ступень) — искры и вон.
        if (lvl.solidRect(k.x - 2.0f, k.y - 1.0f, 4.0f, 2.0f) ||
            lvl.stepRect(k.x - 2.0f, k.y - 1.0f, 4.0f, 2.0f)) {
            spawnSparkle(k.x, k.y, 5, SDL_Color{222, 205, 168, 220});
            continue;
        }

        if (state == ST_PLAY && player.invuln <= 0 &&
            overlapRect(k.x - 3.0f, k.y - 2.5f, 6.0f, 5.0f,
                        player.x, player.y, player.w, player.h)) {
            killPlayer(DEATH_BULLET);   // сама пуля и остальные снимаются смертью
            return;
        }

        keep.push_back(k);
    }
    bullets.swap(keep);
}

} // namespace kolek
