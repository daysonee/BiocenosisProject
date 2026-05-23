#pragma once

#include "animal.hpp"

class Sheep : public Animal{
private:
    float myWalkSpeed;
    float myRunSpeed;
    float myVisionRadius;
    float myHungerThreshold;
    float myMaxHunger;

    Vector3 wanderTarget = { 0.0f, 0.0f, 0.0f }; // Личная цель блуждания
    float wanderTimer = 0.0f;                    // Таймер до смены цели
    bool hasWanderTarget = false;                // Есть ли сейчас цель
    Vector3 flockCenter; // Центр стада, вокруг которого гуляет овечка
public:
    bool isMating = false; //(спаривание)
    
    Sheep(Vector3 startPosition);

    ~Sheep() override = default;

    void Update(float deltaTime, World* world) override;
    void Draw() override;

    float stateTimer;
    Entity* targetHunter;
};