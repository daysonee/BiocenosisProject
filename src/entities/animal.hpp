#pragma once

#include "../core/entity.hpp"

class World;

enum class AnimalState{
    IDLE,
    WANDERING,
    HUNGRY,
    FLEEING
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

    void MoveTowardsTarget(float deltaTime, World* world);
};