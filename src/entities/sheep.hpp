#pragma once

#include "animal.hpp"
#include "../core/constants.hpp"

class Sheep : public Animal {
private:
    float myWalkSpeed;
    float myRunSpeed;
    float myVisionRadius;
    float myHungerThreshold;
    float myMaxHunger;

    Vector3 flockCenter;

    float stateTimer = 0.0f;
    Entity* targetHunter = nullptr;

    // Тактика побега
    float zigzagTimer        = 0.0f;
    float zigzagAngle        = 0.0f;   // радианы, текущее отклонение
    float dodgeCooldownTimer = 0.0f;

    // После финта направление "залочено" — овца держит курс
    // вбок несколько тиков, чтобы волк не успел скомпенсировать
    Vector3 lockedFleeDir    = { 0.0f, 0.0f, 0.0f };
    float   lockedFleeTimer  = 0.0f;
    float   dodgeBoostTimer  = 0.0f;   // ускорение сразу после финта

    float   stuckCheckTimer = 0.0f;
    Vector3 posAtLastCheck  = {};
    int     stuckCount      = 0;

    // Каждые ~5 сек овца пересчитывает свой flockCenter по соседям.
    // Это позволяет перемещаться между стадами после побега.
    float   flockUpdateTimer = 5.0f;

    // ── РЕАКЦИЯ НА ПРИЛИВ ────────────────────────────────────
    // tideCheckTimer  — раз в секунду проверяем угрозу
    // tideFrozen      — если true, овца "замерла" и тонет (не двигается)
    // tideEvalDone    — однократная роль ставки замирания после первого триггера
    float tideCheckTimer = 0.0f;
    bool  tideFrozen     = false;
    bool  tideEvalDone   = false;

    // ── НАПРАВЛЕНИЕ ВЗГЛЯДА ──────────────────────────────────
    // Запоминается при движении и используется в Draw для поворота модели.
    float facingAngle = 0.0f;
    Vector3 lastPosition = { 0.0f, 0.0f, 0.0f };

    bool playerControlled = false;
    Vector3 controlForward = { 0.0f, 0.0f, 1.0f };
    Vector3 controlRight = { 1.0f, 0.0f, 0.0f };
    void UpdatePlayerControl(float deltaTime, World* world);

    void PickNewWanderTarget(World* world);
    void ForceEscape(World* world);

    Config::Sheep::AgeStage ageStage;
    float maxAgeInCurrentStage = 0.0f;
    float ageTimer = 0.0f;

    float matingCooldownTimer  = 0.0f;
    float matingProgressTimer  = 0.0f;
    Sheep* mateTarget          = nullptr;

public:
    bool isMating = false;

    ~Sheep() override = default;

    Sheep(Vector3 startPosition,
          Config::Sheep::AgeStage startStage = Config::Sheep::AgeStage::ADULT);

    Config::Sheep::AgeStage GetAgeStage() const { return ageStage; }

    bool CanMate() const {
        return ageStage      == Config::Sheep::AgeStage::ADULT
            && hunger        >= Config::Sheep::MATING_HUNGER_THRESHOLD
            && matingCooldownTimer <= 0.0f
            && mateTarget    == nullptr
            && !isMating
            && isAlive;
    }

    void SetMateTarget(Sheep* partner) { mateTarget = partner; }

    void Update(float deltaTime, World* world) override;
    void Draw() override;

    void SetFlockCenter(Vector3 center) { flockCenter = center; }

    void SetPlayerControlled(bool enabled);
    bool IsPlayerControlled() const;
    void SetPlayerAim(Vector3 forward, Vector3 right);
    Color GetDeathColor() const override { return LIGHTGRAY; }

    // Очистка dangling pointers перед удалением dying-сущности
    void OnEntityDying(Entity* dying) override {
        if (targetHunter == dying) targetHunter = nullptr;
        if (mateTarget == dying)   mateTarget   = nullptr;
    }
};