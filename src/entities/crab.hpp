#pragma once

#include "animal.hpp"
#include "../core/constants.hpp"

class Crab : public Animal {
private:
    float stateTimer = 0.0f;
    float wanderRadius = 25.0f;

    void PickNewBeachTarget(World* world);
    
    bool IsOnBeach(float height) const;

public:
    Crab(Vector3 startPosition);
    ~Crab() override = default;

    void Update(float deltaTime, World* world) override;
    void Draw() override;
    
    Color GetDeathColor() const override { return ORANGE; }
};