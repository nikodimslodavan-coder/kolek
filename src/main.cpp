// ---------------------------------------------------------------------------
//  "Апартаменты Даймонд" — боевой 2D-скроллер в духе Dangerous Dave.
//  Колян с дробовиком зачищает три этажа особняка: подвал, первый и второй.
//  C++17 + SDL2, без внешних ассетов: вся графика рисуется прямоугольниками.
//
//  Сборка: см. build.bat / CMakeLists.txt / README.md
// ---------------------------------------------------------------------------
#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "font.h"
#include "level.h"

namespace {

// ---------------------------------------------------------------------------
//  Константы
// ---------------------------------------------------------------------------
constexpr int   TILE        = 16;
constexpr int   VIEW_W      = 480;
constexpr int   VIEW_H      = 272;

constexpr float GRAVITY     = 1120.0f;
constexpr float MAX_FALL    = 430.0f;
constexpr float RUN_SPEED   = 138.0f;
constexpr float JUMP_SPEED  = 305.0f;
constexpr float CLIMB_SPEED = 78.0f;

constexpr int   MAG_SIZE       = 8;      // патронов в дробовике
constexpr float SHOT_COOLDOWN  = 0.40f;
constexpr float RELOAD_FULL    = 2.00f;  // полная перезарядка восьми патронов
constexpr float RELOAD_WAIT    = 1.00f;  // секунда полной неподвижности до перезарядки
constexpr float SHOT_RANGE     = 640.0f; // выстрел на всю длину экрана
constexpr int   PELLET_COUNT   = 7;      // дробин в залпе
constexpr float SHOT_SPREAD    = 0.032f; // шаг разброса: крайние дроби уходят на ~±5.5°
constexpr float RECOIL_KICK    = 180.0f; // отдача толкает Коляна назад
constexpr int   START_LIVES    = 3;
constexpr float KNIFE_SPEED    = 52.0f;  // нож летит медленно: игрок быстрее втрое
constexpr float DEATH_TIME     = 2.6f;   // пауза с анимацией смерти
constexpr float CHASE_RADIUS   = 34.0f;  // «два шага» до Коляна
constexpr float PLAYER_H       = 30.0f;  // Колян в полтора раза выше прежнего

constexpr int   CUP_SCORE      = 1000;   // кубок-трофей
constexpr int   KILL_SCORE[3]  = {200, 100, 300};   // паук, зомби, метатель ножей
constexpr int   JEWEL_SCORE[3] = {200, 400, 600};   // три вида украшений

enum GameState {
    ST_TITLE = 0, ST_PLAY = 1, ST_WIN = 2, ST_DYING = 3, ST_GAMEOVER = 4,
    ST_SETTINGS = 5
};
enum DeathCause { DEATH_SPIDER = 0, DEATH_ZOMBIE = 1, DEATH_KNIFE = 2 };

// ---------------------------------------------------------------------------
//  Мелкие структуры
// ---------------------------------------------------------------------------
struct RectF {
    float x = 0, y = 0, w = 0, h = 0;
};

struct Body {
    float x = 0, y = 0, w = 10, h = PLAYER_H;
    float vx = 0, vy = 0;
    bool  grounded = false;
    bool  climbing = false;
    bool  blockedX = false;
    int   facing = 1;
    int   airJumps = 0;
    float coyote = 0;
    float jumpBuf = 0;
    float ladderLock = 0;
    float anim = 0;
    float hurt = 0;
    float invuln = 0;
    float safeTimer = 0;
    int   lookY = 0;         // -1 — смотрит вверх, 0 — вперёд, 1 — вниз
};

struct Monster {
    int   type = 0;          // 0 — паук, 1 — зомби (оба наземные)
    Body  b;
    int   hp = 1, maxHp = 1;
    float anim = 0;
    float hurtFlash = 0;
    float homeX = 0, homeY = 0;
    float seed = 0;
    float turnTimer = 0;
    float throwCd = 0;       // метатель ножей: пауза до следующего броска
    // Паук: 0 — ползёт по потолку, 1 — спускается на паутине,
    // 2 — висит, 3 — поднимается обратно.
    int   webState = 0;
    float ceilingY = 0;
    float webAnchorX = 0, webAnchorY = 0, webTargetY = 0;
    float webTimer = 0;
    bool  dead = false;
};

struct Particle {
    float x = 0, y = 0, vx = 0, vy = 0;
    float life = 0, maxLife = 1;
    float size = 2, grav = 400;
    SDL_Color color{255, 255, 255, 255};
};

// Угасающий след дроби: отрезок, который быстро тает после выстрела.
struct Tracer {
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    float life = 0, maxLife = 0.2f;
    SDL_Color color{255, 230, 160, 255};
};

// Нож метателя: летит медленно и горизонтально (игрок заметно быстрее).
struct Knife {
    float x = 0, y = 0;      // центр ножа в мировых координатах
    float vx = 0;            // горизонтальная скорость
    int   dir = 1;           // куда смотрит остриё
    float spin = 0;          // лёгкое покачивание в полёте
};

// Кубок-трофей: три штуки открывают парадную дверь, каждый даёт 1000 очков.
struct CupItem {
    int  tx = 0, ty = 0;
    bool taken = false;
};

// Украшение: случайно лежит на полу и приносит 200, 400 или 600 очков.
struct Jewel {
    float x = 0, y = 0;
    int   type = 0;
    bool  taken = false;
    float phase = 0;
};

// Всплывающие очки: короткая надпись поднимается над Коляном и тает.
struct ScorePopup {
    float x = 0, y = 0;
    int   value = 0;
    float life = 0, maxLife = 1.1f;
    SDL_Color color{255, 226, 150, 255};
};

struct Decor {
    int tx = 0, ty = 0, type = 0;
};

struct Light {
    float x = 0, y = 0, r = 60;
    SDL_Color color{255, 220, 140, 255};
    float alpha = 100;
};

enum DecorType {
    DEC_WINDOW, DEC_PAINTING, DEC_SCONCE, DEC_CHANDELIER,
    DEC_SHELF, DEC_MIRROR, DEC_CLOCK
};

struct Input {
    bool left = false, right = false, up = false, down = false;
    bool lookUp = false, lookDown = false;
    bool spacePressed = false, jumpHeld = false;
    bool shootPressed = false;
    bool confirm = false;
    bool restart = false;
    // Навигация по меню (не переназначается).
    bool menuUp = false, menuDown = false, menuLeft = false, menuRight = false;
    bool back = false;
};

// ---------------------------------------------------------------------------
//  Управление
// ---------------------------------------------------------------------------
enum ControlAction {
    CA_LEFT = 0, CA_RIGHT, CA_UP, CA_DOWN, CA_JUMP,
    CA_SHOOT, CA_RESTART, CA_COUNT
};

// На каждое действие можно назначить две клавиши: основную и запасную.
struct Controls {
    SDL_Scancode keys[CA_COUNT][2] = {};

    void reset()
    {
        for (int i = 0; i < CA_COUNT; ++i) {
            keys[i][0] = SDL_SCANCODE_UNKNOWN;
            keys[i][1] = SDL_SCANCODE_UNKNOWN;
        }
        keys[CA_LEFT][0]    = SDL_SCANCODE_A;
        keys[CA_LEFT][1]    = SDL_SCANCODE_LEFT;
        keys[CA_RIGHT][0]   = SDL_SCANCODE_D;
        keys[CA_RIGHT][1]   = SDL_SCANCODE_RIGHT;
        keys[CA_UP][0]      = SDL_SCANCODE_W;
        keys[CA_UP][1]      = SDL_SCANCODE_UP;
        keys[CA_DOWN][0]    = SDL_SCANCODE_S;
        keys[CA_DOWN][1]    = SDL_SCANCODE_DOWN;
        // W остаётся и прыжком, и движением вверх — как в прежней версии.
        keys[CA_JUMP][0]    = SDL_SCANCODE_SPACE;
        keys[CA_JUMP][1]    = SDL_SCANCODE_W;
        keys[CA_SHOOT][0]   = SDL_SCANCODE_LCTRL;
        keys[CA_SHOOT][1]   = SDL_SCANCODE_RCTRL;
        keys[CA_RESTART][0] = SDL_SCANCODE_R;
    }
};

inline const char* controlName(int action)
{
    switch (action) {
        case CA_LEFT:    return "ВЛЕВО";
        case CA_RIGHT:   return "ВПРАВО";
        case CA_UP:      return "ВВЕРХ / ЛАЗАТЬ";
        case CA_DOWN:    return "ВНИЗ / ЛАЗАТЬ";
        case CA_JUMP:    return "ПРЫЖОК";
        case CA_SHOOT:   return "ВЫСТРЕЛ";
        case CA_RESTART: return "ЗАНОВО";
        default:         return "";
    }
}

inline std::string configPath()
{
    std::string path;
    char* base = SDL_GetBasePath();
    if (base) {
        path = base;
        SDL_free(base);
    }
    path += "kolek.cfg";
    return path;
}

inline std::string keyLabel(SDL_Scancode sc)
{
    if (sc == SDL_SCANCODE_UNKNOWN) return "НЕТ";
    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
        return std::string(1, static_cast<char>('A' + (sc - SDL_SCANCODE_A)));
    if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9)
        return std::string(1, static_cast<char>('1' + (sc - SDL_SCANCODE_1)));
    if (sc == SDL_SCANCODE_0) return "0";
    switch (sc) {
        case SDL_SCANCODE_SPACE:     return "ПРОБЕЛ";
        case SDL_SCANCODE_UP:        return "↑";
        case SDL_SCANCODE_DOWN:      return "↓";
        case SDL_SCANCODE_LEFT:      return "←";
        case SDL_SCANCODE_RIGHT:     return "→";
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL:     return "CTRL";
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:    return "SHIFT";
        case SDL_SCANCODE_LALT:
        case SDL_SCANCODE_RALT:      return "ALT";
        case SDL_SCANCODE_RETURN:    return "ENTER";
        case SDL_SCANCODE_TAB:       return "TAB";
        case SDL_SCANCODE_BACKSPACE: return "BACKSPACE";
        default: break;
    }
    if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12)
        return "F" + std::to_string(sc - SDL_SCANCODE_F1 + 1);
    return "КЛАВИША " + std::to_string(static_cast<int>(sc));
}

// ---------------------------------------------------------------------------
//  Карта уровня
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
//  Вспомогательное
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
//  Физика (общая для игрока и монстров)
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
//  Игра
// ---------------------------------------------------------------------------
struct Game {
    struct Palette {
        SDL_Color coat, coatDark, pants, shoe, skin, hair, accent, eye;
    };

    SDL_Renderer* ren = nullptr;
    Level lvl;
    Body player;
    std::vector<CupItem> cups;
    std::vector<Jewel> jewelry;
    std::vector<ScorePopup> popups;
    std::vector<Decor> decor;
    std::vector<RectF> rugs;
    std::vector<Particle> parts;
    std::vector<Tracer> tracers;
    std::vector<Knife> knives;
    std::vector<Light> lights;
    std::vector<Monster> monsters;

    RectF door{};
    RectF checkpoint{};
    int state = ST_TITLE;
    Controls controls;
    int menuIndex = 0;
    int settingsIndex = 0;
    int settingsSlot = 0;         // 0 или 1 — какую из двух клавиш меняем
    int bindTarget = -1;          // действие, ждущее новую клавишу
    int bindSlot = 0;             // слот переназначаемой клавиши
    bool quitRequested = false;   // «Выход» в главном меню
    bool persistControls = true;  // не трогать файл в служебных режимах
    int levelIndex = 0;
    int cupsGot = 0;
    int kills = 0;
    int score = 0;
    int lives = START_LIVES;
    int deathCause = DEATH_SPIDER;
    float deathTimer = 0;

    int mag = MAG_SIZE;
    bool reloading = false;
    float shootCd = 0, reloadTimer = 0, reloadDuration = RELOAD_FULL;
    float standStill = 0, muzzle = 0, gunKick = 0;
    bool prevSpacePressed = false;
    bool jumpEdge = false;

    float camX = 0, camY = 0, timeSec = 0;
    float shake = 0, hurtFlash = 0, winDelay = 0;
    float introTimer = 0, titleBlink = 0;
    float shakeX = 0, shakeY = 0;
    float demoJump = 0;

    SDL_Texture* glowTex = nullptr;
    SDL_Texture* vignetteTex = nullptr;

    uint32_t rngState = 0x1a2b3c4du;
    SDL_Color wallPalette[7];

    bool init(SDL_Renderer* r);
    void resetWorld(int index);
    void startGame();
    void assignBind(SDL_Scancode sc);
    void loadControls();
    void saveControls() const;
    void update(float dt, const Input& in);
    void render();

    // --- геймплей -----------------------------------------------------------
    void updatePlayer(float dt, const Input& in);
    void updateMonsters(float dt);
    void updateSpider(Monster& m, float dt, float dxp);
    void updateKnives(float dt);
    void updateWeapon(float dt, const Input& in);
    void updateParticles(float dt);
    void updateTracers(float dt);
    void updateCheckpoint(float dt);
    void updateCamera(float dt);
    void snapCamera();
    void updateDeath(float dt);
    void checkPickups();
    void checkDoor(float dt);
    void advanceLevel();
    void respawnPlayer();
    void killPlayer(int cause);
    void fireShot(const Input& in);
    void damageMonster(Monster& m, int dmg, float dirX);
    void addScorePopup(int value, SDL_Color color);
    void updateScorePopups(float dt);
    void addLight(float x, float y, float r, SDL_Color c, float a);
    void spawnDust(float x, float y, int n, SDL_Color c);
    void spawnSparkle(float x, float y, int n, SDL_Color c);
    void spawnConfetti();

    // --- рисование ----------------------------------------------------------
    int sx(float wx) const { return static_cast<int>(std::lround(wx - camX + shakeX)); }
    int sy(float wy) const { return static_cast<int>(std::lround(wy - camY + shakeY)); }
    void fillW(float wx, float wy, float ww, float hh, SDL_Color c);
    void fillCircle(int cx, int cy, int r, SDL_Color c);
    void fillTriangle(int ax, int ay, int bx, int by, int cx, int cy, SDL_Color c);
    void drawCupIcon(int x, int y, float bob);

    void drawWallpaper();
    void drawTiles();
    void drawBrick(int x, int y, int tx, int ty);
    void drawFloorTile(int x, int y, int tx, int ty);
    void drawTableTile(int x, int y, int tx, int ty);
    void drawStepTile(int x, int y, int tx, int ty);
    void drawLadder(int x, int y);
    void drawPitTile(int x, int y, int tx, int ty);
    void drawRugs();
    void drawDecor();
    void drawWindow(int wx, int wy);
    void drawPainting(int wx, int wy, int variant);
    void drawSconce(int wx, int wy);
    void drawChandelier(int wx, int wy);
    void drawShelf(int wx, int wy);
    void drawMirror(int wx, int wy);
    void drawClock(int wx, int wy);
    void drawDoor();
    void drawCups();
    void drawJewelry();
    void drawJewelIcon(int x, int y, int type, float bob);
    void drawMonsters();
    void drawSpider(const Monster& m);
    void drawZombie(const Monster& m);
    void drawKnifeThrower(const Monster& m);
    void drawKnives();
    void drawGuy(const Body& b, const Palette& p);
    void drawParticles();
    void drawTracers();
    void drawGlows();
    void drawVignette();
    void drawScorePopups();
    void drawHUD();
    void drawDeathOverlay();
    void drawTitleScreen();
    void drawSettingsScreen();
    void drawWinScreen();
    void drawGameOverScreen();

    // --- служебное ----------------------------------------------------------
    void screenshot(const char* path);
    void makeScenarios();
    void runSmoke();
};

// ---------------------------------------------------------------------------
//  Случайные числа
// ---------------------------------------------------------------------------
inline float rndf(uint32_t& state, float a, float b)
{
    state = state * 1664525u + 1013904223u;
    float t = static_cast<float>(state >> 8) * (1.0f / 16777216.0f);
    return a + (b - a) * t;
}

// ---------------------------------------------------------------------------
//  Инициализация и сброс
// ---------------------------------------------------------------------------
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

void Game::resetWorld(int index)
{
    levelIndex = std::max(0, std::min(index, LEVEL_COUNT - 1));
    lvl.load(LEVEL_MAPS[levelIndex]);

    player = Body{};
    player.w = 10;
    player.h = PLAYER_H;
    player.x = lvl.spawnPX;
    player.y = lvl.spawnPY;
    player.facing = 1;
    player.airJumps = 1;

    // кубки
    cups.clear();
    for (int ty = 0; ty < lvl.h; ++ty)
        for (int tx = 0; tx < lvl.w; ++tx)
            if (lvl.at(tx, ty) == 'K') cups.push_back(CupItem{tx, ty, false});
    cupsGot = 0;
    if (levelIndex == 0) score = 0;
    popups.clear();

    // дверь
    door = RectF{};
    int dx0 = lvl.w, dx1 = -1, dy0 = lvl.h, dy1 = -1;
    for (int ty = 0; ty < lvl.h; ++ty) {
        for (int tx = 0; tx < lvl.w; ++tx) {
            if (lvl.at(tx, ty) == 'D') {
                dx0 = std::min(dx0, tx);
                dx1 = std::max(dx1, tx);
                dy0 = std::min(dy0, ty);
                dy1 = std::max(dy1, ty);
            }
        }
    }
    if (dx1 >= 0) {
        door.x = dx0 * TILE;
        door.y = dy0 * TILE;
        door.w = (dx1 - dx0 + 1) * TILE;
        door.h = (dy1 - dy0 + 1) * TILE;
    }

    // украшения: девять случайных тайлов на твёрдом полу, по три каждого вида
    jewelry.clear();
    std::vector<RectF> spots;
    for (int ty = 1; ty < lvl.h; ++ty) {
        for (int tx = 0; tx < lvl.w; ++tx) {
            if (lvl.at(tx, ty) != '.') continue;
            if (!lvl.solid(tx, ty + 1)) continue;
            float jx = tx * 16.0f + 3.0f;
            float jy = ty * 16.0f + 4.0f;
            if (std::fabs(jx - player.x) < 64.0f &&
                std::fabs(jy - player.y) < 96.0f) continue;
            if (door.w > 0.0f &&
                overlapRect(jx - 8.0f, jy - 8.0f, 28.0f, 24.0f,
                            door.x - 24.0f, door.y - 24.0f,
                            door.w + 48.0f, door.h + 48.0f)) continue;
            bool nearCup = false;
            for (const CupItem& c : cups)
                if (std::abs(c.tx - tx) <= 2 && std::abs(c.ty - ty) <= 2)
                    nearCup = true;
            if (nearCup) continue;
            bool nearOther = false;
            for (const RectF& s : spots)
                if (std::fabs(s.x - jx) < 48.0f &&
                    std::fabs(s.y - jy) < 28.0f) nearOther = true;
            if (nearOther) continue;
            spots.push_back(RectF{jx, jy, 11.0f, 10.0f});
        }
    }
    for (size_t i = spots.size(); i > 1; --i) {
        size_t j = static_cast<size_t>(rndf(rngState, 0.0f, static_cast<float>(i)));
        if (j >= i) j = i - 1;
        std::swap(spots[i - 1], spots[j]);
    }
    int jewelCount = std::min<int>(9, static_cast<int>(spots.size()));
    for (int i = 0; i < jewelCount; ++i) {
        Jewel jw;
        jw.x = spots[i].x;
        jw.y = spots[i].y;
        jw.type = i % 3;
        jw.phase = rndf(rngState, 0.0f, 6.28f);
        jewelry.push_back(jw);
    }

    // декор стен
    decor.clear();
    auto D = [&](int tx, int ty, DecorType t) { decor.push_back(Decor{tx, ty, static_cast<int>(t)}); };

    if (levelIndex == 0) {
        const int gWin[][2] = {{6,9},{22,9},{43,9},{59,9},{71,9},{86,9},{107,9},{121,9},{137,9},{151,9}};
        for (auto& p : gWin) D(p[0], p[1], DEC_WINDOW);
        const int gPaint[][2] = {{11,10},{18,10},{25,10},{34,10},{64,10},{80,10},{93,10},{112,10},{127,10},{141,10},{148,10}};
        for (int i = 0; i < static_cast<int>(sizeof(gPaint) / sizeof(gPaint[0])); ++i)
            if (i % 2 == 0) D(gPaint[i][0], gPaint[i][1], DEC_PAINTING);
        const int gSconce[][2] = {{2,11},{15,11},{30,11},{32,11},{52,11},{56,11},{75,11},{79,11},
                                  {102,11},{105,11},{131,11},{134,11},{144,11},{146,11},{155,11}};
        for (auto& p : gSconce) D(p[0], p[1], DEC_SCONCE);
        const int gChand[][2] = {{9,8},{28,8},{39,8},{68,8},{95,8},{116,8},{140,8}};
        for (auto& p : gChand) D(p[0], p[1], DEC_CHANDELIER);
        const int gShelf[][2] = {{33,12},{56,12},{81,12},{100,12},{125,12}};
        for (auto& p : gShelf) D(p[0], p[1], DEC_SHELF);
        D(146, 12, DEC_CLOCK);

        const int uWin[][2] = {{5,2},{12,2},{20,2},{33,2},{42,2},{57,2},{64,2},{76,2},
                               {91,2},{99,2},{112,2},{120,2},{129,2},{146,2},{154,2}};
        for (auto& p : uWin) D(p[0], p[1], DEC_WINDOW);
        const int uPaint[][2] = {{8,5},{16,5},{24,5},{31,5},{53,5},{68,5},{80,5},{87,5},
                                 {103,5},{116,5},{126,5},{138,5},{150,5}};
        for (int i = 0; i < static_cast<int>(sizeof(uPaint) / sizeof(uPaint[0])); ++i)
            if (i % 2 == 0) D(uPaint[i][0], uPaint[i][1], DEC_PAINTING);
        const int uSconce[][2] = {{2,6},{10,6},{18,6},{29,6},{35,6},{51,6},{60,6},{72,6},
                                  {85,6},{97,6},{110,6},{123,6},{131,6},{144,6},{156,6}};
        for (auto& p : uSconce) D(p[0], p[1], DEC_SCONCE);
        const int uChand[][2] = {{7,0},{22,0},{37,0},{50,0},{63,0},{83,0},{95,0},{117,0},{127,0},{150,0}};
        for (auto& p : uChand) D(p[0], p[1], DEC_CHANDELIER);
        const int uShelf[][2] = {{30,5},{55,5},{94,5}};
        for (auto& p : uShelf) D(p[0], p[1], DEC_SHELF);
        D(22, 5, DEC_MIRROR);
        D(66, 5, DEC_MIRROR);
    } else {
        // Второй этаж декорируем процедурно, чтобы не тонуть в координатах.
        auto free = [&](int tx, int ty) {
            return lvl.at(tx, ty) == '.';
        };
        for (int tx = 3; tx < lvl.w - 3; ++tx) {
            if (tx % 11 == 4 && free(tx, 2)) D(tx, 2, DEC_WINDOW);
            if (tx % 18 == 1 && free(tx, 5)) D(tx, 5, DEC_PAINTING);
            if (tx % 7 == 2 && free(tx, 6)) D(tx, 6, DEC_SCONCE);
            if (tx % 13 == 6) D(tx, 0, DEC_CHANDELIER);
            if (tx % 16 == 3) D(tx, 8, DEC_CHANDELIER);
            if (tx % 12 == 6 && free(tx, 9)) D(tx, 9, DEC_WINDOW);
            if (tx % 20 == 3 && free(tx, 10)) D(tx, 10, DEC_PAINTING);
            if (tx % 8 == 2 && free(tx, 11)) D(tx, 11, DEC_SCONCE);
            if (tx % 15 == 7 && free(tx, 12)) D(tx, 12, DEC_SHELF);
            if (tx % 23 == 9 && free(tx, 12)) D(tx, 12, DEC_CLOCK);
            if (tx % 19 == 8 && free(tx, 5)) D(tx, 5, DEC_MIRROR);
        }
    }

    // Три этажа: базовый декор второго этажа повторяем вниз на 17 и 34 ряда.
    {
        const size_t n = decor.size();
        // первый этаж — полная копия
        for (size_t i = 0; i < n; ++i)
            decor.push_back(Decor{decor[i].tx, decor[i].ty + 17, decor[i].type});
        // подвал — без окон, портретов и зеркал
        if (levelIndex == 0) {
            for (size_t i = 0; i < n; ++i) {
                int t = decor[i].type;
                if (t == DEC_WINDOW || t == DEC_PAINTING || t == DEC_MIRROR) continue;
                decor.push_back(Decor{decor[i].tx, decor[i].ty + 34, t});
            }
        }
    }

    // Декор не должен накладываться на ступени, лестницы, дверь, кубки и
    // другие украшения. У каждого элемента считаем реальную рамку рисунка
    // в пикселях и оставляем только те, что ни с чем не пересекаются.
    auto decorPixels = [](const Decor& d) -> SDL_Rect {
        switch (d.type) {
            case DEC_WINDOW:     return SDL_Rect{d.tx * 16 - 3, d.ty * 16 - 2, 40, 57};
            case DEC_PAINTING:   return SDL_Rect{d.tx * 16 - 2, d.ty * 16 - 2, 32, 38};
            case DEC_SCONCE:     return SDL_Rect{d.tx * 16 + 4, d.ty * 16 - 2, 8, 18};
            case DEC_CHANDELIER: return SDL_Rect{d.tx * 16 - 6, d.ty * 16 + 16, 28, 19};
            case DEC_SHELF:      return SDL_Rect{d.tx * 16, d.ty * 16 - 32, 48, 48};
            case DEC_MIRROR:     return SDL_Rect{d.tx * 16 - 2, d.ty * 16, 26, 37};
            case DEC_CLOCK:      return SDL_Rect{d.tx * 16 - 2, d.ty * 16 - 2, 26, 50};
            default:             return SDL_Rect{d.tx * 16, d.ty * 16, 16, 16};
        }
    };
    auto hitRect = [](const SDL_Rect& a, const SDL_Rect& b) {
        return a.x < b.x + b.w && b.x < a.x + a.w &&
               a.y < b.y + b.h && b.y < a.y + a.h;
    };

    std::vector<Decor> keptDecor;
    std::vector<SDL_Rect> keptRects;
    for (const Decor& d : decor) {
        SDL_Rect r = decorPixels(d);
        bool blocked = false;

        // Ступени и лестницы.
        for (int ty = r.y / TILE; ty <= (r.y + r.h - 1) / TILE && !blocked; ++ty) {
            for (int tx = r.x / TILE; tx <= (r.x + r.w - 1) / TILE; ++tx) {
                char c = lvl.at(tx, ty);
                if (c == 'S' || c == 'L') {
                    blocked = true;
                    break;
                }
            }
        }

        // Дверь.
        if (!blocked && door.w > 0.0f) {
            SDL_Rect dr{static_cast<int>(door.x), static_cast<int>(door.y),
                        static_cast<int>(door.w), static_cast<int>(door.h)};
            if (hitRect(r, dr)) blocked = true;
        }

        // Кубки.
        for (const CupItem& c : cups) {
            if (blocked) break;
            SDL_Rect cr{c.tx * 16, c.ty * 16, 16, 16};
            if (hitRect(r, cr)) blocked = true;
        }

        // Другой декор.
        for (const SDL_Rect& q : keptRects) {
            if (hitRect(r, q)) {
                blocked = true;
                break;
            }
        }

        if (!blocked) {
            keptDecor.push_back(d);
            keptRects.push_back(r);
        }
    }
    decor.swap(keptDecor);

    // ковры
    rugs.clear();
    if (levelIndex == 0) {
        rugs.push_back(RectF{2 * 16.f, 240.f, 12 * 16.f, 6.f});
        rugs.push_back(RectF{61 * 16.f, 240.f, 11 * 16.f, 6.f});
        rugs.push_back(RectF{138 * 16.f, 240.f, 17 * 16.f, 6.f});
        rugs.push_back(RectF{6 * 16.f, 128.f, 7 * 16.f, 5.f});
        rugs.push_back(RectF{56 * 16.f, 128.f, 7 * 16.f, 5.f});
        rugs.push_back(RectF{115 * 16.f, 128.f, 8 * 16.f, 5.f});
    } else {
        const int groundRugs[] = {4, 44, 76, 124};
        for (int x : groundRugs) rugs.push_back(RectF{x * 16.f, 240.f, 9 * 16.f, 6.f});
        const int upperRugs[] = {6, 36, 80, 120};
        for (int x : upperRugs) rugs.push_back(RectF{x * 16.f, 128.f, 8 * 16.f, 5.f});
    }

    // ковры: этажи повторяются со сдвигом 272 px, подвал — только напольные
    {
        const size_t n = rugs.size();
        for (size_t i = 0; i < n; ++i)
            rugs.push_back(RectF{rugs[i].x, rugs[i].y + 272.0f, rugs[i].w, rugs[i].h});
        if (levelIndex == 0) {
            for (size_t i = 0; i < n; ++i)
                if (rugs[i].y == 240.0f)
                    rugs.push_back(RectF{rugs[i].x, rugs[i].y + 544.0f, rugs[i].w, rugs[i].h});
        }
    }

    // монстры
    monsters.clear();
    for (int ty = 0; ty < lvl.h; ++ty) {
        for (int tx = 0; tx < lvl.w; ++tx) {
            char c = lvl.at(tx, ty);
            if (c != 'z' && c != 'v' && c != 'n') continue;
            Monster m;
            m.type = (c == 'z') ? 0 : ((c == 'v') ? 1 : 2);
            m.seed = rndf(rngState, 0.0f, 6.28f);
            m.b.facing = (hash2(tx, ty) & 1u) ? 1 : -1;
            if (m.type == 0) {
                // паук: в полтора раза выше прежнего, юркий; живёт на потолке
                // своей комнаты и спускается к Коляну на паутине.
                m.b.w = 16;
                m.b.h = 18;
                m.b.x = tx * 16.0f;
                int ceilRow = ty - 1;
                while (ceilRow >= 0 && !lvl.solid(tx, ceilRow)) --ceilRow;
                m.b.y = (ceilRow >= 0) ? (ceilRow + 1) * 16.0f + 4.0f : 4.0f;
                m.ceilingY = m.b.y;
                m.webAnchorX = m.b.x + m.b.w * 0.5f;
                m.webAnchorY = m.b.y;
                m.hp = m.maxHp = 3;
            } else {
                // зомби и метатель ножей: высокие, ищут пол под своей клеткой
                m.b.w = 14;
                m.b.h = 26;
                m.b.x = tx * 16.0f + 1.0f;
                int floorRow = ty;
                while (floorRow < lvl.h && !lvl.solid(tx, floorRow)) ++floorRow;
                m.b.y = (floorRow < lvl.h) ? (floorRow * 16.0f - m.b.h)
                                           : (ty * 16.0f + 16.0f - m.b.h);
                if (m.type == 1) {
                    m.hp = m.maxHp = 4;
                } else {
                    m.hp = m.maxHp = 3;
                    m.throwCd = rndf(rngState, 0.5f, 2.0f);
                }
            }
            m.homeX = m.b.x;
            m.homeY = m.b.y;
            monsters.push_back(m);
        }
    }

    mag = MAG_SIZE;
    reloading = false;
    shootCd = reloadTimer = standStill = muzzle = gunKick = 0;
    reloadDuration = RELOAD_FULL;
    prevSpacePressed = false;
    prevSpacePressed = false;
    jumpEdge = false;

    parts.clear();
    tracers.clear();
    knives.clear();
    lights.clear();
    checkpoint = RectF{player.x, player.y, player.w, player.h};
    snapCamera();
    shake = hurtFlash = winDelay = 0;
    introTimer = 0;
    state = ST_PLAY;
}

void Game::startGame()
{
    levelIndex = 0;
    kills = 0;
    score = 0;
    lives = START_LIVES;
    deathTimer = 0;
    // Украшения каждый забег лежат по-новому.
    rngState ^= SDL_GetTicks();
    resetWorld(0);
    state = ST_PLAY;
}

void Game::assignBind(SDL_Scancode sc)
{
    if (bindTarget < 0 || bindTarget >= CA_COUNT) return;
    if (bindSlot < 0 || bindSlot > 1) return;
    if (sc == SDL_SCANCODE_UNKNOWN || sc == SDL_SCANCODE_ESCAPE) return;

    // Одинаковых клавиш в двух слотах одного действия не держим:
    // назначенная клавиша меняется местами со второй.
    int other = 1 - bindSlot;
    if (controls.keys[bindTarget][other] == sc)
        controls.keys[bindTarget][other] = controls.keys[bindTarget][bindSlot];
    controls.keys[bindTarget][bindSlot] = sc;
    bindTarget = -1;
    saveControls();
}

void Game::loadControls()
{
    controls.reset();
    std::ifstream f(configPath());
    if (!f) return;

    std::string magic;
    int version = 0;
    if (!(f >> magic >> version)) return;
    if (magic != "kolek-controls") return;

    auto valid = [](int v) {
        return v >= static_cast<int>(SDL_SCANCODE_UNKNOWN) &&
               v < SDL_NUM_SCANCODES &&
               v != static_cast<int>(SDL_SCANCODE_ESCAPE);
    };

    if (version == 1) {
        // Старый файл: по одной клавише на действие, включая отдельный прицел.
        int oldKeys[9] = {};
        for (int i = 0; i < 9; ++i)
            if (!(f >> oldKeys[i]) || !valid(oldKeys[i])) return;
        // Старые индексы: влево, вправо, вверх, вниз, прыжок, прицел↑,
        // прицел↓, выстрел, заново. Прицел теперь идёт от движения.
        const int map[CA_COUNT] = {0, 1, 2, 3, 4, 7, 8};
        for (int i = 0; i < CA_COUNT; ++i) {
            controls.keys[i][0] = static_cast<SDL_Scancode>(oldKeys[map[i]]);
            if (controls.keys[i][1] == controls.keys[i][0])
                controls.keys[i][1] = SDL_SCANCODE_UNKNOWN;
        }
        return;
    }
    if (version != 2) return;

    for (int i = 0; i < CA_COUNT; ++i) {
        int a = 0, b = 0;
        if (!(f >> a >> b)) return;
        if (!valid(a) || !valid(b)) return;
        if (a == b && a != static_cast<int>(SDL_SCANCODE_UNKNOWN)) return;
        controls.keys[i][0] = static_cast<SDL_Scancode>(a);
        controls.keys[i][1] = static_cast<SDL_Scancode>(b);
    }
}

void Game::saveControls() const
{
    if (!persistControls) return;

    std::ofstream f(configPath(), std::ios::trunc);
    if (!f) return;
    f << "kolek-controls 2\n";
    for (int i = 0; i < CA_COUNT; ++i) {
        if (i) f << ' ';
        f << static_cast<int>(controls.keys[i][0]) << ' '
          << static_cast<int>(controls.keys[i][1]);
    }
    f << '\n';
}

// ---------------------------------------------------------------------------
//  Обновление
// ---------------------------------------------------------------------------
void Game::update(float dt, const Input& in)
{
    // Нажатие прыжка считаем один раз за реальное нажатие: главный цикл
    // переиспользует один Input для нескольких подшагов физики.
    jumpEdge = in.spacePressed && !prevSpacePressed;
    prevSpacePressed = in.spacePressed;

    timeSec += dt;
    titleBlink += dt;
    updateParticles(dt);
    updateScorePopups(dt);
    updateTracers(dt);
    shake = std::max(0.0f, shake - dt * 1.9f);
    hurtFlash = std::max(0.0f, hurtFlash - dt * 2.4f);

    if (state == ST_TITLE) {
        if (in.menuUp)
            menuIndex = (menuIndex + 2) % 3;
        if (in.menuDown)
            menuIndex = (menuIndex + 1) % 3;
        if (in.back) {
            quitRequested = true;
            return;
        }
        if (in.confirm) {
            if (menuIndex == 0) {
                startGame();
                // Пробел, которым начали игру, не должен сразу превращаться в прыжок.
                jumpEdge = false;
                prevSpacePressed = true;
            } else if (menuIndex == 1) {
                state = ST_SETTINGS;
                settingsIndex = 0;
                settingsSlot = 0;
                bindTarget = -1;
            } else {
                quitRequested = true;
            }
        }
        return;
    }
    if (state == ST_SETTINGS) {
        const int rows = CA_COUNT + 1;
        if (bindTarget >= 0) return;
        if (in.menuUp)
            settingsIndex = (settingsIndex + rows - 1) % rows;
        if (in.menuDown)
            settingsIndex = (settingsIndex + 1) % rows;
        if (settingsIndex < CA_COUNT) {
            if (in.menuLeft) settingsSlot = 0;
            if (in.menuRight) settingsSlot = 1;
        }
        if (in.back) {
            state = ST_TITLE;
            menuIndex = 1;
            return;
        }
        if (in.confirm) {
            if (settingsIndex < CA_COUNT) {
                bindTarget = settingsIndex;
                bindSlot = settingsSlot;
            } else {
                state = ST_TITLE;
                menuIndex = 1;
            }
        }
        return;
    }
    if (state == ST_WIN || state == ST_GAMEOVER) {
        if (in.back) {
            state = ST_TITLE;
            menuIndex = 0;
            return;
        }
        if (in.restart) startGame();
        return;
    }
    if (state == ST_DYING) {
        if (in.back) {
            state = ST_TITLE;
            menuIndex = 0;
            return;
        }
        updateDeath(dt);
        return;
    }
    if (in.back) {
        state = ST_TITLE;
        menuIndex = 0;
        titleBlink = 0;
        return;
    }
    if (in.restart) {
        startGame();
        return;
    }

    updatePlayer(dt, in);
    updateWeapon(dt, in);
    updateMonsters(dt);
    updateKnives(dt);
    checkPickups();
    updateCheckpoint(dt);
    checkDoor(dt);
    updateCamera(dt);
    introTimer += dt;
}

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
    // Прицел вверх/вниз поворачивает дробовик на 45 градусов,
    // а каждая дробина уходит под своим углом внутри конуса разброса.
    float aimAng = player.lookY * 0.7853981634f;
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
    m.b.vx += (dirX >= 0.0f ? 1.0f : -1.0f) * 24.0f;

    if (m.hp <= 0) {
        m.dead = true;
        ++kills;
        score += KILL_SCORE[m.type];
        addScorePopup(KILL_SCORE[m.type], SDL_Color{255, 226, 150, 255});
        shake = std::max(shake, 0.30f);
        SDL_Color blood = (m.type == 0) ? SDL_Color{150, 40, 45, 255}
                                        : SDL_Color{90, 150, 60, 255};
        spawnSparkle(m.b.x + m.b.w * 0.5f, m.b.y + m.b.h * 0.5f, 18, blood);
        spawnSparkle(m.b.x + m.b.w * 0.5f, m.b.y + m.b.h * 0.5f, 8,
                     SDL_Color{255, 200, 90, 255});
    } else {
        spawnSparkle(m.b.x + m.b.w * 0.5f, m.b.y + m.b.h * 0.5f, 5,
                     (m.type == 0) ? SDL_Color{180, 60, 60, 255}
                                   : SDL_Color{120, 180, 70, 255});
    }
}

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
        bool holding = false;    // метатель стоит и целится

        if (m.type == 2) {
            // Метатель ножей: бросает, когда Колян на одной с ним высоте.
            bool inSight = state == ST_PLAY && player.invuln <= 0 &&
                           std::fabs(dyp) < 26.0f && std::fabs(dxp) < 300.0f;
            if (inSight) {
                dir = (dxp > 0.0f) ? 1 : -1;
                targetSpeed = 0.0f;
                holding = true;
                m.anim += dt * 2.0f;
                if (m.throwCd <= 0.0f) {
                    // Нож летит медленно и по прямой — от него легко уйти.
                    Knife k;
                    k.dir = dir;
                    k.x = b.x + b.w * 0.5f + dir * 12.0f;
                    k.y = b.y + 9.0f;
                    k.vx = dir * KNIFE_SPEED;
                    k.spin = rndf(rngState, 0.0f, 6.28f);
                    knives.push_back(k);
                    m.throwCd = rndf(rngState, 1.7f, 2.4f);
                    m.anim += 1.8f;
                    spawnSparkle(k.x, k.y, 3, SDL_Color{205, 215, 235, 170});
                }
            } else {
                // Медленный патруль в подвале.
                float off = b.x - m.homeX;
                if (off > 40.0f) dir = -1;
                else if (off < -40.0f) dir = 1;
                targetSpeed = 16.0f;
                m.anim += dt * 2.0f;
            }
        } else {
            // Преследование только если Колян совсем рядом — в двух шагах.
            bool chase = state == ST_PLAY && player.invuln <= 0 &&
                         std::fabs(dxp) <= CHASE_RADIUS && std::fabs(dyp) < 30.0f;
            if (chase) {
                dir = (dxp > 0.0f) ? 1 : -1;
                targetSpeed = (m.type == 0) ? 82.0f : 56.0f;
                m.anim += dt * ((m.type == 0) ? 12.0f : 7.0f);
            } else {
                // Патруль вокруг точки появления.
                float off = b.x - m.homeX;
                if (off > 48.0f) dir = -1;
                else if (off < -48.0f) dir = 1;
                targetSpeed = (m.type == 0) ? 30.0f : 20.0f;
                m.anim += dt * ((m.type == 0) ? 5.0f : 3.0f);
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
                                   : (m.type == 1 ? DEATH_ZOMBIE : DEATH_KNIFE));
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

void Game::updateKnives(float dt)
{
    std::vector<Knife> keep;
    keep.reserve(knives.size());
    for (Knife& k : knives) {
        k.x += k.vx * dt;
        k.spin += dt * 13.0f;

        // Нож врезается в стену (или ступень) — искры и вон.
        if (lvl.solidRect(k.x - 2.0f, k.y - 1.0f, 4.0f, 2.0f) ||
            lvl.stepRect(k.x - 2.0f, k.y - 1.0f, 4.0f, 2.0f)) {
            spawnSparkle(k.x, k.y, 5, SDL_Color{222, 205, 168, 220});
            continue;
        }

        if (state == ST_PLAY && player.invuln <= 0 &&
            overlapRect(k.x - 3.0f, k.y - 2.5f, 6.0f, 5.0f,
                        player.x, player.y, player.w, player.h)) {
            killPlayer(DEATH_KNIFE);   // сам нож и остальные снимаются смертью
            return;
        }

        keep.push_back(k);
    }
    knives.swap(keep);
}

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

void Game::updateCheckpoint(float dt)
{
    if (player.grounded && player.hurt <= 0 && player.invuln <= 0 && !player.climbing) {
        player.safeTimer += dt;
        if (player.safeTimer > 0.35f) {
            checkpoint = RectF{player.x, player.y, player.w, player.h};
            player.safeTimer = 0;
        }
    } else {
        player.safeTimer = 0;
    }
}

void Game::updateCamera(float dt)
{
    float look = clampf(player.vx * 0.30f, -34.0f, 34.0f);
    float targetX = player.x + player.w * 0.5f + look - VIEW_W * 0.5f;
    targetX = clampf(targetX, 0.0f, std::max(0.0f, static_cast<float>(lvl.w * TILE - VIEW_W)));

    // Вертикальная камера ездит по всей высоте здания.
    float lookYOff = player.lookY * 24.0f + clampf(player.vy * 0.10f, -22.0f, 22.0f);
    float targetY = player.y + player.h * 0.5f + lookYOff - VIEW_H * 0.5f;
    targetY = clampf(targetY, 0.0f, std::max(0.0f, static_cast<float>(lvl.h * TILE - VIEW_H)));

    camX += (targetX - camX) * std::min(1.0f, dt * 9.0f);
    camY += (targetY - camY) * std::min(1.0f, dt * 8.0f);
}

void Game::snapCamera()
{
    camX = clampf(player.x + player.w * 0.5f - VIEW_W * 0.5f, 0.0f,
                  std::max(0.0f, static_cast<float>(lvl.w * TILE - VIEW_W)));
    camY = clampf(player.y + player.h * 0.5f - VIEW_H * 0.5f, 0.0f,
                  std::max(0.0f, static_cast<float>(lvl.h * TILE - VIEW_H)));
}

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
        int next = levelIndex + 1;
        resetWorld(next);
        spawnSparkle(player.x + player.w * 0.5f, player.y + player.h * 0.5f, 26,
                     SDL_Color{255, 220, 130, 255});
        shake = 0.25f;
    } else {
        spawnSparkle(door.x + door.w * 0.5f, door.y + door.h * 0.5f, 40,
                     SDL_Color{255, 225, 130, 255});
        state = ST_WIN;
        spawnConfetti();
    }
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
    SDL_Color blood = (cause == DEATH_ZOMBIE) ? SDL_Color{120, 200, 90, 255}
                                              : SDL_Color{210, 70, 70, 255};
    spawnSparkle(player.x + player.w * 0.5f, player.y + player.h * 0.5f, 22, blood);
    knives.clear();
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

// ---------------------------------------------------------------------------
//  Отрисовка: общий каркас и фон
// ---------------------------------------------------------------------------
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
        drawKnives();
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

void Game::drawWallpaper()
{
    int tx0 = std::max(0, static_cast<int>(camX / TILE) - 1);
    int tx1 = std::min(lvl.w - 1, static_cast<int>((camX + VIEW_W) / TILE) + 1);

    // Обои повторяются на каждом этаже высотой 272 px.
    for (float base = 0; base < lvl.h * TILE; base += 272.0f) {
        if (base + 272.0f < camY - 8.0f || base > camY + VIEW_H + 8.0f) continue;

        for (int tx = tx0; tx <= tx1; ++tx) {
            int band = ((tx / 24) % 7 + 7) % 7;
            SDL_Color baseCol = wallPalette[band];
            fillW(tx * 16.0f, base + 16.0f, 16.0f, 112.0f, baseCol);
            fillW(tx * 16.0f, base + 144.0f, 16.0f, 96.0f, shade(baseCol, 0.70f));

            // обои в тонкую полоску
            fillW(tx * 16.0f + 14.0f, base + 16.0f, 2.0f, 112.0f, shade(baseCol, 0.86f));
            fillW(tx * 16.0f + 14.0f, base + 144.0f, 2.0f, 96.0f, shade(baseCol, 0.60f));

            // панели-вагонка
            fillW(tx * 16.0f, base + 120.0f, 16.0f, 8.0f, SDL_Color{52, 34, 28, 255});
            fillW(tx * 16.0f, base + 232.0f, 16.0f, 8.0f, SDL_Color{44, 30, 26, 255});
            fillW(tx * 16.0f + 8.0f, base + 120.0f, 1.0f, 8.0f, SDL_Color{76, 52, 36, 255});
            fillW(tx * 16.0f + 8.0f, base + 232.0f, 1.0f, 8.0f, SDL_Color{66, 46, 32, 255});
        }

        // карнизы и плинтусы
        fillW(camX, base + 16.0f, static_cast<float>(VIEW_W), 3.0f, SDL_Color{70, 48, 34, 255});
        fillW(camX, base + 19.0f, static_cast<float>(VIEW_W), 1.0f, SDL_Color{112, 82, 50, 120});
        fillW(camX, base + 144.0f, static_cast<float>(VIEW_W), 3.0f, SDL_Color{58, 40, 30, 255});
        fillW(camX, base + 147.0f, static_cast<float>(VIEW_W), 1.0f, SDL_Color{94, 68, 44, 120});
        fillW(camX, base + 126.0f, static_cast<float>(VIEW_W), 2.0f, SDL_Color{36, 24, 22, 255});
        fillW(camX, base + 238.0f, static_cast<float>(VIEW_W), 2.0f, SDL_Color{30, 20, 18, 255});

        // мягкое затемнение у пола
        for (int i = 0; i < 6; ++i) {
            fillW(camX, base + 96.0f + i * 5.0f, static_cast<float>(VIEW_W), 5.0f,
                  SDL_Color{10, 6, 10, static_cast<Uint8>(6 + i * 3)});
            fillW(camX, base + 202.0f + i * 5.0f, static_cast<float>(VIEW_W), 5.0f,
                  SDL_Color{8, 5, 8, static_cast<Uint8>(10 + i * 4)});
        }
    }
}

// ---------------------------------------------------------------------------
//  Тайлы
// ---------------------------------------------------------------------------
void Game::drawTiles()
{
    int tx0 = std::max(0, static_cast<int>(camX / TILE) - 1);
    int tx1 = std::min(lvl.w - 1, static_cast<int>((camX + VIEW_W) / TILE) + 1);
    for (int ty = 0; ty < lvl.h; ++ty) {
        int y = sy(ty * 16.0f);
        if (y <= -20 || y >= VIEW_H + 20) continue;
        for (int tx = tx0; tx <= tx1; ++tx) {
            int x = sx(tx * 16.0f);
            switch (lvl.at(tx, ty)) {
                case '#': drawBrick(x, y, tx, ty); break;
                case '=': drawFloorTile(x, y, tx, ty); break;
                case 't': drawTableTile(x, y, tx, ty); break;
                case 'S': drawStepTile(x, y, tx, ty); break;
                case 'L': drawLadder(x, y); break;
                case '.': drawPitTile(x, y, tx, ty); break;
                default: break;
            }
        }
    }
}

void Game::drawBrick(int x, int y, int tx, int ty)
{
    uint32_t h = hash2(tx, ty);
    int v = static_cast<int>(h % 12);
    SDL_Color base{static_cast<Uint8>(58 + v), static_cast<Uint8>(39 + v / 2),
                   static_cast<Uint8>(43 + v / 3), 255};
    SDL_SetRenderDrawColor(ren, base.r, base.g, base.b, 255);
    SDL_Rect r{x, y, 16, 16};
    SDL_RenderFillRect(ren, &r);

    SDL_SetRenderDrawColor(ren, 36, 24, 29, 255);
    SDL_RenderDrawLine(ren, x, y + 7, x + 15, y + 7);
    SDL_RenderDrawLine(ren, x, y + 15, x + 15, y + 15);
    int off = (ty % 2 == 0) ? 8 : 0;
    SDL_RenderDrawLine(ren, x + off, y, x + off, y + 6);
    SDL_RenderDrawLine(ren, x + (off == 8 ? 0 : 8), y + 8, x + (off == 8 ? 0 : 8), y + 14);

    SDL_SetRenderDrawColor(ren, base.r + 18, base.g + 12, base.b + 10, 90);
    SDL_RenderDrawLine(ren, x + 1, y + 1, x + 14, y + 1);
}

void Game::drawFloorTile(int x, int y, int tx, int ty)
{
    (void)tx;
    SDL_SetRenderDrawColor(ren, 62, 40, 29, 255);
    SDL_Rect r{x, y, 16, 16};
    SDL_RenderFillRect(ren, &r);
    SDL_SetRenderDrawColor(ren, 112, 74, 44, 255);
    SDL_Rect top{x, y, 16, 4};
    SDL_RenderFillRect(ren, &top);
    SDL_SetRenderDrawColor(ren, 146, 100, 58, 255);
    SDL_Rect shine{x, y, 16, 1};
    SDL_RenderFillRect(ren, &shine);

    SDL_SetRenderDrawColor(ren, 44, 28, 22, 255);
    int off = (ty % 2 == 0) ? 4 : 12;
    SDL_RenderDrawLine(ren, x + off, y + 4, x + off, y + 15);
    SDL_SetRenderDrawColor(ren, 24, 16, 14, 150);
    SDL_Rect bottom{x, y + 13, 16, 3};
    SDL_RenderFillRect(ren, &bottom);
}

void Game::drawTableTile(int x, int y, int tx, int ty)
{
    SDL_SetRenderDrawColor(ren, 76, 47, 30, 255);
    SDL_Rect r{x, y, 16, 16};
    SDL_RenderFillRect(ren, &r);
    if (lvl.at(tx, ty - 1) != 't') {
        SDL_SetRenderDrawColor(ren, 138, 88, 48, 255);
        SDL_Rect top{x, y, 16, 5};
        SDL_RenderFillRect(ren, &top);
        SDL_SetRenderDrawColor(ren, 176, 120, 68, 255);
        SDL_Rect line{x, y, 16, 1};
        SDL_RenderFillRect(ren, &line);
    }
    if (lvl.at(tx, ty + 1) != 't') {
        SDL_SetRenderDrawColor(ren, 50, 30, 22, 255);
        SDL_Rect shadow{x, y + 13, 16, 3};
        SDL_RenderFillRect(ren, &shadow);
        SDL_SetRenderDrawColor(ren, 60, 38, 26, 255);
        SDL_Rect legL{x + 2, y + 5, 3, 11};
        SDL_Rect legR{x + 11, y + 5, 3, 11};
        SDL_RenderFillRect(ren, &legL);
        SDL_RenderFillRect(ren, &legR);
    }
    SDL_SetRenderDrawColor(ren, 102, 64, 38, 120);
    SDL_Rect grain{x + 3, y + 8, 10, 1};
    SDL_RenderFillRect(ren, &grain);
}

void Game::drawStepTile(int x, int y, int tx, int ty)
{
    (void)ty;
    // Широкая деревянная ступень: доска у верхней кромки тайла.
    SDL_SetRenderDrawColor(ren, 92, 58, 34, 255);
    SDL_Rect board{x, y, 16, 7};
    SDL_RenderFillRect(ren, &board);
    SDL_SetRenderDrawColor(ren, 150, 100, 58, 255);
    SDL_Rect topEdge{x, y, 16, 2};
    SDL_RenderFillRect(ren, &topEdge);
    SDL_SetRenderDrawColor(ren, 60, 38, 24, 255);
    SDL_Rect lowEdge{x, y + 6, 16, 2};
    SDL_RenderFillRect(ren, &lowEdge);

    // Торец с одной стороны ступени — как у настоящей лестницы.
    if (lvl.at(tx - 1, ty) != 'S') {
        SDL_SetRenderDrawColor(ren, 44, 28, 18, 255);
        SDL_Rect cap{x, y, 2, 8};
        SDL_RenderFillRect(ren, &cap);
    }
    if (lvl.at(tx + 1, ty) != 'S') {
        SDL_SetRenderDrawColor(ren, 44, 28, 18, 255);
        SDL_Rect cap{x + 14, y, 2, 8};
        SDL_RenderFillRect(ren, &cap);
    }
    if (lvl.at(tx, ty - 1) == 'S' || lvl.at(tx, ty + 1) == 'S') {
        SDL_SetRenderDrawColor(ren, 74, 46, 28, 210);
        SDL_Rect grain{x + 3, y + 3, 10, 1};
        SDL_RenderFillRect(ren, &grain);
    }
}

void Game::drawLadder(int x, int y)
{
    SDL_SetRenderDrawColor(ren, 132, 92, 50, 255);
    SDL_Rect railL{x + 4, y, 2, 16};
    SDL_Rect railR{x + 11, y, 2, 16};
    SDL_RenderFillRect(ren, &railL);
    SDL_RenderFillRect(ren, &railR);
    SDL_SetRenderDrawColor(ren, 174, 126, 70, 255);
    SDL_Rect rung1{x + 4, y + 3, 9, 2};
    SDL_Rect rung2{x + 4, y + 10, 9, 2};
    SDL_RenderFillRect(ren, &rung1);
    SDL_RenderFillRect(ren, &rung2);
    SDL_SetRenderDrawColor(ren, 70, 44, 24, 120);
    SDL_Rect shadeL{x + 12, y, 1, 16};
    SDL_RenderFillRect(ren, &shadeL);
}

void Game::drawPitTile(int x, int y, int tx, int ty)
{
    bool edgeL = (tx > 0 && lvl.at(tx - 1, ty) == '=');
    bool edgeR = (tx + 1 < lvl.w && lvl.at(tx + 1, ty) == '=');
    if (!edgeL && !edgeR) return;

    // Провал в полу: тёмный колодец с обломанными краями.
    SDL_SetRenderDrawColor(ren, 20, 14, 18, 255);
    SDL_Rect top{x, y, 16, 5};
    SDL_RenderFillRect(ren, &top);
    SDL_SetRenderDrawColor(ren, 13, 9, 12, 255);
    SDL_Rect mid{x, y + 5, 16, 5};
    SDL_RenderFillRect(ren, &mid);
    SDL_SetRenderDrawColor(ren, 6, 4, 7, 255);
    SDL_Rect deep{x, y + 10, 16, 6};
    SDL_RenderFillRect(ren, &deep);

    // тусклые кирпичи на дне
    SDL_SetRenderDrawColor(ren, 42, 28, 24, 255);
    SDL_Rect b1{x + 3, y + 12, 4, 2};
    SDL_Rect b2{x + 10, y + 13, 3, 2};
    SDL_RenderFillRect(ren, &b1);
    SDL_RenderFillRect(ren, &b2);

    // обломанные доски по краям
    SDL_SetRenderDrawColor(ren, 104, 66, 38, 255);
    if (edgeL) {
        SDL_Rect e{x, y, 2, 4};
        SDL_Rect e2{x + 2, y, 1, 2};
        SDL_RenderFillRect(ren, &e);
        SDL_RenderFillRect(ren, &e2);
    }
    if (edgeR) {
        SDL_Rect e{x + 14, y, 2, 4};
        SDL_Rect e2{x + 13, y, 1, 2};
        SDL_RenderFillRect(ren, &e);
        SDL_RenderFillRect(ren, &e2);
    }
}

void Game::drawRugs()
{
    for (const RectF& rug : rugs) {
        fillW(rug.x, rug.y, rug.w, rug.h, SDL_Color{108, 30, 38, 235});
        fillW(rug.x + 2, rug.y + 1, rug.w - 4, rug.h - 2, SDL_Color{132, 42, 48, 235});
        fillW(rug.x + 3, rug.y + 2, rug.w - 6, 1, SDL_Color{214, 168, 86, 160});
        fillW(rug.x, rug.y, rug.w, 1, SDL_Color{196, 152, 80, 190});
        for (float px = rug.x + 12; px < rug.x + rug.w - 8; px += 22) {
            fillW(px, rug.y + 2, 2, 2, SDL_Color{198, 160, 88, 120});
        }
    }
}

// ---------------------------------------------------------------------------
//  Декор особняка
// ---------------------------------------------------------------------------
void Game::drawDecor()
{
    for (const Decor& d : decor) {
        float wx = d.tx * 16.0f;
        float wy = d.ty * 16.0f;
        if (wx < camX - 80 || wx > camX + VIEW_W + 80) continue;
        if (wy < camY - 120 || wy > camY + VIEW_H + 80) continue;
        switch (d.type) {
            case DEC_WINDOW:      drawWindow(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_PAINTING:    drawPainting(static_cast<int>(wx), static_cast<int>(wy),
                                               (d.tx * 7 + d.ty) % 3); break;
            case DEC_SCONCE:      drawSconce(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_CHANDELIER:  drawChandelier(static_cast<int>(wx) + 8,
                                                 static_cast<int>(wy) + 16); break;
            case DEC_SHELF:       drawShelf(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_MIRROR:      drawMirror(static_cast<int>(wx), static_cast<int>(wy)); break;
            case DEC_CLOCK:       drawClock(static_cast<int>(wx), static_cast<int>(wy)); break;
            default: break;
        }
    }
}

void Game::drawWindow(int wx, int wy)
{
    int x = sx(static_cast<float>(wx)) + 2;
    int y = sy(static_cast<float>(wy));
    if (x < -48 || x > VIEW_W + 16) return;

    // рама
    SDL_SetRenderDrawColor(ren, 48, 32, 24, 255);
    SDL_Rect frame{x - 3, y - 2, 36, 52};
    SDL_RenderFillRect(ren, &frame);
    SDL_SetRenderDrawColor(ren, 94, 64, 40, 255);
    SDL_Rect innerFrame{x - 1, y, 32, 48};
    SDL_RenderFillRect(ren, &innerFrame);

    // ночное небо
    SDL_SetRenderDrawColor(ren, 18, 24, 52, 255);
    SDL_Rect sky{x, y, 30, 46};
    SDL_RenderFillRect(ren, &sky);
    SDL_SetRenderDrawColor(ren, 32, 40, 78, 255);
    SDL_Rect sky2{x, y + 28, 30, 18};
    SDL_RenderFillRect(ren, &sky2);
    SDL_SetRenderDrawColor(ren, 54, 58, 96, 255);
    SDL_Rect sky3{x, y + 38, 30, 8};
    SDL_RenderFillRect(ren, &sky3);

    // луна и звёзды
    fillCircle(x + 21, y + 12, 4, SDL_Color{236, 232, 198, 235});
    fillCircle(x + 23, y + 10, 3, SDL_Color{24, 32, 68, 255});
    SDL_SetRenderDrawColor(ren, 220, 226, 245, 220);
    SDL_Rect st1{x + 6, y + 8, 1, 1};
    SDL_Rect st2{x + 12, y + 16, 1, 1};
    SDL_Rect st3{x + 25, y + 23, 1, 1};
    SDL_RenderFillRect(ren, &st1);
    SDL_RenderFillRect(ren, &st2);
    SDL_RenderFillRect(ren, &st3);

    // силуэт деревьев
    SDL_SetRenderDrawColor(ren, 12, 18, 24, 255);
    fillTriangle(x + 2, y + 46, x + 8, y + 32, x + 14, y + 46, SDL_Color{12, 18, 24, 255});
    fillTriangle(x + 12, y + 46, x + 20, y + 30, x + 27, y + 46, SDL_Color{10, 16, 22, 255});

    // переплёт
    SDL_SetRenderDrawColor(ren, 74, 50, 32, 255);
    SDL_Rect crossV{x + 14, y, 2, 46};
    SDL_Rect crossH{x, y + 22, 30, 2};
    SDL_RenderFillRect(ren, &crossV);
    SDL_RenderFillRect(ren, &crossH);

    // шторы
    SDL_SetRenderDrawColor(ren, 104, 32, 40, 255);
    SDL_Rect curtL{x - 3, y - 1, 6, 47};
    SDL_Rect curtR{x + 27, y - 1, 6, 47};
    SDL_RenderFillRect(ren, &curtL);
    SDL_RenderFillRect(ren, &curtR);
    SDL_SetRenderDrawColor(ren, 138, 48, 52, 255);
    SDL_Rect curtShine{x - 2, y, 1, 45};
    SDL_RenderFillRect(ren, &curtShine);

    // подоконник
    SDL_SetRenderDrawColor(ren, 120, 84, 52, 255);
    SDL_Rect sill{x - 5, y + 48, 40, 4};
    SDL_RenderFillRect(ren, &sill);
    SDL_SetRenderDrawColor(ren, 66, 44, 30, 255);
    SDL_Rect sillShade{x - 5, y + 52, 40, 2};
    SDL_RenderFillRect(ren, &sillShade);

    addLight(wx + 17.0f, wy + 24.0f, 52.0f, SDL_Color{120, 160, 255, 255}, 26.0f);
}

void Game::drawPainting(int wx, int wy, int variant)
{
    int x = sx(static_cast<float>(wx));
    int y = sy(static_cast<float>(wy));
    if (x < -40 || x > VIEW_W + 20) return;

    SDL_SetRenderDrawColor(ren, 74, 50, 26, 255);
    SDL_Rect outer{x - 2, y - 2, 32, 38};
    SDL_RenderFillRect(ren, &outer);
    SDL_SetRenderDrawColor(ren, 172, 132, 62, 255);
    SDL_Rect gold{x, y, 28, 34};
    SDL_RenderFillRect(ren, &gold);
    SDL_SetRenderDrawColor(ren, 26, 18, 22, 255);
    SDL_Rect canvas{x + 3, y + 3, 22, 28};
    SDL_RenderFillRect(ren, &canvas);

    if (variant == 0) {
        // портрет предка
        fillCircle(x + 14, y + 12, 5, SDL_Color{196, 164, 132, 255});
        SDL_SetRenderDrawColor(ren, 56, 40, 44, 255);
        SDL_Rect body{x + 6, y + 18, 16, 11};
        SDL_RenderFillRect(ren, &body);
        SDL_SetRenderDrawColor(ren, 232, 210, 176, 255);
        SDL_Rect face{x + 12, y + 10, 4, 4};
        SDL_RenderFillRect(ren, &face);
    } else if (variant == 1) {
        // ночной пейзаж
        SDL_SetRenderDrawColor(ren, 34, 44, 66, 255);
        SDL_Rect sky{x + 3, y + 3, 22, 28};
        SDL_RenderFillRect(ren, &sky);
        fillCircle(x + 18, y + 9, 3, SDL_Color{220, 220, 198, 255});
        fillTriangle(x + 2, y + 28, x + 10, y + 14, x + 18, y + 28,
                     SDL_Color{20, 28, 34, 255});
        fillTriangle(x + 11, y + 28, x + 18, y + 18, x + 26, y + 28,
                     SDL_Color{16, 24, 30, 255});
    } else {
        // цветы
        SDL_SetRenderDrawColor(ren, 52, 66, 50, 255);
        SDL_Rect vase{x + 10, y + 20, 9, 8};
        SDL_RenderFillRect(ren, &vase);
        SDL_SetRenderDrawColor(ren, 132, 170, 108, 255);
        SDL_Rect stem1{x + 13, y + 12, 1, 9};
        SDL_Rect stem2{x + 16, y + 14, 1, 7};
        SDL_RenderFillRect(ren, &stem1);
        SDL_RenderFillRect(ren, &stem2);
        fillCircle(x + 13, y + 11, 2, SDL_Color{196, 84, 92, 255});
        fillCircle(x + 17, y + 13, 2, SDL_Color{226, 196, 96, 255});
    }

    SDL_SetRenderDrawColor(ren, 214, 178, 96, 150);
    SDL_Rect glare{x + 1, y + 1, 1, 32};
    SDL_RenderFillRect(ren, &glare);
}

void Game::drawSconce(int wx, int wy)
{
    int x = sx(static_cast<float>(wx)) + 8;
    int y = sy(static_cast<float>(wy));
    if (x < -24 || x > VIEW_W + 24) return;

    float flick = 0.75f + 0.25f * std::sin(timeSec * 8.0f + wx * 0.8f) *
                                   std::sin(timeSec * 13.0f + wx * 0.31f);
    SDL_SetRenderDrawColor(ren, 52, 40, 30, 255);
    SDL_Rect bracket{x - 4, y + 14, 8, 2};
    SDL_Rect stem{x - 1, y + 10, 2, 6};
    SDL_RenderFillRect(ren, &bracket);
    SDL_RenderFillRect(ren, &stem);
    SDL_SetRenderDrawColor(ren, 226, 208, 160, 255);
    SDL_Rect candle{x - 2, y + 2, 4, 9};
    SDL_RenderFillRect(ren, &candle);
    SDL_SetRenderDrawColor(ren, 168, 136, 80, 255);
    SDL_Rect holder{x - 3, y + 10, 6, 2};
    SDL_RenderFillRect(ren, &holder);

    SDL_SetRenderDrawColor(ren, 255, 170, 60, 255);
    SDL_Rect flame{x - 1, y - 2 + static_cast<int>(flick * 1.5f), 2, 4};
    SDL_RenderFillRect(ren, &flame);
    SDL_SetRenderDrawColor(ren, 255, 240, 170, 255);
    SDL_Rect flameCore{x - 1, y + 1, 1, 2};
    SDL_RenderFillRect(ren, &flameCore);

    addLight(wx + 8.0f, wy + 6.0f, 58.0f, SDL_Color{255, 180, 90, 255},
             70.0f * flick);
    addLight(wx + 8.0f, wy + 10.0f, 18.0f, SDL_Color{255, 230, 160, 255},
             90.0f * flick);
}

void Game::drawChandelier(int wx, int wy)
{
    int x = sx(static_cast<float>(wx));
    int y = sy(static_cast<float>(wy));
    if (x < -48 || x > VIEW_W + 48) return;

    float flick = 0.82f + 0.18f * std::sin(timeSec * 5.5f + wx * 0.4f);
    SDL_SetRenderDrawColor(ren, 96, 72, 34, 255);
    SDL_Rect chain{x - 1, y, 2, 9};
    SDL_RenderFillRect(ren, &chain);

    SDL_SetRenderDrawColor(ren, 176, 132, 54, 255);
    SDL_Rect bar{x - 14, y + 9, 28, 3};
    SDL_RenderFillRect(ren, &bar);
    fillCircle(x, y + 9, 3, SDL_Color{206, 162, 70, 255});

    for (int i = -1; i <= 1; ++i) {
        int cx = x + i * 10;
        SDL_SetRenderDrawColor(ren, 118, 86, 38, 255);
        SDL_Rect stem{cx - 1, y + 6, 2, 6};
        SDL_RenderFillRect(ren, &stem);
        SDL_SetRenderDrawColor(ren, 232, 214, 168, 255);
        SDL_Rect candle{cx - 2, y + 12, 4, 7};
        SDL_RenderFillRect(ren, &candle);
        SDL_SetRenderDrawColor(ren, 255, 176, 60, 255);
        SDL_Rect flame{cx - 1, y + 8 + static_cast<int>(flick * 1.5f), 2, 4};
        SDL_RenderFillRect(ren, &flame);
    }

    SDL_SetRenderDrawColor(ren, 214, 172, 84, 200);
    SDL_Rect shine{x - 12, y + 9, 8, 1};
    SDL_RenderFillRect(ren, &shine);

    addLight(wx, wy + 18.0f, 108.0f, SDL_Color{255, 190, 100, 255}, 60.0f * flick);
    addLight(wx, wy + 18.0f, 34.0f, SDL_Color{255, 230, 170, 255}, 85.0f * flick);
}

void Game::drawShelf(int wx, int wy)
{
    int x = sx(static_cast<float>(wx));
    // Шкаф — подвесной: поднимаем его над головой Коляна, чтобы он не
    // «застревал» в шкафу и мог свободно проходить под ним.
    int y = sy(static_cast<float>(wy)) - 32;
    if (x < -56 || x > VIEW_W + 16) return;

    SDL_SetRenderDrawColor(ren, 48, 30, 22, 255);
    SDL_Rect body{x, y, 48, 48};
    SDL_RenderFillRect(ren, &body);
    SDL_SetRenderDrawColor(ren, 82, 52, 32, 255);
    SDL_Rect inner{x + 3, y + 3, 42, 42};
    SDL_RenderFillRect(ren, &inner);

    for (int shelf = 0; shelf < 3; ++shelf) {
        int sy0 = y + 8 + shelf * 13;
        SDL_SetRenderDrawColor(ren, 42, 26, 18, 255);
        SDL_Rect shelfLine{x + 3, sy0 + 8, 42, 2};
        SDL_RenderFillRect(ren, &shelfLine);
        for (int b = 0; b < 7; ++b) {
            uint32_t h = hash2(wx + b, wy + shelf);
            int bw = 3 + static_cast<int>(h % 3);
            SDL_Color cols[5] = {
                {126, 48, 48, 255}, {52, 86, 62, 255}, {60, 66, 112, 255},
                {132, 102, 52, 255}, {92, 58, 84, 255}
            };
            SDL_Color c = cols[(h >> 4) % 5];
            SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, 255);
            SDL_Rect book{x + 5 + b * 5, sy0 + 1, bw, 7};
            SDL_RenderFillRect(ren, &book);
        }
    }
    SDL_SetRenderDrawColor(ren, 110, 76, 44, 255);
    SDL_Rect top{x, y, 48, 3};
    SDL_RenderFillRect(ren, &top);
}

void Game::drawMirror(int wx, int wy)
{
    int x = sx(static_cast<float>(wx));
    int y = sy(static_cast<float>(wy));
    if (x < -32 || x > VIEW_W + 16) return;

    SDL_SetRenderDrawColor(ren, 64, 44, 24, 255);
    SDL_Rect frame{x, y, 22, 34};
    SDL_RenderFillRect(ren, &frame);
    SDL_SetRenderDrawColor(ren, 168, 128, 62, 255);
    SDL_Rect gold{x + 2, y + 2, 18, 30};
    SDL_RenderFillRect(ren, &gold);
    SDL_SetRenderDrawColor(ren, 60, 82, 102, 255);
    SDL_Rect glass{x + 4, y + 4, 14, 26};
    SDL_RenderFillRect(ren, &glass);
    SDL_SetRenderDrawColor(ren, 108, 142, 168, 190);
    SDL_Rect shine{x + 5, y + 6, 3, 18};
    SDL_RenderFillRect(ren, &shine);
    SDL_SetRenderDrawColor(ren, 196, 214, 232, 180);
    SDL_Rect shine2{x + 9, y + 8, 1, 9};
    SDL_RenderFillRect(ren, &shine2);
    SDL_SetRenderDrawColor(ren, 116, 80, 40, 255);
    SDL_Rect base{x - 2, y + 34, 26, 3};
    SDL_RenderFillRect(ren, &base);
}

void Game::drawClock(int wx, int wy)
{
    int x = sx(static_cast<float>(wx));
    int y = sy(static_cast<float>(wy));
    if (x < -40 || x > VIEW_W + 16) return;

    SDL_SetRenderDrawColor(ren, 56, 34, 24, 255);
    SDL_Rect body{x, y, 22, 48};
    SDL_RenderFillRect(ren, &body);
    SDL_SetRenderDrawColor(ren, 96, 62, 38, 255);
    SDL_Rect inner{x + 3, y + 3, 16, 42};
    SDL_RenderFillRect(ren, &inner);
    fillCircle(x + 11, y + 12, 7, SDL_Color{186, 160, 108, 255});
    fillCircle(x + 11, y + 12, 5, SDL_Color{238, 226, 190, 255});
    SDL_SetRenderDrawColor(ren, 38, 28, 22, 255);
    SDL_RenderDrawLine(ren, x + 11, y + 12, x + 11, y + 8);
    SDL_RenderDrawLine(ren, x + 11, y + 12, x + 14, y + 14);
    SDL_SetRenderDrawColor(ren, 150, 120, 60, 255);
    SDL_Rect pendulum{x + 10, y + 22, 2, 14};
    SDL_RenderFillRect(ren, &pendulum);
    fillCircle(x + 11, y + 37, 3, SDL_Color{186, 148, 70, 255});
    SDL_SetRenderDrawColor(ren, 124, 88, 46, 255);
    SDL_Rect top{x - 2, y - 2, 26, 3};
    SDL_RenderFillRect(ren, &top);
}

// ---------------------------------------------------------------------------
//  Дверь и кубки
// ---------------------------------------------------------------------------
void Game::drawDoor()
{
    if (door.w <= 0) return;
    int x = sx(door.x);
    int y = sy(door.y);
    int w = static_cast<int>(door.w);
    int h = static_cast<int>(door.h);
    if (x < -80 || x > VIEW_W + 40) return;

    SDL_SetRenderDrawColor(ren, 42, 26, 20, 255);
    SDL_Rect frame{x - 3, y - 3, w + 6, h + 6};
    SDL_RenderFillRect(ren, &frame);
    SDL_SetRenderDrawColor(ren, 78, 44, 28, 255);
    SDL_Rect doorRect{x, y, w, h};
    SDL_RenderFillRect(ren, &doorRect);

    // арка сверху
    SDL_SetRenderDrawColor(ren, 50, 30, 22, 255);
    SDL_Rect arch1{x + 2, y - 2, w - 4, 4};
    SDL_RenderFillRect(ren, &arch1);
    SDL_SetRenderDrawColor(ren, 150, 110, 58, 255);
    SDL_Rect archGold{x + 2, y + 1, w - 4, 2};
    SDL_RenderFillRect(ren, &archGold);

    // створки
    SDL_SetRenderDrawColor(ren, 112, 66, 38, 255);
    SDL_Rect left{x + 3, y + 6, w / 2 - 4, h - 10};
    SDL_Rect right{x + w / 2 + 1, y + 6, w / 2 - 4, h - 10};
    SDL_RenderFillRect(ren, &left);
    SDL_RenderFillRect(ren, &right);
    SDL_SetRenderDrawColor(ren, 66, 38, 24, 255);
    SDL_Rect seam{x + w / 2 - 1, y + 6, 2, h - 10};
    SDL_RenderFillRect(ren, &seam);

    // филёнки
    SDL_SetRenderDrawColor(ren, 138, 86, 46, 255);
    SDL_Rect p1{x + 6, y + 12, w / 2 - 10, 12};
    SDL_Rect p2{x + 6, y + 28, w / 2 - 10, 12};
    SDL_Rect p3{x + w / 2 + 4, y + 12, w / 2 - 10, 12};
    SDL_Rect p4{x + w / 2 + 4, y + 28, w / 2 - 10, 12};
    SDL_RenderDrawRect(ren, &p1);
    SDL_RenderDrawRect(ren, &p2);
    SDL_RenderDrawRect(ren, &p3);
    SDL_RenderDrawRect(ren, &p4);

    // ручки
    fillCircle(x + w / 2 - 5, y + h / 2 + 2, 2, SDL_Color{220, 178, 82, 255});
    fillCircle(x + w / 2 + 5, y + h / 2 + 2, 2, SDL_Color{220, 178, 82, 255});

    // три замка под кубки
    for (int i = 0; i < 3; ++i) {
        int ky = y + 15 + i * 9;
        bool lit = i < cupsGot;
        fillCircle(x + 8, ky, 2,
                   lit ? SDL_Color{255, 214, 90, 255} : SDL_Color{40, 26, 20, 255});
    }

    bool unlocked = (cupsGot >= static_cast<int>(cups.size()));
    if (unlocked) {
        float pulse = 0.5f + 0.5f * std::sin(timeSec * 3.2f);
        addLight(door.x + door.w * 0.5f, door.y + door.h * 0.5f,
                 90.0f, SDL_Color{255, 210, 110, 255}, 55.0f + 45.0f * pulse);
        SDL_SetRenderDrawColor(ren, 255, 226, 140,
                               static_cast<Uint8>(90 + pulse * 100));
        SDL_Rect glowLine{x + 4, y + 4, w - 8, 1};
        SDL_RenderFillRect(ren, &glowLine);
    }

    // табличка
    SDL_SetRenderDrawColor(ren, 176, 138, 66, 255);
    SDL_Rect sign{x + w / 2 - 12, y - 12, 24, 8};
    SDL_RenderFillRect(ren, &sign);
    SDL_SetRenderDrawColor(ren, 42, 28, 18, 255);
    SDL_RenderDrawLine(ren, x + w / 2 - 7, y - 8, x + w / 2 + 7, y - 8);
}

void Game::drawCups()
{
    for (const CupItem& c : cups) {
        if (c.taken) continue;
        float bob = std::sin(timeSec * 3.4f + c.tx * 0.7f) * 2.0f;
        float wx = c.tx * 16.0f + 2.0f;
        float wy = c.ty * 16.0f + 3.0f + bob;
        drawCupIcon(sx(wx), sy(wy), 0.0f);
        addLight(c.tx * 16.0f + 8.0f, c.ty * 16.0f + 8.0f, 30.0f,
                 SDL_Color{255, 210, 90, 255}, 50.0f);
    }
}

void Game::drawJewelry()
{
    for (const Jewel& jw : jewelry) {
        if (jw.taken) continue;
        if (jw.x + 16.0f < camX || jw.x > camX + VIEW_W + 16.0f) continue;
        if (jw.y + 16.0f < camY || jw.y > camY + VIEW_H + 16.0f) continue;

        float bob = std::sin(timeSec * 2.8f + jw.phase) * 1.6f;

        // тень на полу
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 60);
        SDL_Rect sh{sx(jw.x) + 1, sy(jw.y) + 11, 9, 2};
        SDL_RenderFillRect(ren, &sh);

        drawJewelIcon(sx(jw.x), sy(jw.y), jw.type, bob);

        SDL_Color glow = (jw.type == 0) ? SDL_Color{150, 205, 255, 255}
                       : (jw.type == 1) ? SDL_Color{130, 240, 160, 255}
                                        : SDL_Color{255, 210, 110, 255};
        addLight(jw.x + 5.0f, jw.y + 5.0f, 22.0f, glow, 34.0f);
    }
}

void Game::drawJewelIcon(int x, int y, int type, float bob)
{
    int yy = y + static_cast<int>(std::lround(bob));

    if (type == 0) {
        // серебряный перстень с голубым камнем
        fillCircle(x + 5, yy + 6, 4, SDL_Color{198, 204, 216, 255});
        fillCircle(x + 5, yy + 6, 2, SDL_Color{42, 36, 42, 255});
        SDL_SetRenderDrawColor(ren, 112, 198, 246, 255);
        SDL_Rect gem{x + 3, yy + 1, 5, 4};
        SDL_RenderFillRect(ren, &gem);
        SDL_SetRenderDrawColor(ren, 206, 240, 255, 230);
        SDL_Rect gl{x + 4, yy + 2, 2, 1};
        SDL_RenderFillRect(ren, &gl);
    } else if (type == 1) {
        // золотой кулон с изумрудом на цепочке
        SDL_SetRenderDrawColor(ren, 238, 206, 120, 200);
        SDL_RenderDrawLine(ren, x + 5, yy, x + 2, yy + 4);
        SDL_RenderDrawLine(ren, x + 5, yy, x + 8, yy + 4);
        fillCircle(x + 5, yy + 6, 4, SDL_Color{240, 190, 60, 255});
        fillTriangle(x + 5, yy + 3, x + 2, yy + 6, x + 8, yy + 6,
                     SDL_Color{80, 214, 128, 255});
        SDL_SetRenderDrawColor(ren, 220, 255, 230, 220);
        SDL_Rect gl{x + 4, yy + 4, 1, 2};
        SDL_RenderFillRect(ren, &gl);
    } else {
        // корона с рубином
        SDL_SetRenderDrawColor(ren, 246, 194, 62, 255);
        SDL_Rect base{x + 1, yy + 5, 9, 3};
        SDL_RenderFillRect(ren, &base);
        SDL_Rect p1{x + 1, yy + 2, 2, 3};
        SDL_Rect p2{x + 4, yy + 1, 3, 4};
        SDL_Rect p3{x + 8, yy + 2, 2, 3};
        SDL_RenderFillRect(ren, &p1);
        SDL_RenderFillRect(ren, &p2);
        SDL_RenderFillRect(ren, &p3);
        fillCircle(x + 5, yy + 3, 1, SDL_Color{232, 72, 82, 255});
        fillCircle(x + 2, yy + 7, 1, SDL_Color{110, 190, 255, 255});
        fillCircle(x + 8, yy + 7, 1, SDL_Color{110, 220, 140, 255});
        SDL_SetRenderDrawColor(ren, 255, 244, 190, 200);
        SDL_Rect gl{x + 2, yy + 5, 2, 1};
        SDL_RenderFillRect(ren, &gl);
    }
}

// ---------------------------------------------------------------------------
//  Персонажи, частицы, свет
// ---------------------------------------------------------------------------
void Game::drawMonsters()
{
    for (const Monster& m : monsters) {
        if (m.type == 0) drawSpider(m);
        else if (m.type == 1) drawZombie(m);
        else drawKnifeThrower(m);
    }
}

void Game::drawSpider(const Monster& m)
{
    const Body& b = m.b;
    if (b.x + b.w < camX - 30.0f || b.x > camX + VIEW_W + 30.0f) return;
    if (b.y + b.h < camY - 30.0f || b.y > camY + VIEW_H + 30.0f) return;

    int x = sx(b.x);
    int y = sy(b.y);
    int w = static_cast<int>(std::lround(b.w));
    int h = static_cast<int>(std::lround(b.h));
    int cx = x + w / 2;

    if (b.grounded) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 80);
        SDL_Rect sh{x - 3, y + h - 1, w + 6, 2};
        SDL_RenderFillRect(ren, &sh);
    }

    // Паутина: от потолочного крепления к спускающемуся пауку.
    if (m.webState != 0) {
        int ax = sx(m.webAnchorX);
        int ay = sy(m.webAnchorY);
        int tx = cx;
        int ty = y + 2;
        SDL_SetRenderDrawColor(ren, 226, 228, 238, 165);
        SDL_RenderDrawLine(ren, ax, ay, tx, ty);
        SDL_SetRenderDrawColor(ren, 186, 190, 208, 95);
        SDL_RenderDrawLine(ren, ax + 1, ay, tx + 1, ty);
        SDL_Rect knot{ax - 1, ay - 1, 3, 2};
        SDL_SetRenderDrawColor(ren, 214, 218, 230, 190);
        SDL_RenderFillRect(ren, &knot);
    }

    // Восемь лап: на потолке они выгнуты вверх и держатся за перекрытие,
    // в полёте на паутине и на земле — свисают вниз.
    const bool onCeiling = (m.webState == 0);
    SDL_SetRenderDrawColor(ren, 42, 28, 38, 255);
    for (int side = 0; side < 2; ++side) {
        for (int i = 0; i < 4; ++i) {
            int phase = static_cast<int>(
                std::sin(m.anim * 7.0f + i * 1.6f + side * 1.1f) * 2.0f);
            int rootX = (side == 0) ? cx - 2 : cx + 2;
            int rootY = y + 5 + i * 2;
            int kneeX = (side == 0) ? x - 1 - i / 2 : x + w + 1 + i / 2;
            int kneeY = onCeiling ? rootY - 5 - (i % 2)
                                  : rootY - 4 - (i % 2);
            int footX = (side == 0) ? x - 4 - i : x + w + 4 + i;
            int footY = onCeiling ? y - 3 + phase / 2
                                  : y + h - 1 + phase + (i % 2);
            SDL_RenderDrawLine(ren, rootX, rootY, kneeX, kneeY);
            SDL_RenderDrawLine(ren, kneeX, kneeY, footX, footY);
            SDL_RenderDrawLine(ren, rootX, rootY + 1, kneeX, kneeY + 1);
            SDL_RenderDrawLine(ren, kneeX, kneeY + 1, footX, footY + 1);
        }
    }

    // брюшко с крестом
    float abdX = (b.facing > 0) ? b.x + 1.0f : b.x + b.w - 11.0f;
    fillW(abdX + 1.0f, b.y + 3.0f, 8.0f, b.h - 5.0f, SDL_Color{66, 38, 54, 255});
    fillW(abdX, b.y + 5.0f, 10.0f, b.h - 9.0f, SDL_Color{84, 48, 66, 255});
    fillW(abdX + 2.0f, b.y + 4.0f, 5.0f, 2.0f, SDL_Color{118, 72, 94, 255});
    fillW(abdX + 4.0f, b.y + 6.0f, 2.0f, b.h - 9.0f, SDL_Color{146, 58, 68, 210});
    fillW(abdX + 1.0f, b.y + 9.0f, 8.0f, 2.0f, SDL_Color{146, 58, 68, 210});

    // голова и хелицеры
    float hx = (b.facing > 0) ? b.x + b.w - 6.0f : b.x + 1.0f;
    fillW(hx, b.y + 4.0f, 6.0f, 5.0f, SDL_Color{102, 60, 80, 255});
    fillW(hx + 1.0f, b.y + 3.0f, 4.0f, 2.0f, SDL_Color{126, 78, 100, 255});
    float clX = (b.facing > 0) ? b.x + b.w - 1.0f : b.x - 2.0f;
    fillW(clX, b.y + 7.0f, 3.0f, 1.0f, SDL_Color{52, 34, 46, 255});

    // глаза
    int ex = (b.facing > 0) ? sx(hx + 3.0f) : sx(hx + 1.0f);
    SDL_SetRenderDrawColor(ren, 255, 74, 62, 255);
    SDL_Rect e1{ex, sy(b.y + 5.0f), 2, 2};
    SDL_Rect e2{ex + (b.facing > 0 ? -3 : 3), sy(b.y + 6.0f), 1, 1};
    SDL_RenderFillRect(ren, &e1);
    SDL_RenderFillRect(ren, &e2);
    addLight(b.x + b.w * 0.5f, b.y + 6.0f, 20.0f,
             SDL_Color{255, 100, 70, 255}, 18.0f);

    if (m.hurtFlash > 0.0f) {
        SDL_SetRenderDrawColor(ren, 255, 225, 225, 80);
        SDL_Rect flash{x + 2, y + 2, w - 4, h - 4};
        SDL_RenderFillRect(ren, &flash);
    }
}

void Game::drawZombie(const Monster& m)
{
    const Body& b = m.b;
    if (b.x + b.w < camX - 40.0f || b.x > camX + VIEW_W + 40.0f) return;
    if (b.y + b.h < camY - 40.0f || b.y > camY + VIEW_H + 40.0f) return;

    int x = sx(b.x);
    int y = sy(b.y);
    int w = static_cast<int>(std::lround(b.w));
    int h = static_cast<int>(std::lround(b.h));
    int dir = b.facing;

    if (b.grounded) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 85);
        SDL_Rect sh{x - 3, y + h - 1, w + 6, 2};
        SDL_RenderFillRect(ren, &sh);
    }

    int step = (static_cast<int>(m.anim * 5.0f) % 2 == 0) ? 1 : 0;

    // ноги
    fillW(b.x + 2.0f + step, b.y + 18.0f, 4.0f, 6.0f, SDL_Color{52, 46, 42, 255});
    fillW(b.x + 8.0f - step, b.y + 18.0f, 4.0f, 6.0f, SDL_Color{42, 38, 36, 255});
    fillW(b.x + 1.0f + step, b.y + 23.0f, 5.0f, 3.0f, SDL_Color{28, 26, 24, 255});
    fillW(b.x + 8.0f - step, b.y + 23.0f, 5.0f, 3.0f, SDL_Color{24, 22, 22, 255});

    // рваный пиджак
    fillW(b.x + 2.0f, b.y + 8.0f, b.w - 4.0f, 10.0f, SDL_Color{84, 92, 80, 255});
    fillW(b.x + 2.0f, b.y + 8.0f, 2.0f, 10.0f, SDL_Color{58, 68, 58, 255});
    fillW(b.x + 4.0f, b.y + 17.0f, 3.0f, 1.0f, SDL_Color{60, 52, 44, 255});
    fillW(b.x + 9.0f, b.y + 17.0f, 2.0f, 2.0f, SDL_Color{58, 50, 42, 255});
    fillW(b.x + 6.0f, b.y + 11.0f, 3.0f, 2.0f, SDL_Color{118, 36, 36, 210});

    // вытянутые вперёд руки
    float hand1X = (dir > 0) ? b.x + b.w + 3.0f : b.x - 7.0f;
    float hand2X = (dir > 0) ? b.x + b.w + 1.0f : b.x - 5.0f;
    fillW((dir > 0) ? b.x + b.w - 2.0f : b.x - 6.0f, b.y + 9.0f,
          8.0f, 3.0f, SDL_Color{96, 138, 84, 255});
    fillW((dir > 0) ? b.x + b.w - 2.0f : b.x - 4.0f, b.y + 14.0f,
          6.0f, 3.0f, SDL_Color{78, 116, 70, 255});
    fillW(hand1X, b.y + 8.0f, 4.0f, 4.0f, SDL_Color{110, 154, 92, 255});
    fillW(hand2X, b.y + 13.0f, 4.0f, 4.0f, SDL_Color{96, 140, 82, 255});
    fillW(hand1X + (dir > 0 ? 3.0f : -1.0f), b.y + 9.0f, 2.0f, 1.0f,
          SDL_Color{130, 176, 108, 255});

    // голова
    fillW(b.x + 2.0f, b.y, 10.0f, 9.0f, SDL_Color{118, 158, 94, 255});
    fillW(b.x + 1.0f, b.y - 1.0f, 12.0f, 3.0f, SDL_Color{48, 54, 38, 255});
    fillW(b.x + (dir > 0 ? 2.0f : 9.0f), b.y + 1.0f, 3.0f, 1.0f,
          SDL_Color{150, 190, 118, 255});
    fillW(b.x + 4.0f, b.y + 8.0f, 6.0f, 2.0f, SDL_Color{70, 96, 58, 255});

    // глаза и зубы
    float eyeX = (dir > 0) ? b.x + b.w - 4.0f : b.x + 2.0f;
    fillW(eyeX, b.y + 3.0f, 2.0f, 2.0f, SDL_Color{246, 224, 120, 255});
    fillW(eyeX + (dir > 0 ? -3.0f : 3.0f), b.y + 4.0f, 1.0f, 1.0f,
          SDL_Color{230, 190, 90, 255});
    fillW(b.x + 5.0f, b.y + 7.0f, 4.0f, 1.0f, SDL_Color{238, 232, 210, 255});

    addLight(b.x + b.w * 0.5f, b.y + 8.0f, 24.0f,
             SDL_Color{150, 210, 90, 255}, 18.0f);

    if (m.hurtFlash > 0.0f) {
        SDL_SetRenderDrawColor(ren, 235, 255, 220, 85);
        SDL_Rect flash{x + 1, y + 1, w - 2, h - 2};
        SDL_RenderFillRect(ren, &flash);
    }
}

void Game::drawKnifeThrower(const Monster& m)
{
    const Body& b = m.b;
    if (b.x + b.w < camX - 40.0f || b.x > camX + VIEW_W + 40.0f) return;
    if (b.y + b.h < camY - 40.0f || b.y > camY + VIEW_H + 40.0f) return;

    int x = sx(b.x);
    int y = sy(b.y);
    int w = static_cast<int>(std::lround(b.w));
    int h = static_cast<int>(std::lround(b.h));
    int dir = b.facing;

    if (b.grounded) {
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 80);
        SDL_Rect sh{x - 2, y + h - 1, w + 4, 2};
        SDL_RenderFillRect(ren, &sh);
    }

    int step = (static_cast<int>(m.anim * 5.0f) % 2 == 0) ? 1 : 0;

    // ноги
    fillW(b.x + 2.0f + step, b.y + 19.0f, 4.0f, 7.0f, SDL_Color{38, 34, 40, 255});
    fillW(b.x + 8.0f - step, b.y + 19.0f, 4.0f, 7.0f, SDL_Color{30, 28, 34, 255});
    fillW(b.x + 1.0f + step, b.y + 24.0f, 5.0f, 2.0f, SDL_Color{20, 18, 22, 255});
    fillW(b.x + 8.0f - step, b.y + 24.0f, 5.0f, 2.0f, SDL_Color{18, 16, 20, 255});

    // длинный тёмный плащ
    fillW(b.x + 1.0f, b.y + 7.0f, b.w - 2.0f, 13.0f, SDL_Color{58, 44, 62, 255});
    fillW(b.x + 1.0f, b.y + 7.0f, 3.0f, 13.0f, SDL_Color{40, 30, 46, 255});
    fillW(b.x + 3.0f, b.y + 18.0f, 4.0f, 3.0f, SDL_Color{48, 36, 52, 255});
    fillW(b.x + 9.0f, b.y + 18.0f, 3.0f, 3.0f, SDL_Color{48, 36, 52, 255});

    // пояс с метательными ножами
    fillW(b.x + 1.0f, b.y + 16.0f, b.w - 2.0f, 2.0f, SDL_Color{26, 20, 24, 255});
    fillW(b.x + 3.0f, b.y + 15.0f, 1.0f, 3.0f, SDL_Color{200, 210, 225, 255});
    fillW(b.x + 7.0f, b.y + 15.0f, 1.0f, 3.0f, SDL_Color{180, 192, 210, 255});
    fillW(b.x + 10.0f, b.y + 15.0f, 1.0f, 3.0f, SDL_Color{200, 210, 225, 255});

    // капюшон и лицо в тени
    fillW(b.x + 2.0f, b.y, 10.0f, 9.0f, SDL_Color{44, 32, 52, 255});
    fillW(b.x + 1.0f, b.y + 1.0f, 12.0f, 3.0f, SDL_Color{32, 24, 40, 255});
    fillW(b.x + 4.0f, b.y + 3.0f, 6.0f, 5.0f, SDL_Color{24, 18, 26, 255});

    float eyeX = (dir > 0) ? b.x + b.w - 5.0f : b.x + 3.0f;
    fillW(eyeX, b.y + 4.0f, 2.0f, 2.0f, SDL_Color{255, 96, 64, 255});
    fillW(eyeX + (dir > 0 ? -3.0f : 3.0f), b.y + 5.0f, 1.0f, 1.0f,
          SDL_Color{220, 80, 60, 255});

    // занесённая рука с ножом
    float handX = (dir > 0) ? b.x + b.w + 3.0f : b.x - 7.0f;
    fillW((dir > 0) ? b.x + b.w - 2.0f : b.x - 5.0f, b.y + 9.0f, 7.0f, 3.0f,
          SDL_Color{64, 48, 68, 255});
    fillW(handX, b.y + 8.0f, 4.0f, 4.0f, SDL_Color{96, 74, 72, 255});
    fillW((dir > 0) ? handX + 4.0f : handX - 5.0f, b.y + 9.0f, 5.0f, 1.0f,
          SDL_Color{206, 216, 230, 255});

    addLight(b.x + b.w * 0.5f, b.y + 8.0f, 22.0f,
             SDL_Color{180, 70, 70, 255}, 16.0f);

    if (m.hurtFlash > 0.0f) {
        SDL_SetRenderDrawColor(ren, 255, 220, 220, 80);
        SDL_Rect flash{x + 1, y + 1, w - 2, h - 2};
        SDL_RenderFillRect(ren, &flash);
    }
}

void Game::drawKnives()
{
    for (const Knife& k : knives) {
        int x = sx(k.x);
        int y = sy(k.y);
        if (x < -24 || x > VIEW_W + 24 || y < -24 || y > VIEW_H + 24) continue;
        int dir = k.dir;

        // рукоять и гарда
        SDL_SetRenderDrawColor(ren, 58, 38, 22, 255);
        SDL_Rect handle{(dir > 0) ? x - 8 : x + 2, y - 1, 6, 2};
        SDL_RenderFillRect(ren, &handle);
        SDL_SetRenderDrawColor(ren, 150, 110, 50, 255);
        SDL_Rect guard{(dir > 0) ? x - 3 : x + 1, y - 3, 2, 6};
        SDL_RenderFillRect(ren, &guard);

        // клинок-треугольник
        SDL_Color steel{212, 222, 238, 255};
        if (dir > 0)
            fillTriangle(x + 2, y - 3, x + 2, y + 3, x + 12, y, steel);
        else
            fillTriangle(x - 2, y - 3, x - 2, y + 3, x - 12, y, steel);

        // блик по лезвию
        SDL_SetRenderDrawColor(ren, 248, 252, 255, 200);
        if (dir > 0)
            SDL_RenderDrawLine(ren, x + 3, y - 1, x + 10, y - 1);
        else
            SDL_RenderDrawLine(ren, x - 3, y - 1, x - 10, y - 1);

        addLight(k.x, k.y, 16.0f, SDL_Color{180, 200, 230, 255}, 12.0f);
    }
}

void Game::drawGuy(const Body& b, const Palette& p)
{
    if (b.invuln > 0 && b.hurt <= 0 && static_cast<int>(timeSec * 26.0f) % 2 == 0) return;

    int x = sx(b.x);
    int y = sy(b.y);
    int w = static_cast<int>(std::lround(b.w));
    int h = static_cast<int>(std::lround(b.h));
    if (x + w < -20 || x > VIEW_W + 20) return;
    if (y + h < -20 || y > VIEW_H + 20) return;

    bool walking = b.grounded && std::fabs(b.vx) > 8.0f && !b.climbing;
    int step = walking ? (static_cast<int>(b.anim) / 6) % 4 : 0;
    int swing = (step == 1) ? 1 : ((step == 3) ? -1 : 0);
    if (b.climbing) swing = (static_cast<int>(b.anim / 4.0f) % 2 == 0) ? 1 : -1;
    if (b.hurt > 0) swing = 0;

    // Вертикальные координаты — мировые: fillW сам переводит их в экранные.
    float bodyY = b.y + 8.0f;
    float bodyH = b.h - 18.0f;
    float legY = bodyY + bodyH;
    float feet = b.y + b.h;
    int look = b.lookY;
    float hy = b.y + static_cast<float>(look);  // голова чуть выше/ниже

    // тень
    if (b.grounded) {
        fillW(b.x - 2.0f, b.y + b.h - 1.0f, b.w + 4.0f, 2.0f,
              SDL_Color{0, 0, 0, 70});
    }

    // ноги и башмаки
    fillW(b.x + 1.0f + swing, legY, 3.0f, feet - legY - 2.0f, p.pants);
    fillW(b.x + b.w - 4.0f - swing, legY, 3.0f, feet - legY - 2.0f, p.pants);
    fillW(b.x + swing, feet - 3.0f, 4.0f, 3.0f, p.shoe);
    fillW(b.x + b.w - 4.0f - swing, feet - 3.0f, 4.0f, 3.0f, p.shoe);

    // туловище
    fillW(b.x, bodyY, b.w, bodyH, p.coat);
    fillW(b.x, bodyY, 2.0f, bodyH, p.coatDark);
    fillW(b.x, bodyY + bodyH - 2.0f, b.w, 2.0f,
          SDL_Color{38, 30, 30, 255});
    fillW(b.x + b.w - 2.0f, bodyY + 1.0f, 1.0f,
          bodyH - 3.0f, shade(p.coat, 1.18f));

    // руки
    int armOff = (step == 1) ? 1 : ((step == 3) ? -1 : 0);
    if (b.climbing) armOff = (swing > 0) ? -2 : 1;
    fillW(b.x - 2.0f, bodyY + 1.0f + armOff, 2.0f, 5.0f, p.coatDark);
    fillW(b.x + b.w, bodyY + 1.0f - armOff, 2.0f, 5.0f, p.coatDark);

    // шарф
    fillW(b.x - 1.0f, bodyY, b.w + 2.0f, 2.0f, p.accent);
    float tailX = (b.facing > 0) ? b.x - 3.0f : b.x + b.w + 1.0f;
    fillW(tailX, bodyY + 2.0f, 3.0f, 4.0f, p.accent);

    // голова
    fillW(b.x - 1.0f, hy, b.w + 2.0f, 7.0f, p.skin);
    fillW(b.x - 1.0f, hy - 1.0f, b.w + 2.0f, 3.0f, p.hair);
    float sideX = (b.facing > 0) ? b.x - 1.0f : b.x + b.w - 1.0f;
    fillW(sideX, hy + 1.0f, 2.0f, 2.0f, p.hair);
    // чубчик
    fillW(b.x + (b.facing > 0 ? b.w - 3.0f : 1.0f), hy - 2.0f,
          3.0f, 2.0f, p.hair);

    // глаз
    int eyeX = (b.facing > 0) ? x + w - 3 : x + 2;
    int eyeY = sy(hy + 4.0f);
    SDL_SetRenderDrawColor(ren, p.eye.r, p.eye.g, p.eye.b, 255);
    SDL_Rect eye{eyeX, eyeY, 1, 2};
    SDL_RenderFillRect(ren, &eye);
    // брови при взгляде вверх/вниз
    SDL_SetRenderDrawColor(ren, p.hair.r, p.hair.g, p.hair.b, 255);
    SDL_Rect brow{eyeX - 1, eyeY - 1, 3, 1};
    SDL_RenderFillRect(ren, &brow);

    // дробовик в руках: ствол поворачивается по диагонали вместе со взглядом
    float aimAng = b.lookY * 0.7853981634f;
    float ax = b.facing * std::cos(aimAng);
    float ay = std::sin(aimAng);
    float kick = (gunKick > 0.0f) ? (gunKick / 0.14f) * 1.5f : 0.0f;
    float shoulderX = b.x + b.w * 0.5f + b.facing * 1.0f - b.facing * kick * 0.4f;
    float shoulderY = b.y + 12.0f - ay * kick * 0.3f;
    if (!b.climbing && b.hurt <= 0) {
        if (b.lookY == 0) {
            float gx = (b.facing > 0) ? b.x + b.w - 1.0f : b.x - 7.0f;
            gx -= b.facing * kick;
            float gy = shoulderY - 1.5f;
            fillW(gx, gy, 8.0f, 3.0f, SDL_Color{62, 64, 72, 255});
            fillW(gx + (b.facing > 0 ? 6.0f : 0.0f), gy, 2.0f, 2.0f,
                  SDL_Color{126, 130, 142, 255});
            fillW(gx + (b.facing > 0 ? 1.0f : 4.0f), gy + 3.0f, 3.0f, 1.0f,
                  SDL_Color{112, 76, 42, 255});
            float stockX = (b.facing > 0) ? b.x + b.w - 4.0f : b.x - 1.0f;
            stockX -= b.facing * kick;
            fillW(stockX, gy + 1.0f, 4.0f, 3.0f, SDL_Color{104, 68, 36, 255});
        } else {
            for (int i = 0; i < 4; ++i) {
                float t = 1.0f + i * 2.1f;
                fillW(shoulderX + ax * t - 1.5f, shoulderY + ay * t - 1.5f,
                      3.0f, 3.0f, (i == 3 ? SDL_Color{126, 130, 142, 255}
                                         : SDL_Color{62, 64, 72, 255}));
            }
            fillW(shoulderX - ax * 2.2f - 1.0f, shoulderY - ay * 2.2f - 1.0f,
                  3.0f, 3.0f, SDL_Color{104, 68, 36, 255});
        }
    }

    // вспышка выстрела
    if (muzzle > 0.0f) {
        float tipX = shoulderX + ax * 9.0f;
        float tipY = shoulderY + ay * 9.0f;
        int mx = sx(tipX);
        int my = sy(tipY);
        int ex = sx(tipX + ax * 9.0f);
        int ey = sy(tipY + ay * 9.0f);
        int px = sx(tipX - ay * 5.0f);
        int py = sy(tipY + ax * 5.0f);
        int qx = sx(tipX + ay * 5.0f);
        int qy = sy(tipY - ax * 5.0f);
        fillCircle(mx, my, 4, SDL_Color{255, 190, 70, 210});
        fillCircle(mx, my, 2, SDL_Color{255, 250, 210, 245});
        SDL_SetRenderDrawColor(ren, 255, 210, 100, 190);
        SDL_RenderDrawLine(ren, mx, my, ex, ey);
        SDL_RenderDrawLine(ren, px, py, qx, qy);
    }
}

void Game::drawParticles()
{
    for (const Particle& p : parts) {
        float a = clampf(p.life / p.maxLife, 0.0f, 1.0f);
        if (p.x < camX - 10 || p.x > camX + VIEW_W + 10) continue;
        if (p.y < camY - 10 || p.y > camY + VIEW_H + 10) continue;
        SDL_SetRenderDrawColor(ren, p.color.r, p.color.g, p.color.b,
                               static_cast<Uint8>(p.color.a * a));
        SDL_Rect r{sx(p.x), sy(p.y),
                   static_cast<int>(std::max(1.0f, p.size)),
                   static_cast<int>(std::max(1.0f, p.size))};
        SDL_RenderFillRect(ren, &r);
    }
}

void Game::drawTracers()
{
    for (const Tracer& t : tracers) {
        float k = clampf(t.life / t.maxLife, 0.0f, 1.0f);
        if (k <= 0.0f) continue;

        int x0 = sx(t.x0), y0 = sy(t.y0);
        int x1 = sx(t.x1), y1 = sy(t.y1);
        if (std::max(x0, x1) < -8 || std::min(x0, x1) > VIEW_W + 8) continue;
        if (std::max(y0, y1) < -8 || std::min(y0, y1) > VIEW_H + 8) continue;

        float len = std::sqrt(static_cast<float>((x1 - x0) * (x1 - x0) +
                                                 (y1 - y0) * (y1 - y0)));
        float nx = 0.0f, ny = 0.0f;
        if (len > 0.01f) {
            nx = -(y1 - y0) / len;
            ny = (x1 - x0) / len;
        }

        // Мягкое свечение вокруг следа.
        Uint8 glow = static_cast<Uint8>(90.0f * k * k);
        SDL_SetRenderDrawColor(ren, t.color.r, t.color.g, t.color.b, glow);
        for (int o = -1; o <= 1; o += 2) {
            int ox = static_cast<int>(std::lround(nx * o));
            int oy = static_cast<int>(std::lround(ny * o));
            SDL_RenderDrawLine(ren, x0 + ox, y0 + oy, x1 + ox, y1 + oy);
        }

        // Яркое ядро, тающее к концу жизни.
        Uint8 a = static_cast<Uint8>(245.0f * k * k);
        SDL_SetRenderDrawColor(ren, t.color.r, t.color.g, t.color.b, a);
        SDL_RenderDrawLine(ren, x0, y0, x1, y1);
    }
}

void Game::drawGlows()
{
    addLight(player.x + player.w * 0.5f, player.y + player.h * 0.6f,
             46.0f, SDL_Color{255, 200, 140, 255}, 24.0f);
    if (muzzle > 0.0f) {
        float aimAng = player.lookY * 0.7853981634f;
        float dirX = player.facing * std::cos(aimAng);
        float dirY = std::sin(aimAng);
        float tipX = player.x + player.w * 0.5f + player.facing * 1.0f + dirX * 9.0f;
        float tipY = player.y + 12.0f + dirY * 9.0f;
        addLight(tipX, tipY, 52.0f, SDL_Color{255, 190, 80, 255},
                 130.0f * clampf(muzzle / 0.10f, 0.0f, 1.0f));
    }

    for (const Light& L : lights) {
        if (L.x + L.r < camX || L.x - L.r > camX + VIEW_W) continue;
        if (L.y + L.r < camY || L.y - L.r > camY + VIEW_H) continue;
        int r = static_cast<int>(L.r);
        SDL_Rect dst{sx(L.x) - r, sy(L.y) - r, r * 2, r * 2};
        SDL_SetTextureColorMod(glowTex, L.color.r, L.color.g, L.color.b);
        SDL_SetTextureAlphaMod(glowTex,
                               static_cast<Uint8>(clampf(L.alpha, 0.0f, 255.0f)));
        SDL_RenderCopy(ren, glowTex, nullptr, &dst);
    }
    SDL_SetTextureColorMod(glowTex, 255, 255, 255);
    SDL_SetTextureAlphaMod(glowTex, 255);
}

void Game::drawVignette()
{
    SDL_RenderCopy(ren, vignetteTex, nullptr, nullptr);
}

void Game::drawScorePopups()
{
    for (const ScorePopup& p : popups) {
        float fade = clampf(p.life / 0.35f, 0.0f, 1.0f);
        std::string txt = "+" + std::to_string(p.value);
        int tx = sx(p.x) - pixfont::textWidth(txt, 1) / 2;
        int ty = sy(p.y);
        SDL_SetRenderDrawColor(ren, 12, 8, 12, static_cast<Uint8>(180.0f * fade));
        pixfont::drawTextRaw(ren, tx + 1, ty + 1, txt, 1);
        SDL_SetRenderDrawColor(ren, p.color.r, p.color.g, p.color.b,
                               static_cast<Uint8>(255.0f * fade));
        pixfont::drawTextRaw(ren, tx, ty, txt, 1);
    }
}

// ---------------------------------------------------------------------------
//  Интерфейс
// ---------------------------------------------------------------------------
void Game::drawHUD()
{
    // Верхний левый блок: кубки и очки в одной тонкой рамке.
    bool allCups = cupsGot >= static_cast<int>(cups.size());
    float pulse = allCups ? (0.5f + 0.5f * std::sin(timeSec * 4.5f)) : 0.0f;
    SDL_Rect panel{4, 4, 100, 17};
    SDL_SetRenderDrawColor(ren, 10, 8, 12, 145);
    SDL_RenderFillRect(ren, &panel);
    SDL_SetRenderDrawColor(ren,
                           allCups ? static_cast<Uint8>(160 + pulse * 60) : 110,
                           allCups ? static_cast<Uint8>(126 + pulse * 50) : 86,
                           allCups ? static_cast<Uint8>(58 + pulse * 40) : 48, 200);
    SDL_RenderDrawRect(ren, &panel);

    drawCupIcon(6, 7, std::sin(timeSec * 2.5f) * (allCups ? 1.0f : 0.4f));
    std::string cupText = std::to_string(cupsGot) + "/" + std::to_string(cups.size());
    pixfont::drawText(ren, 21, 9, cupText, 1,
                      allCups ? SDL_Color{255, 224, 120, 255}
                              : SDL_Color{236, 226, 206, 255}, false);

    SDL_SetRenderDrawColor(ren, 88, 70, 42, 180);
    SDL_RenderDrawLine(ren, 42, 7, 42, 17);

    // Маленькая монетка и счёт.
    fillTriangle(49, 7, 52, 11, 49, 15, SDL_Color{255, 205, 80, 255});
    fillTriangle(49, 7, 46, 11, 49, 15, SDL_Color{255, 232, 160, 255});
    pixfont::drawText(ren, 56, 9, std::to_string(score), 1,
                      SDL_Color{255, 234, 172, 255}, false);

    // Сердца вверху по центру, без рамки.
    for (int i = 0; i < START_LIVES; ++i) {
        int cx = VIEW_W / 2 - 16 + i * 12;
        int cy = 7;
        SDL_Color hc = (i < lives) ? SDL_Color{232, 66, 72, 255}
                                   : SDL_Color{74, 52, 56, 210};
        fillCircle(cx - 2, cy, 2, hc);
        fillCircle(cx + 2, cy, 2, hc);
        fillTriangle(cx - 4, cy, cx + 4, cy, cx, cy + 4, hc);
        if (i < lives) {
            SDL_SetRenderDrawColor(ren, 255, 152, 152, 170);
            SDL_Rect shine{cx - 2, cy - 2, 2, 1};
            SDL_RenderFillRect(ren, &shine);
        }
    }
    std::string floorText = "ЭТАЖ " + std::to_string(levelIndex + 1) + "/" +
                            std::to_string(LEVEL_COUNT);
    pixfont::drawTextCentered(ren, VIEW_W / 2, 13, floorText, 1,
                              SDL_Color{214, 204, 184, 220}, false);

    // Патроны: крошечная пиктограмма, число и гильзы.
    bool lowAmmo = (mag == 0);
    SDL_Rect ammoPanel{VIEW_W - 92, 4, 88, 17};
    SDL_SetRenderDrawColor(ren, 10, 8, 12, 145);
    SDL_RenderFillRect(ren, &ammoPanel);
    SDL_SetRenderDrawColor(ren, lowAmmo ? 200 : 106, lowAmmo ? 92 : 86,
                           lowAmmo ? 70 : 52, 200);
    SDL_RenderDrawRect(ren, &ammoPanel);

    {
        int ix = ammoPanel.x + 4;
        int iy = ammoPanel.y + 4;
        SDL_SetRenderDrawColor(ren, 104, 68, 36, 255);
        SDL_Rect stock{ix, iy + 3, 4, 4};
        SDL_RenderFillRect(ren, &stock);
        SDL_SetRenderDrawColor(ren, 70, 74, 84, 255);
        SDL_Rect body{ix + 3, iy + 3, 10, 3};
        SDL_RenderFillRect(ren, &body);
        SDL_SetRenderDrawColor(ren, 140, 146, 158, 255);
        SDL_Rect barrel{ix + 12, iy + 3, 5, 2};
        SDL_RenderFillRect(ren, &barrel);
        SDL_SetRenderDrawColor(ren, 120, 82, 44, 255);
        SDL_Rect pump{ix + 8, iy + 6, 4, 2};
        SDL_RenderFillRect(ren, &pump);
    }

    std::string ammoText = std::to_string(mag) + "/" + std::to_string(MAG_SIZE);
    pixfont::drawText(ren, ammoPanel.x + 24, 9, ammoText, 1,
                      lowAmmo ? SDL_Color{230, 120, 90, 255}
                              : SDL_Color{245, 235, 215, 255}, false);

    for (int i = 0; i < MAG_SIZE; ++i) {
        int px = ammoPanel.x + 46 + i * 5;
        int py = ammoPanel.y + 5;
        if (i < mag) {
            SDL_SetRenderDrawColor(ren, 222, 172, 74, 255);
            SDL_Rect shell{px, py, 3, 7};
            SDL_RenderFillRect(ren, &shell);
            SDL_SetRenderDrawColor(ren, 255, 214, 120, 255);
            SDL_Rect tip{px, py, 3, 2};
            SDL_RenderFillRect(ren, &tip);
        } else {
            SDL_SetRenderDrawColor(ren, 70, 58, 46, 220);
            SDL_Rect shell{px, py, 3, 7};
            SDL_RenderDrawRect(ren, &shell);
        }
    }

    if (reloading) {
        float k = reloadDuration > 0.001f
                      ? clampf(1.0f - reloadTimer / reloadDuration, 0.0f, 1.0f)
                      : 1.0f;
        SDL_SetRenderDrawColor(ren, 58, 44, 30, 210);
        SDL_Rect barBg{ammoPanel.x + 4, ammoPanel.y + 13, 80, 2};
        SDL_RenderFillRect(ren, &barBg);
        SDL_SetRenderDrawColor(ren, 240, 190, 90, 255);
        SDL_Rect bar{ammoPanel.x + 4, ammoPanel.y + 13,
                     static_cast<int>(80 * k), 2};
        SDL_RenderFillRect(ren, &bar);
    }

    // Короткая подсказка в начале — тонкая полоска без рамки.
    if (introTimer < 14.0f) {
        float a = introTimer < 11.0f ? 1.0f : clampf((14.0f - introTimer) / 3.0f, 0.0f, 1.0f);
        std::string hint = "A/D ИДТИ  W/↑ ПРИЦЕЛ ВВЕРХ  S/↓ ВНИЗ  CTRL ВЫСТРЕЛ";
        int tw = pixfont::textWidth(hint, 1);
        SDL_SetRenderDrawColor(ren, 8, 6, 10, static_cast<Uint8>(120 * a));
        SDL_Rect hp{VIEW_W / 2 - tw / 2 - 4, VIEW_H - 13, tw + 8, 10};
        SDL_RenderFillRect(ren, &hp);
        pixfont::drawText(ren, VIEW_W / 2 - tw / 2, VIEW_H - 11, hint, 1,
                          SDL_Color{220, 205, 180, static_cast<Uint8>(210 * a)},
                          false);
    }
}

void Game::drawTitleScreen()
{
    SDL_SetRenderDrawColor(ren, 8, 5, 10, 150);
    SDL_RenderFillRect(ren, nullptr);

    // золотые линейки
    SDL_SetRenderDrawColor(ren, 214, 174, 92, 140);
    SDL_Rect l1{70, 44, VIEW_W - 140, 1};
    SDL_Rect l2{70, 128, VIEW_W - 140, 1};
    SDL_RenderFillRect(ren, &l1);
    SDL_RenderFillRect(ren, &l2);

    pixfont::drawTextCentered(ren, VIEW_W / 2, 52, "АПАРТАМЕНТЫ", 3,
                              SDL_Color{255, 222, 130, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 84, "«ДАЙМОНД»", 3,
                              SDL_Color{238, 206, 120, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 112, "ПРИКЛЮЧЕНИЕ КОЛЯНА", 1,
                              SDL_Color{212, 196, 170, 255});

    const char* items[3] = {"НОВАЯ ИГРА", "НАСТРОЙКИ", "ВЫХОД"};
    for (int i = 0; i < 3; ++i) {
        int y = 148 + i * 24;
        bool sel = (i == menuIndex);
        if (sel) {
            SDL_SetRenderDrawColor(ren, 70, 54, 28, 210);
            SDL_Rect bg{VIEW_W / 2 - 92, y - 4, 184, 17};
            SDL_RenderFillRect(ren, &bg);
            SDL_SetRenderDrawColor(ren, 214, 174, 92, 220);
            SDL_Rect ln{VIEW_W / 2 - 92, y + 13, 184, 1};
            SDL_RenderFillRect(ren, &ln);
        }
        pixfont::drawTextCentered(ren, VIEW_W / 2, y, items[i], 2,
                                  sel ? SDL_Color{255, 232, 160, 255}
                                      : SDL_Color{196, 184, 162, 255});
    }

    pixfont::drawTextCentered(ren, VIEW_W / 2, 232,
                              "W/S ИЛИ ↑/↓ - ВЫБОР   ENTER - ОК", 1,
                              SDL_Color{196, 176, 148, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 248,
                              "ESC - ВЫХОД ИЗ ИГРЫ", 1,
                              SDL_Color{150, 140, 128, 255});
}

void Game::drawSettingsScreen()
{
    SDL_SetRenderDrawColor(ren, 8, 5, 10, 190);
    SDL_RenderFillRect(ren, nullptr);

    // золотые линейки
    SDL_SetRenderDrawColor(ren, 214, 174, 92, 140);
    SDL_Rect l1{40, 20, VIEW_W - 80, 1};
    SDL_Rect l2{40, 206, VIEW_W - 80, 1};
    SDL_RenderFillRect(ren, &l1);
    SDL_RenderFillRect(ren, &l2);

    pixfont::drawTextCentered(ren, VIEW_W / 2, 26, "НАСТРОЙКИ", 2,
                              SDL_Color{255, 222, 130, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 44, "УПРАВЛЕНИЕ", 1,
                              SDL_Color{196, 176, 148, 255});

    // Заголовки двух колонок с клавишами.
    pixfont::drawText(ren, 386 - pixfont::textWidth("КНОПКА 1", 1), 56,
                      "КНОПКА 1", 1, SDL_Color{170, 158, 138, 255});
    pixfont::drawText(ren, 452 - pixfont::textWidth("КНОПКА 2", 1), 56,
                      "КНОПКА 2", 1, SDL_Color{170, 158, 138, 255});

    const int rows = CA_COUNT + 1;
    const int slotRight[2] = {386, 452};
    for (int i = 0; i < rows; ++i) {
        int y = 72 + i * 17;
        bool sel = (i == settingsIndex);
        bool waiting = (i == bindTarget);
        if (sel) {
            if (waiting) SDL_SetRenderDrawColor(ren, 84, 44, 30, 215);
            else         SDL_SetRenderDrawColor(ren, 56, 44, 24, 215);
            SDL_Rect bg{44, y - 3, 412, 13};
            SDL_RenderFillRect(ren, &bg);
            if (waiting) SDL_SetRenderDrawColor(ren, 230, 120, 70, 230);
            else         SDL_SetRenderDrawColor(ren, 214, 174, 92, 230);
            SDL_Rect mark{44, y - 3, 2, 13};
            SDL_RenderFillRect(ren, &mark);
        }

        const char* label = (i < CA_COUNT) ? controlName(i) : "НАЗАД";
        SDL_Color lc = sel ? SDL_Color{246, 236, 214, 255}
                           : SDL_Color{190, 180, 160, 255};
        pixfont::drawText(ren, 56, y, label, 1, lc);

        if (i < CA_COUNT) {
            for (int s = 0; s < 2; ++s) {
                bool slotSel = sel && (s == settingsSlot);
                bool slotWaiting = waiting && (s == bindSlot);
                std::string key = slotWaiting ? "?"
                                              : keyLabel(controls.keys[i][s]);
                int tw = std::max(12, pixfont::textWidth(key, 1));
                if (slotSel) {
                    if (slotWaiting) SDL_SetRenderDrawColor(ren, 96, 46, 28, 235);
                    else             SDL_SetRenderDrawColor(ren, 96, 74, 38, 235);
                    SDL_Rect kr{slotRight[s] - tw - 4, y - 2, tw + 8, 11};
                    SDL_RenderFillRect(ren, &kr);
                }
                SDL_Color kc;
                if (slotWaiting)     kc = SDL_Color{255, 200, 130, 255};
                else if (slotSel)    kc = SDL_Color{255, 232, 160, 255};
                else if (controls.keys[i][s] == SDL_SCANCODE_UNKNOWN)
                                     kc = SDL_Color{120, 110, 100, 255};
                else                 kc = SDL_Color{212, 196, 170, 255};
                pixfont::drawText(ren, slotRight[s] - pixfont::textWidth(key, 1),
                                  y, key, 1, kc);
            }
        } else {
            pixfont::drawText(ren, 452 - pixfont::textWidth("ESC", 1), y,
                              "ESC", 1, sel ? SDL_Color{255, 226, 150, 255}
                                            : SDL_Color{160, 150, 138, 255});
        }
    }

    if (bindTarget >= 0) {
        pixfont::drawTextCentered(ren, VIEW_W / 2, 216,
                                  "НАЖМИТЕ КЛАВИШУ   ESC - ОТМЕНА", 1,
                                  SDL_Color{235, 180, 120, 255});
    } else {
        pixfont::drawTextCentered(ren, VIEW_W / 2, 216,
                                  "W/S - ВЫБОР   A/D - КНОПКА 1 / 2", 1,
                                  SDL_Color{196, 176, 148, 255});
        pixfont::drawTextCentered(ren, VIEW_W / 2, 232,
                                  "ENTER - СМЕНИТЬ   ESC - НАЗАД", 1,
                                  SDL_Color{150, 140, 128, 255});
    }
}

void Game::drawWinScreen()
{
    SDL_SetRenderDrawColor(ren, 8, 10, 26, 150);
    SDL_RenderFillRect(ren, nullptr);
    drawParticles();

    SDL_SetRenderDrawColor(ren, 220, 184, 96, 160);
    SDL_Rect l1{64, 64, VIEW_W - 128, 1};
    SDL_Rect l2{64, 168, VIEW_W - 128, 1};
    SDL_RenderFillRect(ren, &l1);
    SDL_RenderFillRect(ren, &l2);

    pixfont::drawTextCentered(ren, VIEW_W / 2, 80, "КОЛЯН ВЫБРАЛСЯ!", 3,
                              SDL_Color{255, 226, 140, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 124, "ИЗ АПАРТАМЕНТОВ «ДАЙМОНД»", 2,
                              SDL_Color{230, 220, 200, 255});

    if (std::fmod(titleBlink, 1.15f) < 0.78f) {
        pixfont::drawTextCentered(ren, VIEW_W / 2, 192, "R - ИГРАТЬ СНОВА", 2,
                                  SDL_Color{255, 232, 170, 255});
    }
    std::string stats = "КУБКИ 3/3   ОЧКИ: " + std::to_string(score) +
                        "   МОНСТРОВ: " + std::to_string(kills);
    pixfont::drawTextCentered(ren, VIEW_W / 2, 226, stats, 1,
                              SDL_Color{190, 180, 160, 255});
}

void Game::drawDeathOverlay()
{
    const bool spider = (deathCause == DEATH_SPIDER);
    const bool knife = (deathCause == DEATH_KNIFE);
    float t = deathTimer;

    SDL_SetRenderDrawColor(ren, 6, 3, 8, 170);
    SDL_RenderFillRect(ren, nullptr);

    // маленькое окно по центру — без единой надписи, всего на несколько
    // пикселей больше самого Коляна в кадре
    SDL_Rect panel{VIEW_W / 2 - 50, (VIEW_H - 100) / 2, 100, 100};
    SDL_SetRenderDrawColor(ren, 20, 14, 20, 242);
    SDL_RenderFillRect(ren, &panel);

    SDL_Color edge = spider ? SDL_Color{178, 86, 140, 255}
                            : (knife ? SDL_Color{196, 208, 228, 255}
                                     : SDL_Color{124, 180, 78, 255});
    SDL_SetRenderDrawColor(ren, edge.r, edge.g, edge.b, 255);
    SDL_RenderDrawRect(ren, &panel);
    SDL_SetRenderDrawColor(ren, static_cast<Uint8>(edge.r / 2),
                           static_cast<Uint8>(edge.g / 2),
                           static_cast<Uint8>(edge.b / 2), 255);
    SDL_Rect inner{panel.x + 3, panel.y + 3, panel.w - 6, panel.h - 6};
    SDL_RenderDrawRect(ren, &inner);

    SDL_RenderSetClipRect(ren, &panel);

    auto box = [&](int bx, int by, int bw, int bh, SDL_Color c) {
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
        SDL_Rect r{bx, by, bw, bh};
        SDL_RenderFillRect(ren, &r);
    };
    auto limb = [&](int x0, int y0, int x1, int y1, int th, SDL_Color c) {
        int dx = x1 - x0, dy = y1 - y0;
        int steps = std::max(1, std::max(std::abs(dx), std::abs(dy)) / 2);
        for (int i = 0; i <= steps; ++i) {
            int lx = x0 + dx * i / steps;
            int ly = y0 + dy * i / steps;
            box(lx - th / 2, ly - th / 2, th, th, c);
        }
    };

    if (spider) {
        // Смерть от паука: он спускается на нити и кусает.
        int px = panel.x + 24;
        int py = panel.y + 20;
        float k = clampf((t - 0.20f) / 1.05f, 0.0f, 1.0f);
        float bite = clampf((t - 0.80f) / 0.55f, 0.0f, 1.0f);
        SDL_Color badSkin{158, 86, 198, 255};
        SDL_Color skin{
            static_cast<Uint8>(238 + (badSkin.r - 238) * bite),
            static_cast<Uint8>(190 + (badSkin.g - 190) * bite),
            static_cast<Uint8>(150 + (badSkin.b - 150) * bite), 255};

        box(px + 1, py + 38, 46, 16, SDL_Color{52, 84, 148, 255});
        box(px + 1, py + 38, 46, 3, SDL_Color{190, 58, 62, 255});
        box(px + 7, py + 8, 32, 32, skin);
        box(px + 10, py + 40, 26, 3, skin);
        box(px + 5, py + 2, 36, 10, SDL_Color{62, 40, 30, 255});
        box(px + 4, py + 8, 4, 12, SDL_Color{62, 40, 30, 255});
        box(px + 38, py + 8, 4, 12, SDL_Color{62, 40, 30, 255});

        if (bite < 0.55f) {
            box(px + 13, py + 20, 8, 6, SDL_Color{248, 246, 240, 255});
            box(px + 26, py + 20, 8, 6, SDL_Color{248, 246, 240, 255});
            int look = (bite > 0.25f) ? 2 : 0;
            box(px + 15 + look, py + 22, 3, 3, SDL_Color{30, 22, 24, 255});
            box(px + 28 + look, py + 22, 3, 3, SDL_Color{30, 22, 24, 255});
        } else {
            SDL_SetRenderDrawColor(ren, 56, 16, 24, 255);
            for (int e = 0; e < 2; ++e) {
                int ex = px + (e == 0 ? 13 : 26);
                int ey = py + 20;
                SDL_RenderDrawLine(ren, ex, ey, ex + 7, ey + 6);
                SDL_RenderDrawLine(ren, ex + 7, ey, ex, ey + 6);
            }
        }
        if (bite > 0.35f && bite < 0.95f) {
            box(px + 19, py + 32, 9, 6, SDL_Color{90, 20, 28, 255});
            box(px + 22, py + 32, 3, 2, SDL_Color{240, 236, 226, 255});
        } else {
            box(px + 20, py + 33, 8, 2, SDL_Color{150, 72, 72, 255});
        }

        int jitter = (bite > 0.0f && bite < 1.0f)
                         ? static_cast<int>(std::sin(t * 42.0f) * 2.0f) : 0;
        int mx = px + 39 + jitter;
        int my = static_cast<int>(panel.y + 8 + k * (py + 22 - (panel.y + 8)));
        SDL_SetRenderDrawColor(ren, 210, 210, 220, 120);
        SDL_RenderDrawLine(ren, mx, panel.y + 4, mx, my - 4);
        SDL_SetRenderDrawColor(ren, 38, 24, 34, 255);
        for (int side = -1; side <= 1; side += 2) {
            for (int i = 0; i < 4; ++i) {
                int lx = mx + side * (6 + i);
                int ly = my + 1 - i * 2;
                SDL_RenderDrawLine(ren, mx, my, lx, ly - 2);
                SDL_RenderDrawLine(ren, lx, ly - 2, lx + side * 3, ly + 3);
            }
        }
        fillCircle(mx, my, 5, SDL_Color{82, 44, 62, 255});
        fillCircle(mx, my - 2, 3, SDL_Color{112, 64, 86, 255});
        box(mx - 2, my - 4, 2, 2, SDL_Color{255, 70, 60, 255});
        box(mx + 1, my - 4, 2, 2, SDL_Color{255, 70, 60, 255});
        box(mx - 2, my + 5, 5, 2, SDL_Color{30, 18, 26, 255});
    } else if (knife) {
        // Смерть от ножа: крупный нож входит в глаз, голова — строго в профиль.
        float u = clampf((t - 0.12f) / 0.60f, 0.0f, 1.0f);
        float impact = clampf((t - 0.72f) / 0.12f, 0.0f, 1.0f);
        float jt = clampf((t - 0.72f) / 0.32f, 0.0f, 1.0f);
        float jolt = -std::sin(jt * 3.14159265f) * 5.0f;

        int px = panel.x + 24 + static_cast<int>(jolt);
        int py = panel.y + 14;

        const SDL_Color skin{236, 188, 148, 255};
        const SDL_Color skinShade{204, 154, 120, 255};
        const SDL_Color hair{58, 38, 28, 255};

        // плечи, шарф, шея
        box(px - 14, py + 56, 82, 20, SDL_Color{52, 84, 148, 255});
        box(px - 14, py + 56, 82, 4, SDL_Color{190, 58, 62, 255});
        box(px + 18, py + 52, 32, 6, SDL_Color{190, 58, 62, 255});
        box(px + 26, py + 44, 20, 12, skinShade);

        // голова в профиль, лицом влево
        box(px + 2, py, 50, 48, skin);
        box(px + 44, py + 8, 10, 38, skinShade);
        // нос и подбородок
        fillTriangle(px + 2, py + 20, px - 6, py + 28, px + 2, py + 34, skinShade);
        box(px + 4, py + 44, 24, 6, skinShade);
        // волосы: затылок, макушка, чёлка
        box(px - 1, py - 5, 54, 12, hair);
        box(px + 40, py + 2, 15, 40, hair);
        box(px + 34, py + 8, 7, 13, hair);
        box(px + 3, py + 5, 12, 4, hair);
        // ухо поверх волос — сразу читается профиль
        box(px + 38, py + 23, 11, 13, skinShade);
        box(px + 41, py + 26, 4, 7, hair);

        // один глаз в профиль: нож войдёт прямо в него
        int eyeY = py + 17;
        if (impact < 0.5f) {
            box(px + 11, eyeY, 13, 8, SDL_Color{248, 246, 240, 255});
            box(px + 14, eyeY + 2, 4, 4, SDL_Color{30, 22, 24, 255});
            box(px + 10, eyeY - 4, 17, 3, hair);
        } else {
            SDL_SetRenderDrawColor(ren, 56, 16, 24, 255);
            SDL_RenderDrawLine(ren, px + 11, eyeY + 4, px + 23, eyeY + 4);
            SDL_RenderDrawLine(ren, px + 11, eyeY + 2, px + 23, eyeY + 6);
            SDL_RenderDrawLine(ren, px + 11, eyeY + 6, px + 23, eyeY + 2);
        }
        // рот
        box(px + 5, py + 40, 12, 2, SDL_Color{150, 72, 72, 255});

        // цель ножа — глаз
        float targetX = static_cast<float>(px + 23);
        float targetY = static_cast<float>(eyeY + 4);
        float startX = static_cast<float>(panel.x - 14);
        float tipX = startX + (targetX - startX) * u;

        // след полёта
        if (u > 0.05f && u < 1.0f) {
            SDL_SetRenderDrawColor(ren, 210, 224, 240, 80);
            SDL_RenderDrawLine(ren, static_cast<int>(tipX) - 30,
                               static_cast<int>(targetY),
                               static_cast<int>(tipX),
                               static_cast<int>(targetY));
        }

        // крупный нож: длинный клинок, гарда и толстая рукоять
        const int bladeLen = 27;
        const int handleLen = 14;
        int hy = static_cast<int>(targetY);
        int tip = static_cast<int>(tipX);
        int guardX = tip - bladeLen;
        int handleX = guardX - handleLen;
        // рукоять
        box(handleX, hy - 3, handleLen, 6, SDL_Color{52, 34, 20, 255});
        box(handleX + 2, hy - 3, 2, 6, SDL_Color{96, 66, 36, 255});
        box(handleX + 7, hy - 3, 2, 6, SDL_Color{96, 66, 36, 255});
        box(handleX - 2, hy - 4, 3, 8, SDL_Color{78, 52, 28, 255});
        // гарда
        box(guardX - 2, hy - 8, 4, 16, SDL_Color{176, 136, 64, 255});
        box(guardX - 1, hy - 7, 2, 14, SDL_Color{224, 186, 96, 255});
        // клинок с долом и блеском
        fillTriangle(guardX, hy - 5, guardX, hy + 5, tip, hy,
                     SDL_Color{206, 216, 232, 255});
        fillTriangle(guardX, hy - 5, guardX, hy, tip, hy,
                     SDL_Color{236, 242, 250, 255});
        SDL_SetRenderDrawColor(ren, 120, 132, 154, 255);
        SDL_RenderDrawLine(ren, guardX + 2, hy + 2, tip - 3, hy + 1);
        SDL_SetRenderDrawColor(ren, 255, 255, 255, 220);
        SDL_RenderDrawLine(ren, guardX + 2, hy - 3, tip - 4, hy - 1);

        // удар: брызги и струйка крови из глаза
        if (impact > 0.0f) {
            int cx = px + 18;
            int cy = eyeY + 4;
            box(cx - 3, cy - 2, 7, 5, SDL_Color{150, 30, 40, 235});
            box(cx - 1, cy + 3, 2, 7, SDL_Color{128, 24, 34, 210});
            box(cx + 3, cy + 5, 1, 5, SDL_Color{120, 22, 32, 190});
            float ia = clampf(1.0f - (t - 0.80f) / 0.5f, 0.0f, 1.0f);
            SDL_SetRenderDrawColor(ren, 200, 40, 50,
                                   static_cast<Uint8>(210 * ia));
            for (int i = 0; i < 7; ++i) {
                float a = 0.45f + i * 0.38f;
                int len = 6 + static_cast<int>(9.0f * impact);
                SDL_RenderDrawLine(ren, cx, cy,
                                   cx + static_cast<int>(std::cos(a) * len),
                                   cy - static_cast<int>(std::sin(a) * len) + 2);
            }
        }

        // рука Коляна дёрнулась к лицу
        limb(px + 10, py + 62, px + 5, py + 42, 6, skinShade);
        box(px + 1, py + 37, 8, 6, skin);
    } else {
        // Смерть от зомби: удар сзади сносит Коляну голову.
        float u = clampf((t - 0.10f) / 0.85f, 0.0f, 1.0f);
        float impact = clampf((t - 0.95f) / 0.12f, 0.0f, 1.0f);
        float jt = clampf((t - 0.95f) / 0.30f, 0.0f, 1.0f);
        float jolt = -std::sin(jt * 3.14159265f) * 6.0f;
        float detach = clampf((t - 0.95f) / 0.70f, 0.0f, 1.0f);
        bool hit = (t >= 0.95f);
        // голова остаётся в кадре, а тело оседает вниз — между шеей и головой
        // появляется хорошо читаемый провал.
        int bodyDrop = static_cast<int>(18.0f * detach);

        int px = panel.x + 23 + static_cast<int>(jolt);
        int py = panel.y + 14;
        int headX = px - static_cast<int>(12.0f * detach);
        int headY = py - static_cast<int>(6.0f * detach);

        const SDL_Color skin{238, 190, 150, 255};
        const SDL_Color skinShade{206, 156, 122, 255};
        const SDL_Color hair{62, 40, 30, 255};

        // плечи и шарф — после удара тело оседает
        box(px - 14, py + 56 + bodyDrop, 82, 20, SDL_Color{52, 84, 148, 255});
        box(px - 14, py + 56 + bodyDrop, 82, 4, SDL_Color{190, 58, 62, 255});
        box(px + 18, py + 54 + bodyDrop, 32, 6, SDL_Color{190, 58, 62, 255});

        // шея: до удара целая, после — кровавый обрубок
        if (!hit) {
            box(px + 30, py + 46, 16, 12, skinShade);
        } else {
            box(px + 30, py + 42 + bodyDrop, 16, 12, SDL_Color{126, 24, 34, 255});
            box(px + 32, py + 42 + bodyDrop, 12, 3,
                SDL_Color{232, 220, 200, 255});
            box(px + 33, py + 45 + bodyDrop, 3, 2,
                SDL_Color{180, 40, 48, 255});
            box(px + 41, py + 45 + bodyDrop, 3, 2,
                SDL_Color{180, 40, 48, 255});
            // кровь стекает по обрубку
            box(px + 31, py + 50 + bodyDrop, 2, 4, SDL_Color{150, 28, 38, 220});
            box(px + 43, py + 49 + bodyDrop, 1, 3, SDL_Color{140, 26, 36, 200});
        }

        // голова в профиль: после удара отлетает влево-вверх
        box(headX, headY, 52, 50, skin);
        box(headX + 3, headY + 42, 46, 9, skin);
        fillTriangle(headX, headY + 24, headX - 7, headY + 31,
                     headX, headY + 35, skinShade);
        box(headX - 3, headY - 5, 58, 14, hair);
        box(headX + 38, headY - 2, 16, 44, hair);
        box(headX + 34, headY + 4, 6, 12, hair);
        box(headX + 40, headY + 22, 9, 11, skinShade);
        box(headX + 42, headY + 25, 4, 5, hair);
        box(headX + 7, headY + 15, 13, 2, hair);

        if (impact < 0.5f) {
            box(headX + 8, headY + 19, 11, 5, SDL_Color{248, 246, 240, 255});
            box(headX + 10, headY + 20, 3, 3, SDL_Color{30, 22, 24, 255});
        } else {
            SDL_SetRenderDrawColor(ren, 56, 16, 24, 255);
            SDL_RenderDrawLine(ren, headX + 8, headY + 18, headX + 19, headY + 24);
            SDL_RenderDrawLine(ren, headX + 19, headY + 18, headX + 8, headY + 24);
            SDL_RenderDrawLine(ren, headX + 8, headY + 19, headX + 18, headY + 24);
            SDL_RenderDrawLine(ren, headX + 9, headY + 18, headX + 19, headY + 23);
        }

        if (impact > 0.2f) {
            box(headX + 4, headY + 38, 16, 7, SDL_Color{90, 20, 28, 255});
            box(headX + 6, headY + 38, 3, 2, SDL_Color{240, 236, 226, 255});
            box(headX + 11, headY + 38, 2, 2, SDL_Color{240, 236, 226, 255});
        } else {
            box(headX + 6, headY + 41, 12, 2, SDL_Color{150, 72, 72, 255});
        }

        // рваный срез шеи на отлетевшей голове
        if (hit) {
            box(headX + 24, headY + 47, 20, 4, SDL_Color{126, 24, 34, 255});
            box(headX + 30, headY + 47, 7, 2, SDL_Color{232, 220, 200, 255});
            box(headX + 26, headY + 50, 2, 5, SDL_Color{128, 24, 34, 210});
            box(headX + 38, headY + 50, 1, 4, SDL_Color{120, 22, 32, 190});
        }

        // кровь хлещет из обрубка шеи
        if (hit) {
            int nx = px + 38;
            int ny = py + 42 + bodyDrop;
            float fade = clampf(1.0f - (t - 1.10f) / 1.20f, 0.0f, 1.0f);
            for (int i = 0; i < 9; ++i) {
                float a = 3.35f + i * 0.22f;
                int len = 8 + static_cast<int>(13.0f * impact) +
                          static_cast<int>(2.0f * std::sin(t * 24.0f + i));
                SDL_SetRenderDrawColor(ren, 196, 34, 44,
                                       static_cast<Uint8>(220 * fade));
                SDL_RenderDrawLine(ren, nx, ny,
                                   nx + static_cast<int>(std::cos(a) * len),
                                   ny + static_cast<int>(std::sin(a) * len));
            }
            for (int i = 0; i < 5; ++i) {
                int dx = ((i * 7 + 3) % 25) - 12;
                int dy = static_cast<int>(std::fmod(t * 46.0f + i * 9.0f, 26.0f));
                SDL_SetRenderDrawColor(ren, 170, 28, 38,
                                       static_cast<Uint8>(210 * fade));
                SDL_Rect drop{nx + dx, ny + dy, 2, 3};
                SDL_RenderFillRect(ren, &drop);
            }
        }

        // рука зомби летит по дуге от правого верхнего угла к затылку
        int startX = panel.x + 112, startY = panel.y - 12;
        int ctrlX = panel.x + 94, ctrlY = panel.y - 14;
        int endX = px + 46, endY = py + 10;
        float inv = 1.0f - u;
        int hx = static_cast<int>(inv * inv * startX + 2.0f * inv * u * ctrlX +
                                  u * u * endX);
        int hy = static_cast<int>(inv * inv * startY + 2.0f * inv * u * ctrlY +
                                  u * u * endY);
        // после удара зомби отдёргивает руку, выпуская голову
        float retract = clampf((t - 1.02f) / 0.45f, 0.0f, 1.0f);
        hx += static_cast<int>(18.0f * retract);
        hy -= static_cast<int>(6.0f * retract);
        if (u >= 1.0f && retract <= 0.0f) {
            hx += static_cast<int>(std::sin(t * 55.0f));
            hy += static_cast<int>(std::cos(t * 47.0f));
        }

        // плечо и предплечье зомби
        int shoulderX = panel.x + 112, shoulderY = panel.y + 84;
        int elbowX = panel.x + 80, elbowY = panel.y + 10;
        limb(shoulderX, shoulderY, elbowX, elbowY, 11, SDL_Color{54, 62, 46, 255});
        limb(shoulderX + 2, shoulderY, elbowX + 2, elbowY + 2, 7,
             SDL_Color{70, 82, 58, 255});
        limb(elbowX, elbowY, hx, hy, 8, SDL_Color{104, 148, 86, 255});
        limb(elbowX, elbowY, hx + 2, hy - 1, 4, SDL_Color{128, 172, 102, 255});

        // кисть и пальцы
        SDL_Color hand{118, 162, 96, 255};
        SDL_Color finger{144, 186, 112, 255};
        box(hx - 8, hy - 6, 15, 12, hand);
        box(hx - 13, hy - 6, 6, 3, finger);
        box(hx - 14, hy - 2, 7, 3, finger);
        box(hx - 13, hy + 2, 6, 3, finger);
        box(hx - 9, hy + 6, 5, 3, finger);
        box(hx + 3, hy - 8, 4, 5, SDL_Color{92, 132, 74, 255});

        if (impact > 0.0f) {
            float ia = clampf(1.0f - (t - 1.05f) / 0.35f, 0.0f, 1.0f);
            int cx = px + 46, cy = py + 8;
            SDL_SetRenderDrawColor(ren, 255, 238, 150,
                                   static_cast<Uint8>(220 * ia));
            for (int i = 0; i < 8; ++i) {
                float a = i * 0.785398f;
                int len = 6 + static_cast<int>(10.0f * impact);
                SDL_RenderDrawLine(ren, cx, cy,
                                   cx + static_cast<int>(std::cos(a) * len),
                                   cy + static_cast<int>(std::sin(a) * len));
            }
        } else if (u > 0.25f) {
            // след движения руки
            SDL_SetRenderDrawColor(ren, 220, 230, 210, 60);
            SDL_RenderDrawLine(ren, hx + 12, hy - 14, hx + 3, hy - 2);
            SDL_RenderDrawLine(ren, hx + 18, hy - 6, hx + 7, hy + 1);
        }
    }

    SDL_RenderSetClipRect(ren, nullptr);
}

void Game::drawGameOverScreen()
{
    SDL_SetRenderDrawColor(ren, 12, 3, 7, 195);
    SDL_RenderFillRect(ren, nullptr);

    SDL_SetRenderDrawColor(ren, 168, 62, 68, 150);
    SDL_Rect l1{64, 62, VIEW_W - 128, 1};
    SDL_Rect l2{64, 198, VIEW_W - 128, 1};
    SDL_RenderFillRect(ren, &l1);
    SDL_RenderFillRect(ren, &l2);

    pixfont::drawTextCentered(ren, VIEW_W / 2, 76, "ИГРА ОКОНЧЕНА", 3,
                              SDL_Color{232, 96, 92, 255});
    pixfont::drawTextCentered(ren, VIEW_W / 2, 120,
                              "КОЛЯН ПОГИБ В АПАРТАМЕНТАХ «ДАЙМОНД»", 1,
                              SDL_Color{218, 200, 190, 255});
    const char* causeText = (deathCause == DEATH_SPIDER) ? "ПОСЛЕДНЯЯ ПРИЧИНА: ПАУК"
                            : (deathCause == DEATH_KNIFE) ? "ПОСЛЕДНЯЯ ПРИЧИНА: НОЖ"
                                                          : "ПОСЛЕДНЯЯ ПРИЧИНА: ЗОМБИ";
    SDL_Color causeCol = (deathCause == DEATH_SPIDER) ? SDL_Color{196, 110, 166, 255}
                         : (deathCause == DEATH_KNIFE) ? SDL_Color{196, 208, 228, 255}
                                                       : SDL_Color{140, 196, 90, 255};
    pixfont::drawTextCentered(ren, VIEW_W / 2, 138, causeText, 1, causeCol);

    std::string stats = "МОНСТРОВ ПОВЕРЖЕНО: " + std::to_string(kills) +
                        "   ОЧКИ: " + std::to_string(score);
    pixfont::drawTextCentered(ren, VIEW_W / 2, 160, stats, 1,
                              SDL_Color{180, 168, 152, 255});

    if (std::fmod(titleBlink, 1.15f) < 0.78f) {
        pixfont::drawTextCentered(ren, VIEW_W / 2, 210, "R - ЗАНОВО", 2,
                                  SDL_Color{255, 226, 140, 255});
    }
}

// ---------------------------------------------------------------------------
//  Скриншоты и автоматический тест
// ---------------------------------------------------------------------------
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

    // 1b — подвал: метатель ножей и нож в полёте
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
            thrower.b.y = 49 * 16.0f - thrower.b.h;
            thrower.homeX = thrower.b.x;
            thrower.homeY = thrower.b.y;
            thrower.b.facing = -1;
            thrower.throwCd = 0.0f;
            monsters.push_back(thrower);
        }
    }
    player.x = 110 * 16.0f;
    player.y = 49 * 16.0f - PLAYER_H;
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

    // 3 — бой: диагональный выстрел вверх по зомби
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
        m.b.y = player.y - 45.0f;
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
    player.y = 49 * 16.0f - PLAYER_H;
    player.vx = player.vy = 0;
    player.grounded = true;
    player.facing = 1;
    snapCamera();
    introTimer = 20.0f;
    render();
    screenshot("shot_door.bmp");
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

    // 7 — победа
    state = ST_WIN;
    kills = 9;
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

    // 8b — зомби сносит Коляну голову
    deathCause = DEATH_ZOMBIE;
    deathTimer = 1.60f;
    render();
    screenshot("shot_death_zombie.bmp");
    SDL_RenderPresent(ren);

    // 8c — нож между глаз (вид в профиль)
    resetWorld(0);
    state = ST_PLAY;
    player.x = 17 * 16.0f;
    player.y = 482.0f;
    player.facing = 1;
    player.grounded = true;
    snapCamera();
    introTimer = 20.0f;
    lives = 2;
    deathCause = DEATH_KNIFE;
    deathTimer = 1.20f;
    titleBlink = 0.25f;
    state = ST_DYING;
    render();
    screenshot("shot_death_knife.bmp");
    SDL_RenderPresent(ren);

    // 9 — экран поражения
    resetWorld(0);
    state = ST_PLAY;
    kills = 7;
    lives = 0;
    deathCause = DEATH_ZOMBIE;
    deathTimer = 0;
    titleBlink = 0.25f;
    state = ST_GAMEOVER;
    render();
    screenshot("shot_gameover.bmp");
    SDL_RenderPresent(ren);
}

void Game::runSmoke()
{
    resetWorld(0);
    state = ST_PLAY;
    monsters.clear();

    bool startFirstFloor = std::fabs((player.y + player.h) - 32 * 16.0f) < 1.0f;
    bool doorBasement = (door.w > 0 && door.y >= 46 * 16.0f - 0.1f &&
                         door.y + door.h >= 49 * 16.0f - 0.1f &&
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
    player.y = 49 * 16.0f - PLAYER_H;
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

    // 2b. Метатель ножей: медленный нож долетает и убивает.
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
    knives.clear();
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
    bool knifeThrown = false;
    for (int i = 0; i < 40 && !knifeThrown; ++i) {
        update(1.0f / 120.0f, Input{});
        if (!knives.empty()) knifeThrown = true;
    }
    bool knifeSlow = knifeThrown &&
                     std::fabs(knives.front().vx) < RUN_SPEED * 0.6f;
    for (int i = 0; i < 700 && state == ST_PLAY; ++i)
        update(1.0f / 120.0f, Input{});
    bool knifeKill = (state == ST_DYING && deathCause == DEATH_KNIFE);
    SDL_Log("knife: thrown=%d slow=%d kill=%d cause=%d",
            knifeThrown ? 1 : 0, knifeSlow ? 1 : 0, knifeKill ? 1 : 0, deathCause);
    state = ST_PLAY;

    // 3. Подбор кубка на верхней галерее: +1000 очков.
    monsters.clear();
    knives.clear();
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

    // 3b. Очки за убийство: зомби 100, паук 200, метатель 300.
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
    }
    bool killScore = (score == 100 + 200 + 300);
    bool popupOk = (popups.size() == popupBefore + 3 &&
                    popups[popupBefore].value == 100 &&
                    popups[popupBefore + 1].value == 200 &&
                    popups[popupBefore + 2].value == 300);
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

    // 5. Мгновенный выстрел дробовика по зомби.
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

    // 5c. Диагональный выстрел вверх: зомби висит над лучом.
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
        m.b.y = player.y - 45.0f;
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
        m.b.y = player.y + 50.0f;
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

    // 7. Смерть от зомби, три жизни и экран поражения.
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
                  deathCause == DEATH_ZOMBIE);
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

    // 8. Переход на второй уровень и финальная победа.
    resetWorld(0);
    monsters.clear();
    for (CupItem& c : cups) {
        c.taken = true;
        lvl.rows[c.ty][c.tx] = '.';
    }
    cupsGot = static_cast<int>(cups.size());
    player.x = 2496.0f;
    player.y = 754.0f;   // дверь теперь в подвале, на полу row 49
    player.vx = player.vy = 0;
    player.grounded = true;
    player.facing = 1;
    for (int i = 0; i < 200; ++i) update(1.0f / 120.0f, Input{});
    bool level2 = (levelIndex == 1 && state == ST_PLAY);
    SDL_Log("transition: level=%d state=%d", levelIndex, state);

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
    bool won = (state == ST_WIN);
    SDL_Log("win: state=%d kills=%d", state, kills);

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
    {
        Input in;
        in.back = true;
        update(1.0f / 120.0f, in);
    }
    bool escMenu = (state == ST_TITLE);
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
                  moveAimOk;
    SDL_Log("menu: start=%d settings=%d bind=%d/%d rebind=%d/%d "
            "back=%d esc=%d moveaim=%d/%d/%d ok=%d",
            menuStart ? 1 : 0, menuSettings ? 1 : 0, bindWaiting ? 1 : 0,
            bindWaiting2 ? 1 : 0, menuRebind ? 1 : 0, menuRebind2 ? 1 : 0,
            settingsBack ? 1 : 0, escMenu ? 1 : 0, moveAimUp ? 1 : 0,
            moveAimDown ? 1 : 0, moveAimFwd ? 1 : 0, menuOk ? 1 : 0);

    bool ok = startFirstFloor && doorBasement && climbed && camOk && stepHeld &&
              stairFall && spiderOk && spiderCrawl &&
              gotCup && killScore && popupOk && jewelScore &&
              popupRise && popupGone &&
              oneJumpOnly && doubleJump && jumpReset &&
              spaceWhileHoldingW && shotOk && recoilOk &&
              tracerOk && knifeThrown && knifeSlow && knifeKill &&
              diagUpOk && diagDownOk && recoilUpOk && recoilDownOk &&
              noAirShot && lookUpOk && lookDownOk &&
              lookForward && reloadOk && reloadWaitHeld && reloadStarted &&
              reloadProportional && reloadLocked &&
              reloadDone && dying && respawned && gameOver && level2 &&
              level2Stairs && won && menuOk;
    SDL_Log("smoke %s", ok ? "OK" : "FAIL");
}

} // namespace

// ---------------------------------------------------------------------------
//  main
// ---------------------------------------------------------------------------
int main(int argc, char** argv)
{
    bool screens = false;
    bool smoke = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--screens") == 0) screens = true;
        if (strcmp(argv[i], "--smoke") == 0) smoke = true;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    Uint32 winFlags = SDL_WINDOW_RESIZABLE;
    winFlags |= (screens || smoke) ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN;
    SDL_Window* win = SDL_CreateWindow(
        "Апартаменты Даймонд — Колян",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        960, 544, winFlags);
    if (!win) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    Uint32 rflags = SDL_RENDERER_ACCELERATED;
    if (!screens && !smoke) rflags |= SDL_RENDERER_PRESENTVSYNC;
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, rflags);
    if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(ren, VIEW_W, VIEW_H);
    SDL_RenderSetIntegerScale(ren, SDL_TRUE);

    Game game;
    if (screens || smoke) game.persistControls = false;
    if (!game.init(ren)) {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    if (screens) {
        game.makeScenarios();
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 0;
    }
    if (smoke) {
        game.runSmoke();
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 0;
    }

    Uint8 prevKeys[SDL_NUM_SCANCODES] = {};
    memcpy(prevKeys, SDL_GetKeyboardState(nullptr), SDL_NUM_SCANCODES);

    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 last = SDL_GetPerformanceCounter();
    double accumulator = 0.0;
    const double fixedStep = 1.0 / 120.0;
    bool running = true;

    while (running) {
        bool swallowMenu = false;
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_KEYDOWN && game.bindTarget >= 0) {
                SDL_Scancode sc = ev.key.keysym.scancode;
                if (sc == SDL_SCANCODE_ESCAPE) {
                    game.bindTarget = -1;      // отмена переназначения
                } else {
                    game.assignBind(sc);
                }
                // Клавиша, назначенная только что, не должна ещё и
                // сработать как навигация по меню в этом же кадре.
                swallowMenu = true;
            }
        }

        const Uint8* ks = SDL_GetKeyboardState(nullptr);
        auto held = [&](int action) {
            for (int s = 0; s < 2; ++s) {
                SDL_Scancode sc = game.controls.keys[action][s];
                if (sc != SDL_SCANCODE_UNKNOWN && ks[sc]) return true;
            }
            return false;
        };
        auto edge = [&](SDL_Scancode sc) { return ks[sc] && !prevKeys[sc]; };
        auto edgeAction = [&](int action) {
            for (int s = 0; s < 2; ++s) {
                SDL_Scancode sc = game.controls.keys[action][s];
                if (sc != SDL_SCANCODE_UNKNOWN && edge(sc)) return true;
            }
            return false;
        };

        Input in;
        in.left = held(CA_LEFT);
        in.right = held(CA_RIGHT);
        in.up = held(CA_UP);
        in.down = held(CA_DOWN);
        // Прицел идёт за направлением движения: вверх/вниз — те же клавиши.
        in.lookUp = in.up;
        in.lookDown = in.down;
        in.jumpHeld = held(CA_JUMP);

        in.spacePressed = edgeAction(CA_JUMP);
        in.shootPressed = edgeAction(CA_SHOOT);
        in.restart = edgeAction(CA_RESTART);
        // Меню всегда ходит на W/S и стрелках, подтверждение — Enter/Space,
        // в настройках A/D и ←/→ выбирают одну из двух кнопок, назад — Esc.
        in.menuUp = edge(SDL_SCANCODE_W) || edge(SDL_SCANCODE_UP);
        in.menuDown = edge(SDL_SCANCODE_S) || edge(SDL_SCANCODE_DOWN);
        in.menuLeft = edge(SDL_SCANCODE_A) || edge(SDL_SCANCODE_LEFT);
        in.menuRight = edge(SDL_SCANCODE_D) || edge(SDL_SCANCODE_RIGHT);
        in.confirm = edge(SDL_SCANCODE_RETURN) || edge(SDL_SCANCODE_SPACE);
        in.back = edge(SDL_SCANCODE_ESCAPE);
        if (swallowMenu)
            in.menuUp = in.menuDown = in.menuLeft = in.menuRight =
                in.confirm = in.back = false;

        memcpy(prevKeys, ks, SDL_NUM_SCANCODES);

        Uint64 now = SDL_GetPerformanceCounter();
        double dt = static_cast<double>(now - last) / static_cast<double>(freq);
        last = now;
        if (dt > 0.05) dt = 0.05;
        accumulator += dt;
        int guard = 0;
        bool firstStep = true;
        while (accumulator >= fixedStep && guard < 8) {
            Input stepIn = in;
            if (!firstStep)
                stepIn.menuUp = stepIn.menuDown = stepIn.menuLeft =
                    stepIn.menuRight = stepIn.confirm = stepIn.back = false;
            game.update(static_cast<float>(fixedStep), stepIn);
            firstStep = false;
            accumulator -= fixedStep;
            ++guard;
        }

        if (game.quitRequested) break;

        game.render();
        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
