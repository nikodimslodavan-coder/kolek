#pragma once

#include "core/constants.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace kolek {

struct Level {
    std::vector<std::string> rows;
    int w = 0, h = 0;
    float spawnPX = 0, spawnPY = 0;

    void load(const char* const* data)
    {
        rows.clear();
        for (int i = 0; data[i] != nullptr; ++i) rows.emplace_back(data[i]);
        size_t maxw = 0;
        for (const std::string& r : rows) maxw = std::max(maxw, r.size());
        for (std::string& r : rows) r.resize(maxw, '.');
        w = static_cast<int>(maxw);
        h = static_cast<int>(rows.size());

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                char& c = rows[y][x];
                if (c == 'P') {
                    spawnPX = x * TILE + 3.0f;
                    spawnPY = y * TILE + TILE - PLAYER_H;
                    c = '.';
                }
            }
        }
    }

    char at(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= w || y >= h) return '#';
        return rows[y][x];
    }

    bool solid(int x, int y) const
    {
        char c = at(x, y);
        return c == '#' || c == '=' || c == 't';
    }

    // Ступень-платформа: твёрдая только сверху (падение на неё).
    bool stepTile(int x, int y) const
    {
        return at(x, y) == 'S';
    }

    template <typename F>
    bool scanRect(float x, float y, float ww, float hh, F pred) const
    {
        if (ww <= 0 || hh <= 0) return false;
        int x0 = static_cast<int>(std::floor(x / TILE));
        int x1 = static_cast<int>(std::floor((x + ww - 0.001f) / TILE));
        int y0 = static_cast<int>(std::floor(y / TILE));
        int y1 = static_cast<int>(std::floor((y + hh - 0.001f) / TILE));
        for (int ty = y0; ty <= y1; ++ty)
            for (int tx = x0; tx <= x1; ++tx)
                if (pred(tx, ty)) return true;
        return false;
    }

    bool solidRect(float x, float y, float ww, float hh) const
    {
        return scanRect(x, y, ww, hh, [&](int tx, int ty) { return solid(tx, ty); });
    }

    bool stepRect(float x, float y, float ww, float hh) const
    {
        return scanRect(x, y, ww, hh, [&](int tx, int ty) { return stepTile(tx, ty); });
    }

    bool ladderRect(float x, float y, float ww, float hh) const
    {
        return scanRect(x, y, ww, hh, [&](int tx, int ty) { return at(tx, ty) == 'L'; });
    }
};

} // namespace kolek
