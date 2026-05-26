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
    speed      = Config::Wolf::SPEED_RUN;
    hunger     = 100.0f;
    state      = AnimalState::WANDERING;
    targetPrey = nullptr;

    pounceTimer         = 0.0f;
    pounceCooldownTimer = 0.0f;

    // Первая цель блуждания — поблизости от точки спавна
    targetPosition = startPosition;
}

// Запуск прыжка-рывка если жертва близко и кулдаун истёк
void Wolf::TryStartPounce(float distToPrey, World* world) {
    if (pounceTimer > 0.0f) return;            // уже прыгаем
    if (pounceCooldownTimer > 0.0f) return;    // ещё в кулдауне
    if (distToPrey > Config::Wolf::POUNCE_RANGE) return;

    pounceTimer         = Config::Wolf::POUNCE_DURATION;
    pounceCooldownTimer = Config::Wolf::POUNCE_COOLDOWN;

    // Визуальный эффект отталкивания: облачко пыли под лапами
    Vector3 dustPos = { position.x, position.y + 0.1f, position.z };
    world->SpawnParticles(dustPos, (Color){150, 130, 100, 255}, 6, false);
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
    // 0. ТАЙМЕРЫ ПРЫЖКА
    // ----------------------------------------------------------------
    if (pounceCooldownTimer > 0.0f) pounceCooldownTimer -= deltaTime;
    if (pounceTimer > 0.0f) {
        pounceTimer -= deltaTime;
        speed = Config::Wolf::POUNCE_SPEED;
    } else {
        speed = Config::Wolf::SPEED_RUN;
    }

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

                // Если жертва уже близко — пытаемся ринуться рывком
                TryStartPounce(closestDistance, world);

                MoveWithWaterCrossing(deltaTime, world);

                // Радиус хватки: обычно ATTACK_RADIUS, в прыжке шире
                float reach = (pounceTimer > 0.0f)
                              ? Config::Wolf::POUNCE_REACH
                              : Config::Wolf::ATTACK_RADIUS;

                if (closestDistance < reach) {
                    // ☠ Кровавые красные партиклы на месте жертвы
                    Vector3 killPos = targetPrey->GetPosition();
                    killPos.y += 0.5f;
                    world->SpawnParticles(killPos, (Color){200, 30, 30, 255}, 20, false);
                    // Тёмные брызги добавляют объёма
                    world->SpawnParticles(killPos, (Color){120, 10, 10, 255}, 10, false);

                    targetPrey->Die();
                    hunger              = 100.0f;
                    targetPrey          = nullptr;
                    pounceTimer         = 0.0f;     // прерываем рывок
                    pounceCooldownTimer = Config::Wolf::POUNCE_COOLDOWN;
                    state               = AnimalState::IDLE;
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
    // Визуальный прыжок во время рывка — парабола sin(πt)
    Vector3 drawPos = position;
    if (pounceTimer > 0.0f) {
        float t = 1.0f - (pounceTimer / Config::Wolf::POUNCE_DURATION); // 0 → 1
        drawPos.y += sinf(t * PI) * 0.8f;
    }

    DrawCube(drawPos, 1.4f, 1.4f, 1.4f, DARKGRAY);

    Vector3 eye1 = { drawPos.x + 0.7f, drawPos.y + 0.3f, drawPos.z - 0.3f };
    Vector3 eye2 = { drawPos.x + 0.7f, drawPos.y + 0.3f, drawPos.z + 0.3f };

    DrawCube(eye1, 0.2f, 0.2f, 0.2f, RED);
    DrawCube(eye2, 0.2f, 0.2f, 0.2f, RED);
}