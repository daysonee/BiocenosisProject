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

    targetPosition = (Vector3){5.0f, 0.0f, 5.0f};
    targetHunter = nullptr;
}

void Sheep::Update(float deltaTime, World* world){
    hunger -= Config::Sheep::HUNGER_DECAY_RATE * deltaTime;
    
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

    if (targetHunter != nullptr &&  state == AnimalState::FLEEING && Vector3Distance(targetHunter->GetPosition(), position) > myVisionRadius){
        state = AnimalState::WANDERING;
        targetHunter = nullptr;
    }

    if (hunger < myHungerThreshold && state != AnimalState::FLEEING){
        state = AnimalState::HUNGRY;
    }

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
        float closestDist = 99999.0f;

        for (const auto& entity : world->GetEntities()){
            if(!entity->IsAlive()){
                continue;
            }
            Plant* plant = dynamic_cast<Plant*>(entity.get());
            if (plant != nullptr){
                float dist = Vector3Distance(position, plant->GetPosition());
                if (dist < closestDist){
                    closestDist = dist;
                    closestPlant = plant;
                }
            }
        }
        if(closestPlant!=nullptr){
            targetPosition = closestPlant->GetPosition();
            MoveTowardsTarget(deltaTime, world);
            if (closestDist < 1.0f){
                closestPlant -> Die();
                hunger = myMaxHunger;
                state = AnimalState::IDLE;
                stateTimer = Config::Sheep::IDLE_TIME;
            }
        } else{
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

            targetPosition = {rx, world -> GetHeight(rx, rz), rz};
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

    position.y = world->GetHeight(position.x, position.z);
}

void Sheep::Draw(){
    DrawCube(position, 1.2f, 1.2f, 1.2f, WHITE);
    Vector3 headPos = { position.x + 0.6f, position.y + 0.2f, position.z };
    DrawCube(headPos, 0.4f, 0.4f, 0.4f, BLACK);    
}
