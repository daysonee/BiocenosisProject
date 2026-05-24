#include "wolf.hpp"
#include "sheep.hpp"
#include "../core/world.hpp"
#include "../core/constants.hpp"
#include <raymath.h>
#include <cmath>

// Максимальная ширина водоёма (в единицах мира), которую волк решается перейти
static constexpr float WOLF_MAX_WATER_CROSSING = 8.0f;
// Шаг сканирования пути при определении ширины воды
static constexpr float WOLF_SCAN_STEP = 0.6f;

Wolf::Wolf(Vector3 startPosition) : Animal(startPosition) {
    speed      = 3.5f;
    hunger     = 100.0f;
    state      = AnimalState::WANDERING;
    targetPrey = nullptr;

    // Первая цель блуждания — поблизости от точки спавна
    targetPosition = startPosition;
}

// Сканирует путь вперёд: входит ли он в воду и выходит ли обратно на сушу
// в пределах WOLF_MAX_WATER_CROSSING единиц.
bool Wolf::IsNarrowWater(float fromX, float fromZ,
                          float dirX,  float dirZ,
                          World* world) {
    bool enteredWater = false;

    for (float d = WOLF_SCAN_STEP; d <= WOLF_MAX_WATER_CROSSING; d += WOLF_SCAN_STEP) {
        float h = world->GetHeight(fromX + dirX * d, fromZ + dirZ * d);
        if (h <= Config::World::WATER_LEVEL) {
            enteredWater = true;
        } else if (enteredWater) {
            // Вышли на сушу — значит водоём узкий, можно перейти
            return true;
        }
    }
    return false;
}

// Движение с проверкой: если обычный шаг заблокирован водой,
// проверяем ширину водоёма и форсируем переправу при узком канале.
void Wolf::MoveWithWaterCrossing(float deltaTime, World* world) {
    bool moved = MoveTowardsTarget(deltaTime, world);
    if (moved) return; // Прошли без проблем

    // Заблокированы — вычисляем направление к цели
    Vector3 dir = {
        targetPosition.x - position.x,
        0.0f,
        targetPosition.z - position.z
    };
    float len = Vector3Length(dir);
    if (len < 0.01f) return;

    dir = Vector3Normalize(dir);

    // Проверяем ширину водоёма
    if (IsNarrowWater(position.x, position.z, dir.x, dir.z, world)) {
        // Переправа: форсируем шаг, игнорируя воду
        float step = speed * deltaTime;
        position.x += dir.x * step;
        position.z += dir.z * step;
        position.y  = world->GetHeight(position.x, position.z);
    }
    // Иначе — остаёмся, слайдинг из базового MoveTowardsTarget уже применён
}

void Wolf::Update(float deltaTime, World* world) {
    if (!IsAlive()) return;

    // ----------------------------------------------------------------
    // 1. ГОЛОД
    // ----------------------------------------------------------------
    hunger -= 2.0f * deltaTime;
    if (hunger < 0.0f) hunger = 0.0f;

    // Инвалидация мёртвой жертвы
    if (targetPrey != nullptr && !targetPrey->IsAlive()) {
        targetPrey = nullptr;
    }

    // Переход в охоту при низком голоде
    if (hunger < 90.0f && state != AnimalState::HUNGRY) {
        state = AnimalState::HUNGRY;
    }

    // ----------------------------------------------------------------
    // 2. СТЕЙТ-МАШИНА
    // ----------------------------------------------------------------
    switch (state) {

        case AnimalState::IDLE: {
            // После еды — короткий отдых, потом снова бродим
            // (idle timer не реализован, просто переходим)
            state = AnimalState::WANDERING;

            int halfMap = Config::World::MAP_SIZE / 2;
            float rx, rz, ry;
            // Ищем сухую точку для блуждания
            for (int i = 0; i < 10; ++i) {
                rx = (float)GetRandomValue(-halfMap, halfMap);
                rz = (float)GetRandomValue(-halfMap, halfMap);
                ry = world->GetHeight(rx, rz);
                if (ry > Config::World::WATER_LEVEL) {
                    targetPosition = { rx, ry, rz };
                    break;
                }
            }
            break;
        }

        case AnimalState::WANDERING: {
            MoveWithWaterCrossing(deltaTime, world);

            Vector2 flatPos    = { position.x, position.z };
            Vector2 flatTarget = { targetPosition.x, targetPosition.z };
            if (Vector2Distance(flatPos, flatTarget) < 0.5f) {
                state = AnimalState::IDLE;
            }
            break;
        }

        case AnimalState::HUNGRY: {
            // Ищем ближайшую живую овцу
            float  closestDistance = 9999.0f;
            targetPrey             = nullptr;

            for (const auto& entity : world->GetEntities()) {
                if (!entity->IsAlive()) continue;
                Sheep* sheep = dynamic_cast<Sheep*>(entity.get());
                if (sheep) {
                    float dist = Vector3Distance(position, sheep->GetPosition());
                    if (dist < closestDistance) {
                        closestDistance = dist;
                        targetPrey      = sheep;
                    }
                }
            }

            if (targetPrey) {
                targetPosition = targetPrey->GetPosition();
                MoveWithWaterCrossing(deltaTime, world);

                if (closestDistance < Config::Wolf::ATTACK_RADIUS) {
                    targetPrey->Die();
                    hunger     = 100.0f;
                    targetPrey = nullptr;
                    state      = AnimalState::IDLE;
                }
            } else {
                // Жертв нет — бродим
                state = AnimalState::WANDERING;
            }
            break;
        }

        case AnimalState::FLEEING:
            // Волки не убегают (пока)
            state = AnimalState::WANDERING;
            break;
    }

    // Снапим к рельефу
    position.y = world->GetHeight(position.x, position.z);
}

void Wolf::Draw() {
    DrawCube(position, 1.4f, 1.4f, 1.4f, DARKGRAY);

    Vector3 eye1 = { position.x + 0.7f, position.y + 0.3f, position.z - 0.3f };
    Vector3 eye2 = { position.x + 0.7f, position.y + 0.3f, position.z + 0.3f };

    DrawCube(eye1, 0.2f, 0.2f, 0.2f, RED);
    DrawCube(eye2, 0.2f, 0.2f, 0.2f, RED);
}