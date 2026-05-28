#pragma once
#include "animal.hpp"
#include "../core/constants.hpp"

class Sheep;
class World;
class Carcass;

// ─── Состояния волка ─────────────────────────────────────────────────────────
// IDLE     — сытый, в лесу, отдыхает / бродит рядом с homePos
// WANDERING — бродит по лесу в поисках партнёра для размножения
// HUNTING  — голодный, вышел на луга охотиться
// MATING   — идёт к партнёру / спаривается
// FIGHTING — бой (с другой стаей или с отказавшим чужим волком)

class Wolf : public Animal {
private:
    // ── Охота ────────────────────────────────────────────────────────────────
    Sheep*   targetPrey        = nullptr;
    float    pounceTimer       = 0.0f;
    float    pounceCooldownTimer = 0.0f;
    Vector3  prevPreyPos       = {};
    bool     hasPrevPreyPos    = false;

    // ── Возраст ──────────────────────────────────────────────────────────────
    Config::Wolf::AgeStage ageStage;
    float maxAgeInCurrentStage = 0.0f;
    float ageTimer             = 0.0f;
    int   babyKillCount        = 0;

    // ── Плавание / встряхивание ───────────────────────────────────────────────
    bool  wasInWater      = false;
    float shakeTimer      = 0.0f;
    float swimSplashTimer = 0.0f;

    // ── Размножение ───────────────────────────────────────────────────────────
    float matingCooldownTimer  = 0.0f;
    float matingProgressTimer  = 0.0f;
    Wolf* mateTarget           = nullptr;

    // ── Стая ─────────────────────────────────────────────────────────────────
    int     packId   = 0;
    bool    isLeader = false;
    Vector3 homePos  = {};

    // ── Бой ───────────────────────────────────────────────────────────────────
    Wolf* fightTarget        = nullptr;
    float fightHealth        = 100.0f;
    float fightCooldownTimer = 0.0f;

    // ── Голод / смерть ────────────────────────────────────────────────────────
    float starvationTimer  = 0.0f;

    // ── Флаги смерти (для world.cpp) ─────────────────────────────────────────
    bool diedOfOldAge = false;
    bool diedInFight  = false;

    // ── Застревание ───────────────────────────────────────────────────────────
    float   stuckCheckTimer = 0.0f;
    Vector3 posAtLastCheck  = {};
    int     stuckCount      = 0;
    float   idleTimer       = 0.0f;  // таймер отдыха в IDLE

    // ── Отрисовка ─────────────────────────────────────────────────────────────
    float   facingAngle = 0.0f;
    Vector3 lastDrawPos = {};

    // ── Внутренние методы ─────────────────────────────────────────────────────
    void UpdateAge(float dt, World* world);
    void MoveSwimming(float dt, World* world);
    void TryStartPounce(float distToPrey, World* world);

    // Поиск
    Sheep*   FindNearestSheepInRadius(World* world, float radius) const;
    Wolf*    FindNearestOtherWolf(World* world, float radius) const; // любой чужой волк
    Carcass* FindNearestCarcassInRadius(World* world, float radius) const;

    // Вспомогательные
    float CurrentPounceReach()  const;
    float CurrentLeadTime()     const;
    float CurrentSizeFactor()   const;
    float CurrentSpeedFactor()  const;
    float FullCooldown()        const;
    float CurrentHuntingRadius() const;
    bool  IsInForest(World* world) const;   // высота между SAND_LEVEL и BIOME_THRESHOLD
    void  PickForestWanderTarget(World* world); // точка в лесу рядом с homePos
    void  PickMeadowHuntTarget(World* world);   // точка на лугах

    // Состояния
    void UpdateIdle    (float dt, World* world);
    void UpdateWandering(float dt, World* world);
    void UpdateHunting (float dt, World* world);
    void UpdateMating  (float dt, World* world);
    void UpdateFighting(float dt, World* world);

public:
    bool isMating = false;

    Wolf(Vector3 startPos,
         Config::Wolf::AgeStage startStage = Config::Wolf::AgeStage::ADULT,
         int wolfPackId = 0);
    ~Wolf() override = default;

    void Update(float dt, World* world) override;
    void Draw() override;
    Color GetDeathColor() const override { return DARKGRAY; }

    // Геттеры
    Config::Wolf::AgeStage GetAgeStage() const { return ageStage; }
    int     GetPackId()      const { return packId; }
    bool    IsLeader()       const { return isLeader; }
    bool    DiedOfOldAge()   const { return diedOfOldAge; }
    bool    DiedInFight()    const { return diedInFight; }
    Vector3 GetHomePos()     const { return homePos; }

    // Сеттеры
    void SetPackId (int id)      { packId  = id; }
    void SetHomePos(Vector3 hp)  { homePos = hp; }
    void PromoteToLeader();

    bool CanMate() const {
        return (ageStage == Config::Wolf::AgeStage::ADULT ||
                ageStage == Config::Wolf::AgeStage::MEDIUM)
            && hunger              >= Config::Wolf::MATING_HUNGER_THRESHOLD
            && matingCooldownTimer <= 0.0f
            && mateTarget          == nullptr
            && !isMating
            && state               != AnimalState::FIGHTING
            && isAlive;
    }
    void SetMateTarget(Wolf* w) { mateTarget = w; }

    void OnEntityDying(Entity* dying) override {
        if (targetPrey  == (Sheep*)dying) { targetPrey = nullptr; hasPrevPreyPos = false; }
        if (mateTarget  == (Wolf*) dying) {
            mateTarget          = nullptr;
            isMating            = false;
            matingProgressTimer = 0.0f;
        }
        if (fightTarget == (Wolf*) dying) fightTarget = nullptr;
    }
};