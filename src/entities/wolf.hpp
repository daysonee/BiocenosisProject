#pragma once
#include "animal.hpp"
#include "../core/constants.hpp"

class Sheep;
class World;

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

    // Сигнал «я только что умер от старости» — обрабатывается
    // на следующем тике для выбора нового вожака
    bool diedOfOldAge = false;

    static bool IsNarrowWater(float fromX, float fromZ,
                               float dirX,  float dirZ,
                               World* world);

    void MoveSwimming(float deltaTime, World* world);
    void TryStartPounce(float distToPrey, World* world);
    void UpdateAge(float deltaTime, World* world);
    void TryAttemptMating(float deltaTime, World* world);

    float CurrentPounceReach() const;
    float CurrentLeadTime()   const;
    float CurrentSizeFactor() const;
    float CurrentSpeedFactor() const;
    float FullCooldown() const;

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

    bool CanMate() const;
    void SetMateTarget(Wolf* partner) { mateTarget = partner; }

    // Промоушен в вожаки (вызывается извне когда выбран новый лидер)
    void PromoteToLeader();
};