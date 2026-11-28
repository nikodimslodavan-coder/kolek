#pragma once

namespace kolek {

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
constexpr float AIM_DIAG       = 0.5235987756f; // 30°: наклон прицела вверх/вниз
constexpr float RECOIL_KICK    = 180.0f; // отдача толкает Коляна назад
constexpr int   START_LIVES    = 3;
constexpr float BULLET_SPEED  = 69.2f * 1.2f;  // +20% к прежним 69.2 px/s
constexpr float PISTOL_THUG_SPEED = BULLET_SPEED * 0.9f;  // на 10% медленнее пули
// Пуля обязана быть медленнее Коляна, иначе от неё нельзя уйти.
static_assert(BULLET_SPEED < RUN_SPEED, "bullet must be slower than Kolya");
static_assert(PISTOL_THUG_SPEED < BULLET_SPEED,
              "pistol thug must be slower than his bullet");
constexpr float DEATH_TIME     = 2.6f;   // пауза с анимацией смерти
constexpr float CHASE_RADIUS   = 34.0f;  // «два шага» до Коляна
constexpr float PLAYER_H       = 30.0f;  // Колян в полтора раза выше прежнего

// DEV-чит: Ctrl+R создаёт секретную дверь в выбор уровней.
// Перед релизом достаточно поставить false и убрать экран выбора.
constexpr bool DEV_SECRET_LEVELS = true;

constexpr int   CUP_SCORE      = 1000;   // кубок-трофей
constexpr int   BOSS_SCORE     = 5000;   // гигантский браток на парковке
constexpr int   BOSS_HP        = 70;     // 10 залпов по 7 дробин
constexpr float BOSS_W         = 32.0f;  // в 2.5 раза меньше прежнего гиганта
constexpr float BOSS_H         = 96.0f;  // 80 x 240 -> 32 x 96
constexpr float BOSS_SPEED     = 65.0f;  // в 2.5 раза быстрее прежних 26 px/s
constexpr float BOSS_THROW_CD  = 2.6f;   // пауза между гранатами
constexpr float BOSS_BLAST_R   = 36.0f;  // радиус взрыва гранаты
constexpr float JEEP_X         = 396.0f; // джип на парковке — роль двери
constexpr float JEEP_W         = 72.0f;  // зона, где Колян садится в машину
constexpr float JEEP_DOOR_H    = 32.0f;
// паук, браток с битой, браток с пистолетом, доберман, босс
constexpr int   KILL_SCORE[5]  = {200, 100, 300, 250, BOSS_SCORE};
constexpr int   JEWEL_SCORE[3] = {200, 400, 600};   // три вида украшений

enum GameState {
    ST_TITLE = 0, ST_PLAY = 1, ST_WIN = 2, ST_DYING = 3, ST_GAMEOVER = 4,
    ST_SETTINGS = 5, ST_SLIDE = 6, ST_LEVEL_SELECT = 7
};
enum DeathCause {
    DEATH_SPIDER = 0, DEATH_BAT = 1, DEATH_BULLET = 2, DEATH_DOBERMAN = 3,
    DEATH_FALL = 4, DEATH_BOSS = 5, DEATH_GRENADE = 6
};

enum ExitAction { EXIT_NONE, EXIT_LEVEL, EXIT_GAME };

} // namespace kolek
