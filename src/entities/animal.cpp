#include "animal.hpp"
#include "raymath.h"
#include "../core/world.hpp"
#include "../core/constants.hpp"

Animal::Animal(Vector3 startPosition) : Entity(startPosition) {
    health         = 100.0f;
    hunger         = 100.0f;
    speed          = 2.0f;
    state          = AnimalState::IDLE;
    targetPosition = startPosition;
}

// Пробует переместить животное к цели с обходом воды в стиле Minecraft.
bool Animal::MoveTowardsTarget(float deltaTime, World* world) {
    // Защита от null world (важно для тестов и edge cases)
    if (!world) return false;

    Vector3 direction = {
        targetPosition.x - position.x,
        0.0f,
        targetPosition.z - position.z
    };

    float distance = Vector3Length(direction);
    if (distance <= 0.1f) {
        if (world) position.y = world->GetHeight(position.x, position.z);
        return true;
    }

    Vector3 normDir  = Vector3Normalize(direction);
    float   stepDist = speed * deltaTime;
    float   dx       = normDir.x * stepDist;
    float   dz       = normDir.z * stepDist;

    // Текущий уровень воды (с учётом прилива)
    float waterLvl = world->GetCurrentWaterLevel();

    // Границы карты — животное никогда не выходит за пределы
    const int halfMap = Config::World::MAP_SIZE / 2;
    const float minB = -(float)halfMap + 2.0f;
    const float maxB =  (float)halfMap - 2.0f;

    bool moved = false;

    // --- Попытка 1: полный шаг ---
    {
        float nx = Clamp(position.x + dx, minB, maxB);
        float nz = Clamp(position.z + dz, minB, maxB);
        if (world->GetHeight(nx, nz) > waterLvl) {
            position.x = nx;
            position.z = nz;
            moved = true;
        }
    }

    // --- Попытка 2: скольжение по X ---
    if (!moved && fabsf(dx) > 0.001f) {
        float nx = Clamp(position.x + dx, minB, maxB);
        if (world->GetHeight(nx, position.z) > waterLvl) {
            position.x = nx;
            moved = true;
        }
    }

    // --- Попытка 3: скольжение по Z ---
    if (!moved && fabsf(dz) > 0.001f) {
        float nz = Clamp(position.z + dz, minB, maxB);
        if (world->GetHeight(position.x, nz) > waterLvl) {
            position.z = nz;
            moved = true;
        }
    }

    if (moved && world != nullptr) {
        position = world->ResolveTreeCollisions(position, 0.4f);
        // На случай если ResolveTreeCollisions вытолкнул за границу
        position.x = Clamp(position.x, minB, maxB);
        position.z = Clamp(position.z, minB, maxB);
    }

    if (world) position.y = world->GetHeight(position.x, position.z);

    return moved;
}