#pragma once

#include "core/types.h"
#include <algorithm>

namespace kolek {

inline float clampf(float v, float a, float b)
{
    return v < a ? a : (v > b ? b : v);
}

inline float approach(float value, float target, float delta)
{
    if (value < target) return std::min(value + delta, target);
    if (value > target) return std::max(value - delta, target);
    return value;
}

inline bool overlapRect(float ax, float ay, float aw, float ah,
                        float bx, float by, float bw, float bh)
{
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

inline uint32_t hash2(int x, int y)
{
    uint32_t h = static_cast<uint32_t>(x) * 374761393u +
                 static_cast<uint32_t>(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

inline SDL_Color shade(SDL_Color c, float k)
{
    auto f = [&](Uint8 v) { return static_cast<Uint8>(clampf(v * k, 0, 255)); };
    return SDL_Color{f(c.r), f(c.g), f(c.b), c.a};
}

inline float rndf(uint32_t& state, float a, float b)
{
    state = state * 1664525u + 1013904223u;
    float t = static_cast<float>(state >> 8) * (1.0f / 16777216.0f);
    return a + (b - a) * t;
}

} // namespace kolek
