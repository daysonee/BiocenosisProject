#pragma once

#include "../core/entity.hpp"

class World;

enum class AnimalState {
    IDLE,
    WANDERING,
    HUNGRY,
    FLEEING,
    FIGHTING,   // волки: конфликт стай
    HUNTING,    // волки: активная охота на лугах
    MATING      // волки: поиск/процесс спаривания
};

class Animal : public Entity {
protected:
    float health;
    float speed;
    float hunger;
    AnimalState state;
    Vector3 targetPosition;

public:
    Animal(Vector3 startPosition);
    virtual ~Animal() override = default;

    // Возвращает true если движение удалось, false если полностью заблокировано водой.
    // Реализует Minecraft-скольжение: при блокировке пробует двигаться вдоль одной из осей.
    bool MoveTowardsTarget(float deltaTime, World* world);
};