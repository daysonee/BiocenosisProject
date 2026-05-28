#pragma once
 
#include "animal.hpp"
#include "../core/constants.hpp"
 
class Carcass;
 
// Краб живёт на пляже, ест трупы овец, размножается когда наелся.
// Несколько крабов могут есть один труп одновременно.
// Умирает от старости.
class Crab : public Animal {
private:
    // ── Таймеры / состояние ──────────────────────────────────────────
    float stateTimer    = 0.0f;
    float ageTimer      = 0.0f;
    float lifespan      = 0.0f;   // рандомизируется в конструкторе
 
    // ── Питание ─────────────────────────────────────────────────────
    Carcass* targetCarcass = nullptr;   // текущий труп, к которому идёт/ест
 
    // ── Размножение ──────────────────────────────────────────────────
    float matingCooldownTimer = 0.0f;
    float matingProgressTimer = 0.0f;
    Crab*  mateTarget         = nullptr;
    bool   isMating           = false;

    // ── Поворот в направлении движения ──────────────────────────────
    float facingAngle = 0.0f;

    // Индивидуальный таймер страховки пляжа
    float beachCheckTimer = 0.0f;

    // Контроль популяции
    float starvationTimer  = 0.0f;  // сколько секунд на нуле hunger
    float crowdCheckTimer  = 0.0f;  // таймер проверки перенаселения
 
    // ── Вспомогательные ─────────────────────────────────────────────
    bool IsOnBeach(float height, World* world) const;
    void PickNewBeachTarget(World* world);
    Carcass* FindNearestCarcass(World* world) const;
 
public:
    Crab(Vector3 startPosition);
    ~Crab() override = default;
 
    void Update(float deltaTime, World* world) override;
    void Draw() override;
 
    // Очистка dangling-указателей
    void OnEntityDying(Entity* dying) override {
        if (targetCarcass == reinterpret_cast<Carcass*>(dying)) targetCarcass = nullptr;
        if (mateTarget    == reinterpret_cast<Crab*>   (dying)) mateTarget    = nullptr;
    }
 
    bool CanMate() const {
        return hunger        >= Config::Crab::MATING_HUNGER_THRESHOLD
            && matingCooldownTimer <= 0.0f
            && mateTarget    == nullptr
            && !isMating
            && isAlive;
    }
    void SetMateTarget(Crab* partner) { mateTarget = partner; }
 
    Color GetDeathColor() const override { return ORANGE; }
};