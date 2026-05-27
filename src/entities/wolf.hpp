#pragma once
#include "animal.hpp"
#include "../core/constants.hpp"

class Sheep;
class World;
class Carcass;

class Wolf : public Animal {
private:
    Sheep* targetPrey;

    // Прыжок-рывок
    float pounceTimer         = 0.0f;
    float pounceCooldownTimer = 0.0f;

    // Упреждение
    Vector3 prevPreyPos    = { 0.0f, 0.0f, 0.0f };
    bool    hasPrevPreyPos = false;

    // Возраст
    Config::Wolf::AgeStage ageStage;
    float maxAgeInCurrentStage = 0.0f;
    float ageTimer = 0.0f;

    // BABY: счётчик съеденных овец в текущем «голодном цикле»
    int babyKillCount = 0;

    // Плавание / встряхивание
    bool  wasInWater     = false;
    float shakeTimer     = 0.0f;
    float swimSplashTimer = 0.0f;

    // Размножение
    float matingCooldownTimer = 0.0f;
    float matingProgressTimer = 0.0f;
    Wolf* mateTarget          = nullptr;

    // Стая
    int  packId   = 0;
    bool isLeader = false;

    // Дом стаи — ОБЩИЙ центр для всех волков стаи, к которому
    // привязан патруль. Передаётся при спавне (SetHomePos).
    Vector3 homePos = { 0.0f, 0.0f, 0.0f };

    // Смерть от голода
    float starvationTimer = 0.0f;

    // После еды травы голод падает быстрее GRASS_EFFECT_DURATION секунд
    float grassEffectTimer = 0.0f;

    // Сигнал «я только что умер от старости» — обрабатывается
    // на следующем тике для выбора нового вожака
    bool diedOfOldAge = false;
    // Сигнал «я погиб в бою» — обрабатывается для мерджа стаи
    bool diedInFight = false;

    // Конфликт стай
    Wolf* fightTarget       = nullptr;
    float fightHealth       = 100.0f;
    float fightCooldownTimer = 0.0f;

    static bool IsNarrowWater(float fromX, float fromZ,
                               float dirX,  float dirZ,
                               World* world);

    void MoveSwimming(float deltaTime, World* world);
    void TryStartPounce(float distToPrey, World* world);
    void UpdateAge(float deltaTime, World* world);
    void TryAttemptMating(float deltaTime, World* world);

    // Чистая state machine — каждое состояние = отдельный метод
    void PickNewPatrolTarget(World* world);
    void UpdateIdle(float dt, World* world);
    void UpdateWander(float dt, World* world);
    void UpdateHunt(float dt, World* world);
    void UpdateFight(float dt, World* world);

    // Поиск
    Sheep*   FindNearestSheepInRadius(World* world, float radius) const;
    Wolf*    FindEnemyWolfNearby(World* world) const;
    Carcass* FindNearestCarcassInRadius(World* world, float radius) const;

    float CurrentPounceReach() const;
    float CurrentLeadTime()   const;
    float CurrentSizeFactor() const;
    float CurrentSpeedFactor() const;
    float FullCooldown() const;
    float CurrentHuntingRadius() const;

    float   stuckCheckTimer  = 0.0f;
    Vector3 posAtLastCheck   = { 0.0f, 0.0f, 0.0f };
    int     stuckCount       = 0;
    float   wanderTargetTimer = 0.0f;

    // ── НАПРАВЛЕНИЕ ВЗГЛЯДА ──────────────────────────────────
    float   facingAngle    = 0.0f;
    Vector3 lastDrawPos    = { 0.0f, 0.0f, 0.0f };

public:
    bool isMating = false;

    Wolf(Vector3 startPosition,
         Config::Wolf::AgeStage startStage = Config::Wolf::AgeStage::ADULT,
         int wolfPackId = 0);

    ~Wolf() override = default;

    void Update(float deltaTime, World* world) override;
    void Draw() override;
    Color GetDeathColor() const override { return DARKGRAY; }

    Config::Wolf::AgeStage GetAgeStage() const { return ageStage; }
    int  GetPackId() const { return packId; }
    bool IsLeader()  const { return isLeader; }
    bool DiedOfOldAge() const { return diedOfOldAge; }
    bool DiedInFight() const  { return diedInFight; }

    bool CanMate() const;
    void SetMateTarget(Wolf* partner) { mateTarget = partner; }

    // Передаётся при спавне стаи — общий центр для всех её членов
    void SetHomePos(Vector3 hp) { homePos = hp; }
    Vector3 GetHomePos() const  { return homePos; }

    // Слияние стай: смена packId (бэйби переходят к победителю)
    void SetPackId(int newPackId) { packId = newPackId; }

    void PromoteToLeader();

    // Очистка dangling pointers перед удалением dying-сущности
    void OnEntityDying(Entity* dying) override {
        if (targetPrey  == (Sheep*)dying) { targetPrey = nullptr; hasPrevPreyPos = false; }
        if (mateTarget  == (Wolf*) dying)   mateTarget  = nullptr;
        if (fightTarget == (Wolf*) dying)   fightTarget = nullptr;
    }
};