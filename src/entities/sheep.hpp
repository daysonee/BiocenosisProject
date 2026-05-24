#pragma once

#include "animal.hpp"

class Sheep : public Animal {
private:
    float myWalkSpeed;
    float myRunSpeed;
    float myVisionRadius;
    float myHungerThreshold;
    float myMaxHunger;

    // Центр родного стада — овечка держится рядом
    Vector3 flockCenter;

    // Таймер между переходами IDLE -> WANDERING
    float stateTimer = 0.0f;

    // Указатель на охотника (волка). nullptr если нет угрозы
    Entity* targetHunter = nullptr;

    // --- Детектор застревания ---
    float   stuckCheckTimer   = 0.0f;   // до следующей проверки
    Vector3 posAtLastCheck    = {};     // позиция на момент последней проверки
    int     stuckCount        = 0;     // сколько интервалов подряд не двигались

    // Подбирает случайную СУХУЮ точку вблизи центра стада
    void PickNewWanderTarget(World* world);
    // Аварийный выход из кучи: толчок + новая цель
    void ForceEscape(World* world);

public:
    bool isMating = false;

    Sheep(Vector3 startPosition);
    ~Sheep() override = default;

    void Update(float deltaTime, World* world) override;
    void Draw() override;

    void SetFlockCenter(Vector3 center) { flockCenter = center; }
};