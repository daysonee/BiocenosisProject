#pragma once

#include "animal.hpp"

class Sheep : public Animal{
public:
    Sheep(Vector3 startPosition);

    ~Sheep() override = default;

    void Update(float deltaTime, World* world) override;
    void Draw() override;

    float stateTimer;
    Entity* targetHunter;
};