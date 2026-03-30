#pragma once

#include "animal.hpp"

class Sheep : public Animal{
public:
    Sheep(Vector3 startPosition);

    ~Sheep() override = default;

    void Update(float deltaTime) override;
    void Draw() override;

    float stateTimer;
};