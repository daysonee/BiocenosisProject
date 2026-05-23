#include "sheep.hpp"
#include "wolf.hpp"
#include "plant.hpp"
#include "../core/world.hpp"
#include <raymath.h>
#include "../core/constants.hpp"

Sheep::Sheep(Vector3 startPosition) : Animal(startPosition){
    
    float runMod = (float)GetRandomValue(Config::Sheep::RUN_DECREASE, Config::Sheep::RUN_INCREASE) / 100.0f;
    float walkMod = (float)GetRandomValue(Config::Sheep::WALK_DECREASE, Config::Sheep::WALK_INCREASE) / 100.0f;
    float maxHungerMod = (float)GetRandomValue(Config::Sheep::MAX_HUNGER_DECREASE, Config::Sheep::MAX_HUNGER_INCREASE) / 100.0f;
    float hungerThresholdMod = (float)GetRandomValue(Config::Sheep::HUNGER_THRESHOLD_DECREASE, Config::Sheep::HUNGER_THRESHOLD_INCREASE) / 100.0f;
    float visionMod = (float)GetRandomValue(Config::Sheep::VISION_DECREASE, Config::Sheep::VISION_INCREASE) / 100.0f;

    myWalkSpeed = Config::Sheep::SPEED_WALK * walkMod;
    myRunSpeed = Config::Sheep::SPEED_RUN * runMod;
    myVisionRadius = Config::Sheep::VISION_RADIUS * visionMod;
    myHungerThreshold = Config::Sheep::HUNGER_THRESHOLD * hungerThresholdMod;
    myMaxHunger = Config::Sheep::MAX_HUNGER * maxHungerMod;
    
    speed = myWalkSpeed;
    hunger = myMaxHunger;

    state = AnimalState::WANDERING;

    flockCenter = startPosition; 
}

void Sheep::Update(float deltaTime, World* world){
    if (!IsAlive()) return;

    // --- 1. ЖИЗНЕННЫЕ ПОКАЗАТЕЛИ И КРУГОВОРOT ИИ ---
    hunger -= Config::Sheep::HUNGER_DECAY_RATE * deltaTime;
    
    // Проверка волков
    for (const auto& entity : world->GetEntities()){
        if (!entity->IsAlive()) continue;

        Wolf* wolf = dynamic_cast<Wolf*>(entity.get());
        if (wolf != nullptr){
            if(Vector3Distance(position, wolf->GetPosition()) < myVisionRadius){
                state = AnimalState::FLEEING;
                targetHunter = wolf;
                break;
            }
        }
    }

    if (targetHunter != nullptr && state == AnimalState::FLEEING && Vector3Distance(targetHunter->GetPosition(), position) > myVisionRadius){
        state = AnimalState::WANDERING;
        targetHunter = nullptr;
    }

    if (hunger < myHungerThreshold && state != AnimalState::FLEEING){
        state = AnimalState::HUNGRY;
    }

    // --- 2. ВЫПОЛНЕНИЕ ТЕКУЩЕГО СОСТОЯНИЯ ---
    if (state == AnimalState::FLEEING){
        speed = myRunSpeed;
        Vector3 runDir = Vector3Subtract(position, targetHunter->GetPosition());
        Vector3 normalizedRunDir = Vector3Normalize(runDir);
        targetPosition = Vector3Add(position, Vector3Scale(normalizedRunDir, 5.0f));
        MoveTowardsTarget(deltaTime, world);
    }

    if (state == AnimalState::HUNGRY){
        speed = myWalkSpeed;
        Plant* closestPlant = nullptr;
        float closestDist = myVisionRadius; 

        for (const auto& entity : world->GetEntities()){
            if(!entity->IsAlive()) continue;
            
            Plant* plant = dynamic_cast<Plant*>(entity.get());
            if (plant != nullptr){
                float dist = Vector3Distance(position, plant->GetPosition());
                if (dist < closestDist){
                    closestDist = dist;
                    closestPlant = plant;
                }
            }
        }
        if(closestPlant != nullptr){
            targetPosition = closestPlant->GetPosition();
            MoveTowardsTarget(deltaTime, world);
            if (closestDist < 1.0f){
                closestPlant->Die();
                hunger = myMaxHunger;
                state = AnimalState::IDLE;
                stateTimer = Config::Sheep::IDLE_TIME;
            }
        } else {
            MoveTowardsTarget(deltaTime, world);
            Vector2 flatPos = {position.x, position.z};
            Vector2 flatTarget = {targetPosition.x, targetPosition.z};
            if(Vector2Distance(flatPos, flatTarget) < Config::Sheep::REACH_TARGET_DIST){
                state = AnimalState::IDLE;
                stateTimer = Config::Sheep::IDLE_TIME;
            }    
        }
    }

    if (state == AnimalState::IDLE){
        speed = myWalkSpeed;
        stateTimer -= deltaTime;
        if (stateTimer < 0.0f){
            int halfMap = Config::World::MAP_SIZE / 2;
            float rx = (float)GetRandomValue(-halfMap, halfMap);
            float rz = (float)GetRandomValue(-halfMap, halfMap);

            targetPosition = {rx, world->GetHeight(rx, rz), rz};
            state = AnimalState::WANDERING;
        }
    }

    if (state == AnimalState::WANDERING){
        MoveTowardsTarget(deltaTime, world);
        Vector2 flatPos = {position.x, position.z};
        Vector2 flatTarget = {targetPosition.x, targetPosition.z};
        if(Vector2Distance(flatPos, flatTarget) < Config::Sheep::REACH_TARGET_DIST){
            state = AnimalState::IDLE;
            stateTimer = Config::Sheep::IDLE_TIME;
        }    
    }

    // --- 3. МЯГКОЕ УПРУГОЕ РАСТАЛКИВАНИЕ (ANTI-CLIPPING) ---
    // Срабатывает ПОСЛЕ того, как овечка сделала свой шаг по ИИ. Убирает прохождение сквозь текстуры.
    int overlapCount = 0;
    float minDistance = Config::Sheep::BODY_RADIUS * 2.0f; // Полный физический диаметр двух овечек (0.4 * 2 = 0.8)

    for (const auto& entity : world->GetEntities()) {
        if (entity.get() == this || !entity->IsAlive()) continue;

        if (Sheep* otherSheep = dynamic_cast<Sheep*>(entity.get())) {
            if (this->isMating && otherSheep->isMating) continue; 

            float dist = Vector3Distance(this->position, otherSheep->GetPosition());
            if (dist < minDistance) {
                Vector3 pushDir;
                if (dist == 0.0f) {
                    // Если овечки заспавнились идеально в одной точке, толкаем в случайную сторону
                    float randomAngle = (float)GetRandomValue(0, 360) * DEG2RAD;
                    pushDir = { cosf(randomAngle), 0.0f, sinf(randomAngle) };
                    dist = 0.01f; 
                } else {
                    pushDir = Vector3Subtract(this->position, otherSheep->GetPosition());
                    pushDir.y = 0.0f; // Работаем строго в горизонтальной плоскости
                    pushDir = Vector3Normalize(pushDir);
                }

                float overlap = minDistance - dist;

                // ЧИНИМ БАГ: Выталкиваем МГНОВЕННО прямо внутри цикла!
                // Коэффициент 0.5f означает, что эта овечка берет на себя половину шага назад, 
                // а вторую половину сделает соседняя овечка в своем собственном Update.
                this->position.x += pushDir.x * overlap * 0.5f;
                this->position.z += pushDir.z * overlap * 0.5f;

                overlapCount++;
            }
        }
    }

    // Фишка против жестких куч: если овечка застряла в толпе (рядом 2+ овцы), 
    // сбрасываем её цель блуждания, чтобы она выбрала новое случайное направление.
    if (overlapCount >= 2) {
        if (state == AnimalState::WANDERING) {
            state = AnimalState::IDLE;
            stateTimer = (float)GetRandomValue(5, 15) * 0.1f; 
        }
    }

    // Корректируем финальную высоту овечки строго по рельефу мира
    this->position.y = world->GetHeight(this->position.x, this->position.z);
}


void Sheep::Draw(){
    DrawCube(position, 1.2f, 1.2f, 1.2f, WHITE);
    Vector3 headPos = { position.x + 0.6f, position.y + 0.2f, position.z };
    DrawCube(headPos, 0.4f, 0.4f, 0.4f, BLACK);    
}
