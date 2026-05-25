#pragma once
#include "animal.hpp"

class Sheep;
class World;

class Wolf : public Animal {
private:
    Sheep* targetPrey;

    static bool IsNarrowWater(float fromX, float fromZ,
                               float dirX,  float dirZ,
                               World* world);

    void MoveWithWaterCrossing(float deltaTime, World* world);

public:
    Wolf(Vector3 startPosition);
    ~Wolf() override = default;

    void Update(float deltaTime, World* world) override;
    void Draw() override;
    Color GetDeathColor() const override { return DARKGRAY; }
};