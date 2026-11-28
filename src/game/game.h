#pragma once

#include "core/constants.h"
#include "core/types.h"
#include "core/level.h"
#include "core/input.h"
#include "input/controls.h"
#include "input/touch_controls.h"
#include <SDL.h>
#include <cmath>
#include <string>
#include <vector>

namespace kolek {

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
    std::vector<Bullet> bullets;
    std::vector<Grenade> grenades;
    std::vector<Light> lights;
    std::vector<Monster> monsters;

    RectF door{};
    RectF checkpoint{};
    int state = ST_TITLE;
    Controls controls;
    TouchControls touch;
    int menuIndex = 0;
    int settingsIndex = 0;
    int settingsSlot = 0;         // 0 или 1 — какую из двух клавиш меняем
    int bindTarget = -1;          // действие, ждущее новую клавишу
    int bindSlot = 0;             // слот переназначаемой клавиши
    bool quitRequested = false;   // «Выход» в главном меню
    ExitAction exitAction = EXIT_NONE;
    bool exitAccepted = false;    // по умолчанию выбрана отмена
    bool persistControls = true;  // не трогать файл в служебных режимах

    // Слайды между уровнями.
    std::vector<std::string> slideTexts;
    int slideIndex = 0;
    int slideNextLevel = -1;
    bool slideWallScene = false;  // слайд-«фотография» стены с надписью

    // DEV: секретная дверь и экран выбора уровня (Ctrl+R).
    RectF secretDoor{};
    bool secretDoorActive = false;
    int levelSelectIndex = 0;
    int levelIndex = 0;
    int cupsGot = 0;
    int kills = 0;
    int score = 0;
    int lives = START_LIVES;
    int deathCause = DEATH_SPIDER;
    float deathTimer = 0;
    bool bossDefeated = false;    // босс парковки повержен

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
    void requestExit(ExitAction action);
    void render();

    // --- геймплей -----------------------------------------------------------
    void updatePlayer(float dt, const Input& in);
    void updateMonsters(float dt);
    void updateSpider(Monster& m, float dt, float dxp);
    void updateBullets(float dt);
    void updateGrenades(float dt);
    void throwBossGrenade(const Body& b);
    void explodeGrenade(const Grenade& g);
    void updateWeapon(float dt, const Input& in);
    void updateParticles(float dt);
    void updateTracers(float dt);
    void updateCamera(float dt);
    void snapCamera();
    void updateDeath(float dt);
    void checkPickups();
    void checkDoor(float dt);
    void advanceLevel();
    void beginSlides(int nextLevel);
    void toggleSecretDoor();
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
    void drawTechnicalWallpaper();
    void drawLobbyWallpaper();
    void drawParkingWallpaper();
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
    void drawEmergencyLamp(int wx, int wy);
    void drawStepLadder(int wx, int wy);
    void drawTechPipes(int wx, int wy);
    void drawTechVent(int wx, int wy);
    void drawTechBox(int wx, int wy);
    void drawTechSign(int wx, int wy);
    void drawCableCoil(int wx, int wy);
    void drawDoor();
    void drawElevatorDoor();
    void drawSecretDoor();
    void drawCups();
    void drawJewelry();
    void drawJewelIcon(int x, int y, int type, float bob);
    void drawMonsters();
    void drawSpider(const Monster& m);
    void drawBatThug(const Monster& m);
    void drawPistolThug(const Monster& m);
    void drawDoberman(const Monster& m);
    void drawBoss(const Monster& m);
    void drawBullets();
    void drawGrenades();
    void drawGuy(const Body& b, const Palette& p);
    void drawParticles();
    void drawTracers();
    void drawGlows();
    void drawVignette();
    void drawScorePopups();
    void drawHUD();
    void drawTouchControls();
    void drawDeathOverlay();
    void drawTitleScreen();
    void drawSettingsScreen();
    void drawSlideScreen();
    void drawSlideWall();
    void drawLevelSelectScreen();
    void drawWinScreen();
    void drawGameOverScreen();
    void drawExitConfirmation();

    // --- служебное ----------------------------------------------------------
    void screenshot(const char* path);
    void makeScenarios();
    void runSmoke();
};

} // namespace kolek
