#include "game/game.h"
#include "core/math.h"
#include "rendering/font.h"

namespace kolek {

bool Game::init(SDL_Renderer* r)
{
    ren = r;
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

    wallPalette[0] = SDL_Color{ 96, 54, 58, 255 };
    wallPalette[1] = SDL_Color{ 58, 78, 62, 255 };
    wallPalette[2] = SDL_Color{ 82, 56, 74, 255 };
    wallPalette[3] = SDL_Color{ 54, 66, 88, 255 };
    wallPalette[4] = SDL_Color{ 98, 72, 48, 255 };
    wallPalette[5] = SDL_Color{ 70, 50, 64, 255 };
    wallPalette[6] = SDL_Color{ 84, 64, 46, 255 };

    loadControls();

    // Мягкое радиальное свечение
    {
        const int S = 128;
        std::vector<uint32_t> px(static_cast<size_t>(S) * S, 0);
        for (int y = 0; y < S; ++y) {
            for (int x = 0; x < S; ++x) {
                float dx = (x + 0.5f) / S * 2.0f - 1.0f;
                float dy = (y + 0.5f) / S * 2.0f - 1.0f;
                float d = std::sqrt(dx * dx + dy * dy);
                float a = clampf(1.0f - d, 0.0f, 1.0f);
                a = a * a;
                uint32_t al = static_cast<uint32_t>(a * 200.0f);
                px[static_cast<size_t>(y) * S + x] = (al << 24) | 0x00FFFFFFu;
            }
        }
        glowTex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STATIC, S, S);
        SDL_UpdateTexture(glowTex, nullptr, px.data(), S * 4);
        SDL_SetTextureBlendMode(glowTex, SDL_BLENDMODE_ADD);
    }

    // Виньетка на весь экран
    {
        std::vector<uint32_t> px(static_cast<size_t>(VIEW_W) * VIEW_H, 0);
        for (int y = 0; y < VIEW_H; ++y) {
            for (int x = 0; x < VIEW_W; ++x) {
                float dx = (x + 0.5f) / VIEW_W * 2.0f - 1.0f;
                float dy = (y + 0.5f) / VIEW_H * 2.0f - 1.0f;
                float d = std::sqrt(dx * dx * 1.10f + dy * dy * 1.35f);
                float t = clampf((d - 0.58f) / 0.62f, 0.0f, 1.0f);
                t = t * t * (3.0f - 2.0f * t);
                uint32_t al = static_cast<uint32_t>(t * 185.0f);
                px[static_cast<size_t>(y) * VIEW_W + x] = al << 24;
            }
        }
        vignetteTex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                                        SDL_TEXTUREACCESS_STATIC, VIEW_W, VIEW_H);
        SDL_UpdateTexture(vignetteTex, nullptr, px.data(), VIEW_W * 4);
        SDL_SetTextureBlendMode(vignetteTex, SDL_BLENDMODE_BLEND);
    }

    resetWorld(0);
    state = ST_TITLE;
    return true;
}

void Game::render()
{
    shakeX = shakeY = 0;
    if (shake > 0.001f) {
        shakeX = std::sin(timeSec * 95.0f) * shake * 9.0f;
        shakeY = std::cos(timeSec * 83.0f) * shake * 5.0f;
    }
    lights.clear();

    SDL_SetRenderDrawColor(ren, 18, 12, 16, 255);
    SDL_RenderClear(ren);

    drawWallpaper();
    drawTiles();
    drawRugs();
    drawDecor();
    drawDoor();
    drawSecretDoor();
    drawCups();
    drawJewelry();
    drawMonsters();

    const Palette pp{
        SDL_Color{ 52, 84, 148, 255 }, SDL_Color{ 36, 58, 108, 255 },
        SDL_Color{ 44, 40, 54, 255 },  SDL_Color{ 32, 25, 27, 255 },
        SDL_Color{ 238, 190, 150, 255 }, SDL_Color{ 70, 44, 32, 255 },
        SDL_Color{ 190, 58, 62, 255 }, SDL_Color{ 24, 18, 24, 255 }
    };

    drawGuy(player, pp);
    if (state != ST_WIN) {
        drawBullets();
        drawTracers();
        drawParticles();
    }
    drawGlows();
    drawVignette();
    drawScorePopups();

    if (state == ST_TITLE) {
        drawTitleScreen();
    } else if (state == ST_SETTINGS) {
        drawSettingsScreen();
    } else if (state == ST_SLIDE) {
        drawSlideScreen();
    } else if (state == ST_LEVEL_SELECT) {
        drawLevelSelectScreen();
    } else if (state == ST_PLAY) {
        drawHUD();
    } else if (state == ST_DYING) {
        drawHUD();
        drawDeathOverlay();
    } else if (state == ST_GAMEOVER) {
        drawGameOverScreen();
    } else {
        drawWinScreen();
    }

    if (hurtFlash > 0.0f) {
        SDL_SetRenderDrawColor(ren, 190, 40, 40, static_cast<Uint8>(hurtFlash * 90.0f));
        SDL_RenderFillRect(ren, nullptr);
    }

    if (exitAction != EXIT_NONE) drawExitConfirmation();
    drawTouchControls();
}

void Game::fillW(float wx, float wy, float ww, float hh, SDL_Color c)
{
    SDL_Rect r{sx(wx), sy(wy),
               static_cast<int>(std::ceil(ww)), static_cast<int>(std::ceil(hh))};
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(ren, &r);
}

void Game::fillCircle(int cx, int cy, int r, SDL_Color c)
{
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
    for (int dy = -r; dy <= r; ++dy) {
        int dx = static_cast<int>(std::sqrt(std::max(0, r * r - dy * dy)));
        SDL_RenderDrawLine(ren, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void Game::fillTriangle(int ax, int ay, int bx, int by, int cx, int cy, SDL_Color c)
{
    SDL_Vertex v[3];
    v[0].position = SDL_FPoint{static_cast<float>(ax), static_cast<float>(ay)};
    v[1].position = SDL_FPoint{static_cast<float>(bx), static_cast<float>(by)};
    v[2].position = SDL_FPoint{static_cast<float>(cx), static_cast<float>(cy)};
    for (int i = 0; i < 3; ++i) {
        v[i].color = c;
        v[i].tex_coord = SDL_FPoint{0, 0};
    }
    SDL_RenderGeometry(ren, nullptr, v, 3, nullptr, 0);
}

void Game::drawCupIcon(int x, int y, float bob)
{
    int yy = y + static_cast<int>(std::lround(bob));

    // ручки
    SDL_SetRenderDrawColor(ren, 226, 168, 46, 255);
    SDL_Rect h1{x - 2, yy + 1, 2, 3};
    SDL_Rect h2{x + 12, yy + 1, 2, 3};
    SDL_RenderFillRect(ren, &h1);
    SDL_RenderFillRect(ren, &h2);

    // чаша
    SDL_SetRenderDrawColor(ren, 255, 205, 70, 255);
    SDL_Rect rim{x, yy, 12, 3};
    SDL_RenderFillRect(ren, &rim);
    SDL_SetRenderDrawColor(ren, 238, 182, 52, 255);
    SDL_Rect bowl{x + 1, yy + 3, 10, 3};
    SDL_RenderFillRect(ren, &bowl);
    SDL_SetRenderDrawColor(ren, 206, 146, 34, 255);
    SDL_Rect taper{x + 3, yy + 6, 6, 2};
    SDL_RenderFillRect(ren, &taper);

    // ножка и основание
    SDL_SetRenderDrawColor(ren, 222, 164, 44, 255);
    SDL_Rect stem{x + 5, yy + 8, 2, 2};
    SDL_RenderFillRect(ren, &stem);
    SDL_SetRenderDrawColor(ren, 246, 194, 62, 255);
    SDL_Rect base{x + 3, yy + 10, 6, 2};
    SDL_RenderFillRect(ren, &base);

    // блик
    SDL_SetRenderDrawColor(ren, 255, 246, 196, 210);
    SDL_Rect shine{x + 2, yy + 1, 2, 1};
    SDL_RenderFillRect(ren, &shine);
}

} // namespace kolek
