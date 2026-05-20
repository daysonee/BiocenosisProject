#pragma once
#include "animal.hpp"

class Wolf : public Animal{
private:
    Sheep* targetPrey;

public:
    Wolf(Vector3 startPosition);
    ~Wolf() override = default;

    void Update(float deltaTime, World* world) override;
    void Draw() override;
};