#include "crab.hpp"
#include "../core/world.hpp"
#include <raymath.h>

Crab::Crab(Vector3 startPosition) : Animal(startPosition) {
    // Крабы медленнее овец и волков
    speed = 1.5f; 
    hunger = 100.0f;
    state = AnimalState::WANDERING;
    targetPosition = startPosition;
    stateTimer = (float)GetRandomValue(1, 3);
}

bool Crab::IsOnBeach(float height) const {
    // Пляж находится между уровнем воды и уровнем начала биома лугов
    return (height >= Config::World::WATER_LEVEL - 0.2f && 
            height <= Config::World::SAND_LEVEL + 0.5f);
}

void Crab::PickNewBeachTarget(World* world) {
    const int halfMap = Config::World::MAP_SIZE / 2;
    
    // Пытаемся найти подходящую точку на песке неподалёку
    for (int attempt = 0; attempt < 20; ++attempt) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float radius = (float)GetRandomValue(50, (int)(wanderRadius * 10)) / 10.0f;

        float tx = position.x + cosf(angle) * radius;
        float tz = position.z + sinf(angle) * radius;

        // Не вылетаем за границы карты
        tx = Clamp(tx, -(float)halfMap + 5.0f, (float)halfMap - 5.0f);
        tz = Clamp(tz, -(float)halfMap + 5.0f, (float)halfMap - 5.0f);

        float ty = world->GetHeight(tx, tz);

        // Если точка попала на пляж, выбираем её
        if (IsOnBeach(ty)) {
            targetPosition = { tx, ty, tz };
            return;
        }
    }

    // Если за 20 попыток песок рядом не нашли, просто сидим на месте
    targetPosition = position;
    state = AnimalState::IDLE;
    stateTimer = (float)GetRandomValue(2, 4);
}

void Crab::Update(float deltaTime, World* world) {
    if (!IsAlive()) return;

    // Плавное уменьшение лайфбара / сытости (опционально)
    hunger -= deltaTime * 0.2f; 
    stateTimer -= deltaTime;

    float currentHeight = world->GetHeight(position.x, position.z);

    // СТРАХОВКА: Если краб ушёл в глубокую воду или залез на гору, 
    // срочно ищем новый пляжный таргет
    if (!IsOnBeach(currentHeight) && state != AnimalState::IDLE) {
        PickNewBeachTarget(world);
    }

    switch (state) {
        case AnimalState::IDLE: {
            if (stateTimer <= 0.0f) {
                state = AnimalState::WANDERING;
                PickNewBeachTarget(world);
            }
            break;
        }

        case AnimalState::WANDERING: {
            // Двигаемся к цели
            MoveTowardsTarget(deltaTime, world);

            // Если дошли до цели или вышло время блуждания
            float distToTarget = Vector3Distance(position, targetPosition);
            if (distToTarget <= 0.5f || stateTimer <= 0.0f) {
                state = AnimalState::IDLE;
                stateTimer = (float)GetRandomValue(1, 4); // Отдыхаем пару секунд
            }
            break;
        }
        
        default:
            break;
    }

    // Фиксация на поверхности земли
    position.y = world->GetHeight(position.x, position.z);
}

void Crab::Draw() {
    Vector3 crabSize = { 0.8f, 0.4f, 0.6f };
    DrawCube(position, crabSize.x, crabSize.y, crabSize.z, MAROON);
    DrawCubeWires(position, crabSize.x, crabSize.y, crabSize.z, ORANGE);

    Vector3 eyeLeft = { position.x + 0.3f, position.y + 0.3f, position.z + 0.2f };
    Vector3 eyeRight = { position.x + 0.3f, position.y + 0.3f, position.z - 0.2f };
    DrawSphere(eyeLeft, 0.1f, BLACK);
    DrawSphere(eyeRight, 0.1f, BLACK);
}