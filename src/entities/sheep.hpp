#pragma once

#include "animal.hpp"

class Sheep : public Animal{
private:
    float myWalkSpeed;
    float myRunSpeed;
    float myVisionRadius;
    float myHungerThreshold;
    float myMaxHunger;
public:
    Sheep(Vector3 startPosition);

    ~Sheep() override = default;

    void Update(float deltaTime, World* world) override;
    void Draw() override;

    float stateTimer;
    Entity* targetHunter;
};