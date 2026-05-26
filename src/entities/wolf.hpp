#pragma once
#include "animal.hpp"

class Sheep;
class World;

class Wolf : public Animal {
private:
    Sheep* targetPrey;

    // Прыжок-рывок
    float pounceTimer         = 0.0f;  // > 0 — сейчас в прыжке
    float pounceCooldownTimer = 0.0f;  // > 0 — нельзя прыгать

    static bool IsNarrowWater(float fromX, float fromZ,
                               float dirX,  float dirZ,
                               World* world);

    void MoveWithWaterCrossing(float deltaTime, World* world);
    void TryStartPounce(float distToPrey, World* world);

public:
    Wolf(Vector3 startPosition);
    ~Wolf() override = default;

    void Update(float deltaTime, World* world) override;
    void Draw() override;
    Color GetDeathColor() const override { return DARKGRAY; }
};