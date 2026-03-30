#pragma once

#include "../core/entity.hpp"

enum class AnimalState{
    IDLE,
    WANDERING,
    HUNGRY
};

class Animal : public Entity{
protected:
    float health;
    float speed;
    float hunger;
    AnimalState state;
    Vector3 targetPosition;

public:
    Animal(Vector3 startPosition);
    virtual ~Animal() override = default;

    void MoveTowardsTarget(float deltaTime);
};