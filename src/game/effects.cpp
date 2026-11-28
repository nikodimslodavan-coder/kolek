#include "game/game.h"
#include "core/math.h"

namespace kolek {

void Game::updateParticles(float dt)
{
    for (Particle& p : parts) {
        p.life -= dt;
        p.vy += p.grav * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
    }
    parts.erase(std::remove_if(parts.begin(), parts.end(),
                               [](const Particle& p) { return p.life <= 0.0f; }),
                parts.end());
}

void Game::updateScorePopups(float dt)
{
    for (ScorePopup& p : popups) {
        p.life -= dt;
        p.y -= 20.0f * dt;
    }
    popups.erase(std::remove_if(popups.begin(), popups.end(),
                                [](const ScorePopup& p) { return p.life <= 0.0f; }),
                 popups.end());
}

void Game::addScorePopup(int value, SDL_Color color)
{
    ScorePopup p;
    p.x = player.x + player.w * 0.5f;
    p.y = player.y - 12.0f - static_cast<float>(popups.size() % 3) * 9.0f;
    p.value = value;
    p.color = color;
    p.life = p.maxLife;
    popups.push_back(p);
}

void Game::updateTracers(float dt)
{
    for (Tracer& t : tracers) t.life -= dt;
    tracers.erase(std::remove_if(tracers.begin(), tracers.end(),
                                 [](const Tracer& t) { return t.life <= 0.0f; }),
                  tracers.end());
}

void Game::updateCamera(float dt)
{
    float look = clampf(player.vx * 0.30f, -34.0f, 34.0f);
    float targetX = player.x + player.w * 0.5f + look - VIEW_W * 0.5f;
    targetX = clampf(targetX, 0.0f, std::max(0.0f, static_cast<float>(lvl.w * TILE - VIEW_W)));

    // Вертикальная камера ездит по всей высоте здания. В одноэтажном
    // вестибюле она почти стоит на месте: кадр держит потолок и пол, а под
    // полом остаётся полоска тёмной шахты для пропастей.
    float lookYOff = player.lookY * 24.0f + clampf(player.vy * 0.10f, -22.0f, 22.0f);
    float targetY = player.y + player.h * 0.5f + lookYOff - VIEW_H * 0.5f;
    float maxCamY = std::max(0.0f, static_cast<float>(lvl.h * TILE - VIEW_H));
    if (levelIndex == 2 || levelIndex == 3) maxCamY = 16.0f;
    targetY = clampf(targetY, 0.0f, maxCamY);

    camX += (targetX - camX) * std::min(1.0f, dt * 9.0f);
    camY += (targetY - camY) * std::min(1.0f, dt * 8.0f);
}

void Game::snapCamera()
{
    camX = clampf(player.x + player.w * 0.5f - VIEW_W * 0.5f, 0.0f,
                  std::max(0.0f, static_cast<float>(lvl.w * TILE - VIEW_W)));
    camY = clampf(player.y + player.h * 0.5f - VIEW_H * 0.5f, 0.0f,
                  std::max(0.0f, static_cast<float>(lvl.h * TILE - VIEW_H)));
    if (levelIndex == 2 || levelIndex == 3) camY = 16.0f;
}

void Game::addLight(float x, float y, float r, SDL_Color c, float a)
{
    lights.push_back(Light{x, y, r, c, a});
}

void Game::spawnDust(float x, float y, int n, SDL_Color c)
{
    for (int i = 0; i < n; ++i) {
        Particle p;
        p.x = x + rndf(rngState, -5, 5);
        p.y = y + rndf(rngState, -2, 2);
        p.vx = rndf(rngState, -35, 35);
        p.vy = rndf(rngState, -45, 5);
        p.life = p.maxLife = rndf(rngState, 0.25f, 0.55f);
        p.size = rndf(rngState, 1, 3);
        p.grav = 120;
        p.color = c;
        parts.push_back(p);
    }
}

void Game::spawnSparkle(float x, float y, int n, SDL_Color c)
{
    for (int i = 0; i < n; ++i) {
        Particle p;
        float ang = rndf(rngState, 0, 6.2831853f);
        float spd = rndf(rngState, 25, 130);
        p.x = x;
        p.y = y;
        p.vx = std::cos(ang) * spd;
        p.vy = std::sin(ang) * spd - 30;
        p.life = p.maxLife = rndf(rngState, 0.35f, 0.8f);
        p.size = rndf(rngState, 1, 3);
        p.grav = 180;
        p.color = c;
        parts.push_back(p);
    }
}

void Game::spawnConfetti()
{
    const SDL_Color cols[] = {
        {255, 210, 90, 255}, {235, 80, 80, 255}, {110, 200, 255, 255},
        {140, 230, 140, 255}, {230, 140, 255, 255}
    };
    for (int i = 0; i < 70; ++i) {
        Particle p;
        p.x = camX + rndf(rngState, 0, static_cast<float>(VIEW_W));
        p.y = rndf(rngState, -120, -5);
        p.vx = rndf(rngState, -35, 35);
        p.vy = rndf(rngState, 20, 80);
        p.life = p.maxLife = rndf(rngState, 2.5f, 4.5f);
        p.size = rndf(rngState, 2, 4);
        p.grav = 75;
        p.color = cols[rngState % 5];
        parts.push_back(p);
    }
}

} // namespace kolek
