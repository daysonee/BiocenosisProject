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
// Алгоритм:
//   1. Пробуем полный шаг (dx, dz).
//   2. Если в воде — пробуем только X-составляющую (скольжение вдоль берега по Z).
//   3. Если тоже вода — пробуем только Z-составляющую (скольжение вдоль берега по X).
//   4. Если оба варианта в воде — возвращаем false (животное заблокировано).
// Всегда корректируем Y по рельефу.
bool Animal::MoveTowardsTarget(float deltaTime, World* world) {

    // Игнорируем разницу по Y при выборе направления — двигаемся строго в горизонтали
    Vector3 direction = {
        targetPosition.x - position.x,
        0.0f,
        targetPosition.z - position.z
    };

    float distance = Vector3Length(direction);
    if (distance <= 0.1f) {
        // Уже у цели
        if (world) position.y = world->GetHeight(position.x, position.z);
        return true;
    }

    Vector3 normDir  = Vector3Normalize(direction);
    float   stepDist = speed * deltaTime;
    float   dx       = normDir.x * stepDist;
    float   dz       = normDir.z * stepDist;

    bool moved = false;

    // --- Попытка 1: полный шаг ---
    {
        float nx = position.x + dx;
        float nz = position.z + dz;
        if (world->GetHeight(nx, nz) > Config::World::WATER_LEVEL) {
            position.x = nx;
            position.z = nz;
            moved = true;
        }
    }

    // --- Попытка 2: скольжение по X (сохраняем Z) ---
    if (!moved && fabsf(dx) > 0.001f) {
        float nx = position.x + dx;
        if (world->GetHeight(nx, position.z) > Config::World::WATER_LEVEL) {
            position.x = nx;
            moved = true;
        }
    }

    // --- Попытка 3: скольжение по Z (сохраняем X) ---
    if (!moved && fabsf(dz) > 0.001f) {
        float nz = position.z + dz;
        if (world->GetHeight(position.x, nz) > Config::World::WATER_LEVEL) {
            position.z = nz;
            moved = true;
        }
    }

    if (moved && world != nullptr) {
        // Выталкиваем животное из стволов (0.4f - радиус тела из констант овцы)
        position = world->ResolveTreeCollisions(position, 0.4f);
    }

    // Корректируем высоту по рельефу в любом случае
    if (world) position.y = world->GetHeight(position.x, position.z);

    return moved;
}