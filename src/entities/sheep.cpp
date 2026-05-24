#include "sheep.hpp"
#include "wolf.hpp"
#include "plant.hpp"
#include "../core/world.hpp"
#include <raymath.h>
#include "../core/constants.hpp"
#include <cmath>

// Максимальный радиус блуждания от центра стада
static constexpr float FLOCK_WANDER_RADIUS = 14.0f;
// Радиус, после которого овечка «вспоминает» о стаде и разворачивается
static constexpr float FLOCK_MAX_DIST = 22.0f;

Sheep::Sheep(Vector3 startPosition) : Animal(startPosition) {

    float runMod           = (float)GetRandomValue(Config::Sheep::RUN_DECREASE,              Config::Sheep::RUN_INCREASE)              / 100.0f;
    float walkMod          = (float)GetRandomValue(Config::Sheep::WALK_DECREASE,             Config::Sheep::WALK_INCREASE)             / 100.0f;
    float maxHungerMod     = (float)GetRandomValue(Config::Sheep::MAX_HUNGER_DECREASE,       Config::Sheep::MAX_HUNGER_INCREASE)       / 100.0f;
    float hungerThreshMod  = (float)GetRandomValue(Config::Sheep::HUNGER_THRESHOLD_DECREASE, Config::Sheep::HUNGER_THRESHOLD_INCREASE) / 100.0f;
    float visionMod        = (float)GetRandomValue(Config::Sheep::VISION_DECREASE,           Config::Sheep::VISION_INCREASE)           / 100.0f;

    myWalkSpeed       = Config::Sheep::SPEED_WALK        * walkMod;
    myRunSpeed        = Config::Sheep::SPEED_RUN         * runMod;
    myVisionRadius    = Config::Sheep::VISION_RADIUS     * visionMod;
    myHungerThreshold = Config::Sheep::HUNGER_THRESHOLD  * hungerThreshMod;
    myMaxHunger       = Config::Sheep::MAX_HUNGER        * maxHungerMod;

    speed         = myWalkSpeed;
    hunger        = myMaxHunger;
    stateTimer    = (float)GetRandomValue(0, 30) * 0.1f; // Рассинхронизируем старт
    targetHunter  = nullptr;
    flockCenter   = startPosition;

    // Начинаем в IDLE — первая цель выберется через stateTimer секунд
    state          = AnimalState::IDLE;
    targetPosition = startPosition;
}

// Выбирает случайную СУХУЮ точку вблизи flockCenter
void Sheep::PickNewWanderTarget(World* world) {
    const int halfMap = Config::World::MAP_SIZE / 2;

    // Попытки 1–8: ищем точку в радиусе FLOCK_WANDER_RADIUS от центра стада
    for (int attempt = 0; attempt < 8; ++attempt) {
        float angle  = (float)GetRandomValue(0, 360) * DEG2RAD;
        float radius = (float)GetRandomValue(30, (int)(FLOCK_WANDER_RADIUS * 10)) / 10.0f;

        float tx = flockCenter.x + cosf(angle) * radius;
        float tz = flockCenter.z + sinf(angle) * radius;

        tx = Clamp(tx, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        tz = Clamp(tz, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);

        float ty = world->GetHeight(tx, tz);
        if (ty > Config::World::WATER_LEVEL) {
            targetPosition = { tx, ty, tz };
            return;
        }
    }

    // Фолбэк: ищем сухую точку рядом с текущей позицией
    for (int attempt = 0; attempt < 10; ++attempt) {
        float tx = position.x + (float)GetRandomValue(-80, 80) * 0.1f;
        float tz = position.z + (float)GetRandomValue(-80, 80) * 0.1f;

        tx = Clamp(tx, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        tz = Clamp(tz, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);

        float ty = world->GetHeight(tx, tz);
        if (ty > Config::World::WATER_LEVEL) {
            targetPosition = { tx, ty, tz };
            return;
        }
    }
    // Если вообще ничего не нашли — стоим на месте (крайний случай)
}

// Аварийный выход из кучи: случайный сильный толчок в свободном направлении, новая цель
void Sheep::ForceEscape(World* world) {
    const int halfMap = Config::World::MAP_SIZE / 2;

    // Пробуем 8 случайных направлений, берём первое сухое
    for (int i = 0; i < 8; ++i) {
        float angle   = (float)GetRandomValue(0, 360) * DEG2RAD;
        float pushLen = Config::Sheep::BODY_RADIUS * 6.0f;
        float nx = Clamp(position.x + cosf(angle) * pushLen, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        float nz = Clamp(position.z + sinf(angle) * pushLen, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);

        if (world->GetHeight(nx, nz) > Config::World::WATER_LEVEL) {
            position.x = nx;
            position.z = nz;
            position.y = world->GetHeight(position.x, position.z);
            break;
        }
    }

    PickNewWanderTarget(world);
    state      = AnimalState::WANDERING;
    stuckCount = 0;
}

void Sheep::Update(float deltaTime, World* world) {

    if (!IsAlive()) return;

    // ----------------------------------------------------------------
    // 1. ЖИЗНЕННЫЕ ПОКАЗАТЕЛИ
    // ----------------------------------------------------------------
    hunger -= Config::Sheep::HUNGER_DECAY_RATE * deltaTime;
    if (hunger < 0.0f) hunger = 0.0f;

    // ----------------------------------------------------------------
    // 2. ОБНАРУЖЕНИЕ УГРОЗ И ПЕРЕКЛЮЧЕНИЕ ПРИОРИТЕТОВ
    // ----------------------------------------------------------------

    // Инвалидируем targetHunter если волк погиб
    if (targetHunter != nullptr && !targetHunter->IsAlive()) {
        targetHunter = nullptr;
    }

    // Сканируем волков в радиусе зрения
    bool wolfNearby = false;
    for (const auto& entity : world->GetEntities()) {
        if (!entity->IsAlive()) continue;
        Wolf* wolf = dynamic_cast<Wolf*>(entity.get());
        if (wolf && Vector3Distance(position, wolf->GetPosition()) < myVisionRadius) {
            wolfNearby   = true;
            targetHunter = wolf;
            break;
        }
    }

    // Переключаем состояния по приоритету: FLEEING > HUNGRY > остальное
    if (wolfNearby) {
        // Волк рядом — бежим
        state = AnimalState::FLEEING;
    } else if (state == AnimalState::FLEEING) {
        // Волк ушёл из зоны видимости — отдышаться и идти дальше
        targetHunter = nullptr;
        state        = AnimalState::IDLE;
        stateTimer   = 1.5f;
    } else if (hunger < myHungerThreshold) {
        // Только если не убегаем — переходим к поиску еды
        state = AnimalState::HUNGRY;
    }

    // ----------------------------------------------------------------
    // 3. СТЕЙТ-МАШИНА (switch гарантирует исполнение ТОЛЬКО одного блока за кадр)
    // ----------------------------------------------------------------
    switch (state) {

        // --- УБЕГАЕМ ОТ ВОЛКА ---
        case AnimalState::FLEEING: {
            speed = myRunSpeed;

            if (targetHunter) {
                Vector3 runDir = Vector3Subtract(position, targetHunter->GetPosition());
                runDir.y = 0.0f;

                if (Vector3Length(runDir) < 0.01f) {
                    // Случай когда волк прямо сверху
                    float a = (float)GetRandomValue(0, 360) * DEG2RAD;
                    runDir = { cosf(a), 0.0f, sinf(a) };
                } else {
                    runDir = Vector3Normalize(runDir);
                }

                int halfMap = Config::World::MAP_SIZE / 2;
                float tx = Clamp(position.x + runDir.x * 12.0f, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
                float tz = Clamp(position.z + runDir.z * 12.0f, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
                targetPosition = { tx, world->GetHeight(tx, tz), tz };
            }

            MoveTowardsTarget(deltaTime, world);
            break;
        }

        // --- ИЩЕМ ЕДУ ---
        case AnimalState::HUNGRY: {
            speed = myWalkSpeed;
            Plant* closestPlant = nullptr;
            float  closestDist  = myVisionRadius;

            for (const auto& entity : world->GetEntities()) {
                if (!entity->IsAlive()) continue;
                Plant* plant = dynamic_cast<Plant*>(entity.get());
                if (plant) {
                    float dist = Vector3Distance(position, plant->GetPosition());
                    if (dist < closestDist) {
                        closestDist  = dist;
                        closestPlant = plant;
                    }
                }
            }

            if (closestPlant) {
                // Идём к растению
                targetPosition = closestPlant->GetPosition();
                MoveTowardsTarget(deltaTime, world);

                if (closestDist < Config::Sheep::EAT_RADUIS) {
                    closestPlant->Die();
                    hunger     = myMaxHunger;
                    state      = AnimalState::IDLE;
                    stateTimer = Config::Sheep::IDLE_TIME;
                }
            } else {
                // Еды не видно — бродим в поисках, цель обновляем когда достигли или заблокированы
                Vector2 flatPos    = { position.x, position.z };
                Vector2 flatTarget = { targetPosition.x, targetPosition.z };
                if (Vector2Distance(flatPos, flatTarget) < Config::Sheep::REACH_TARGET_DIST) {
                    PickNewWanderTarget(world);
                }
                bool moved = MoveTowardsTarget(deltaTime, world);
                if (!moved) {
                    // Уткнулись в воду со всех сторон — немедленно берём новый курс
                    PickNewWanderTarget(world);
                }
            }
            break;
        }

        // --- ПАУЗА ---
        case AnimalState::IDLE: {
            stateTimer -= deltaTime;
            if (stateTimer <= 0.0f) {
                // Тянет к стаду если убежала слишком далеко
                float distToFlock = Vector2Distance(
                    { position.x, position.z },
                    { flockCenter.x, flockCenter.z }
                );
                if (distToFlock > FLOCK_MAX_DIST) {
                    // Форсируем возврат к стаду
                    flockCenter.y = world->GetHeight(flockCenter.x, flockCenter.z);
                    targetPosition = flockCenter;
                } else {
                    PickNewWanderTarget(world);
                }
                state = AnimalState::WANDERING;
            }
            break;
        }

        // --- БЛУЖДАНИЕ ---
        case AnimalState::WANDERING: {
            speed = myWalkSpeed;
            bool moved = MoveTowardsTarget(deltaTime, world);

            // Заблокированы водой со всех сторон — выбираем новую цель немедленно
            if (!moved) {
                PickNewWanderTarget(world);
                break;
            }

            Vector2 flatPos    = { position.x, position.z };
            Vector2 flatTarget = { targetPosition.x, targetPosition.z };
            if (Vector2Distance(flatPos, flatTarget) < Config::Sheep::REACH_TARGET_DIST) {
                state      = AnimalState::IDLE;
                stateTimer = Config::Sheep::IDLE_TIME;
            }
            break;
        }
    }

    // ----------------------------------------------------------------
    // 4. ФИЗИКА РАСТАЛКИВАНИЯ — Minecraft-стиль (скольжение, не остановка)
    // ----------------------------------------------------------------
    const float minDist = Config::Sheep::BODY_RADIUS * 2.0f;
    int overlapCount    = 0;

    for (const auto& entity : world->GetEntities()) {
        if (entity.get() == this || !entity->IsAlive()) continue;
        Sheep* other = dynamic_cast<Sheep*>(entity.get());
        if (!other) continue;
        if (isMating && other->isMating) continue;

        Vector3 otherPos = other->GetPosition();
        float dist = Vector3Distance(position, otherPos);

        if (dist < minDist) {
            Vector3 push;
            if (dist < 0.001f) {
                // Овечки в одной точке — случайное направление
                float a = (float)GetRandomValue(0, 360) * DEG2RAD;
                push = { cosf(a), 0.0f, sinf(a) };
            } else {
                push = Vector3Subtract(position, otherPos);
                push.y = 0.0f;
                push = Vector3Normalize(push);
            }

            // Отталкиваем ровно на половину перекрытия.
            // Вторая овечка сделает то же самое со своей стороны в своём Update.
            float halfOverlap = (minDist - dist) * 0.5f;
            position.x += push.x * halfOverlap;
            position.z += push.z * halfOverlap;
            ++overlapCount;
        }
    }

    // В толпе: прерываем WANDERING, чтобы не давить в одну кучу
    if (overlapCount >= 2 && state == AnimalState::WANDERING) {
        state      = AnimalState::IDLE;
        stateTimer = (float)GetRandomValue(3, 10) * 0.1f;
    }

    // ----------------------------------------------------------------
    // 5. ДЕТЕКТОР ЗАСТРЕВАНИЯ
    // Каждые STUCK_CHECK_INTERVAL секунд проверяем: сдвинулась ли овца.
    // Два провала подряд → аварийный выход из кучи.
    // ----------------------------------------------------------------
    const float STUCK_CHECK_INTERVAL = 1.5f;
    const float STUCK_MOVE_THRESHOLD = 0.4f; // минимум единиц за интервал
    const int   STUCK_LIMIT          = 2;    // провалов подряд до эскейпа

    stuckCheckTimer -= deltaTime;
    if (stuckCheckTimer <= 0.0f) {
        stuckCheckTimer = STUCK_CHECK_INTERVAL;

        float moved = Vector2Distance(
            { position.x, position.z },
            { posAtLastCheck.x, posAtLastCheck.z }
        );

        if (moved < STUCK_MOVE_THRESHOLD && (state == AnimalState::WANDERING || state == AnimalState::HUNGRY)) {
            ++stuckCount;
            if (stuckCount >= STUCK_LIMIT) {
                ForceEscape(world);
            }
        } else {
            stuckCount = 0;
        }

        posAtLastCheck = position;
    }

    // ----------------------------------------------------------------
    // 6. ПРИЛИПАНИЕ К РЕЛЬЕФУ + УТОПЛЕНИЕ
    // После всех сдвигов (включая расталкивание) овечка может оказаться
    // в воде. Фиксируем Y, и если под водой — постепенно теряем здоровье.
    // ----------------------------------------------------------------
    position.y = world->GetHeight(position.x, position.z);

    if (position.y <= Config::World::WATER_LEVEL) {
        // Овца в воде: тонем ~5 секунд при полном здоровье
        health -= 20.0f * deltaTime;
        if (health <= 0.0f) {
            Die();
        }
    } else {
        // На суше: медленно восстанавливаем здоровье (если выплыла)
        if (health < 100.0f) health = fminf(health + 5.0f * deltaTime, 100.0f);
    }
}

void Sheep::Draw() {
    DrawCube(position, 1.2f, 1.2f, 1.2f, WHITE);
    Vector3 headPos = { position.x + 0.6f, position.y + 0.2f, position.z };
    DrawCube(headPos, 0.4f, 0.4f, 0.4f, BLACK);
}