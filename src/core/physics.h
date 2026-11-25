#pragma once

#include "core/level.h"
#include "core/types.h"
#include <cmath>

namespace kolek {

inline bool groundProbe(const Level& lvl, const Body& b)
{
    if (lvl.solidRect(b.x + 1.0f, b.y + b.h, b.w - 2.0f, 1.5f)) return true;
    // Ступень держит сверху: провалиться сквозь неё нельзя.
    if (lvl.stepRect(b.x + 1.0f, b.y + b.h, b.w - 2.0f, 1.5f)) return true;
    return false;
}

inline bool ladderProbe(const Level& lvl, const Body& b)
{
    // Чуть выступаем вниз, чтобы можно было "висеть" на верхней ступеньке.
    return lvl.ladderRect(b.x + 2.0f, b.y + 1.0f, b.w - 4.0f, b.h + 3.0f);
}

inline void moveX(Level& lvl, Body& b, float dx)
{
    b.blockedX = false;
    if (std::fabs(dx) < 0.0001f) return;
    float nx = b.x + dx;
    if (!lvl.solidRect(nx, b.y, b.w, b.h)) {
        b.x = nx;
        return;
    }
    b.blockedX = true;
    // Авто-шаг на 1 тайл: удобно для лесенок из мебели.
    if (b.grounded && !b.climbing) {
        float upY = b.y - TILE;
        if (!lvl.solidRect(nx, upY, b.w, b.h) &&
            lvl.solidRect(nx + 1.0f, upY + b.h, b.w - 2.0f, 1.5f)) {
            b.x = nx;
            b.y = upY;
        }
    }
}

inline void moveY(Level& lvl, Body& b, float dy)
{
    if (std::fabs(dy) < 0.0001f) {
        b.grounded = !b.climbing && groundProbe(lvl, b);
        return;
    }

    const float prevBottom = b.y + b.h;
    b.y += dy;

    if (lvl.solidRect(b.x, b.y, b.w, b.h)) {
        int x0 = static_cast<int>(std::floor(b.x / TILE));
        int x1 = static_cast<int>(std::floor((b.x + b.w - 0.001f) / TILE));
        int y0 = static_cast<int>(std::floor(b.y / TILE));
        int y1 = static_cast<int>(std::floor((b.y + b.h - 0.001f) / TILE));
        if (dy > 0) {
            float top = 1e9f;
            for (int ty = y0; ty <= y1; ++ty)
                for (int tx = x0; tx <= x1; ++tx)
                    if (lvl.solid(tx, ty)) top = std::min(top, static_cast<float>(ty * TILE));
            if (top < 1e8f) {
                b.y = top - b.h;
                b.vy = 0;
                b.grounded = true;
            } else {
                b.y -= dy;
            }
        } else {
            float bottom = -1e9f;
            for (int ty = y0; ty <= y1; ++ty)
                for (int tx = x0; tx <= x1; ++tx)
                    if (lvl.solid(tx, ty)) bottom = std::max(bottom, static_cast<float>((ty + 1) * TILE));
            if (bottom > -1e8f) {
                b.y = bottom;
                b.vy = 0;
            } else {
                b.y -= dy;
            }
            b.grounded = false;
        }
        return;
    }

    // Ступени-платформы ловят только падающее тело, если оно было над ступенью.
    if (!b.climbing && b.vy > 0.0f) {
        int x0 = static_cast<int>(std::floor(b.x / TILE));
        int x1 = static_cast<int>(std::floor((b.x + b.w - 0.001f) / TILE));
        int y0 = static_cast<int>(std::floor(prevBottom / TILE));
        int y1 = static_cast<int>(std::floor((b.y + b.h - 0.001f) / TILE));
        float top = 1e9f;
        for (int ty = y0; ty <= y1; ++ty)
            for (int tx = x0; tx <= x1; ++tx)
                if (lvl.stepTile(tx, ty)) {
                    float st = static_cast<float>(ty * TILE);
                    if (prevBottom <= st + 0.5f && b.y + b.h >= st)
                        top = std::min(top, st);
                }
        if (top < 1e8f) {
            b.y = top - b.h;
            b.vy = 0;
            b.grounded = true;
            return;
        }
    }

    b.grounded = !b.climbing && groundProbe(lvl, b);
}

} // namespace kolek
