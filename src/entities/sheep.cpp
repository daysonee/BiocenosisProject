#include "sheep.hpp"
#include "wolf.hpp"
#include "../core/world.hpp"
#include <raymath.h>

Sheep::Sheep(Vector3 startPosition) : Animal(startPosition){
    speed = 1.5f;
    state = AnimalState::WANDERING;

    targetPosition = (Vector3){5.0f, 0.0f, 5.0f};

    targetHunter = nullptr;
}

void Sheep::Update(float deltaTime, World* world){
    hunger -= 1.0f * deltaTime;

    if (hunger < 50.0f){
        state = AnimalState::HUNGRY;
    }

    
    for (const auto& entity : world->GetEntities()){
        if (!entity->IsAlive()) continue;

        Wolf* wolf = dynamic_cast<Wolf*>(entity.get());
        if (wolf != nullptr){
            if(Vector3Distance(position, wolf->GetPosition()) < 5.0f){
                state = AnimalState::FLEEING;
                targetHunter = wolf;
                break;
            }
        }
    }

    if (state == AnimalState::FLEEING){
        
    }


    if (state == AnimalState::IDLE){
        stateTimer -= deltaTime;
        if (stateTimer < 0.0f){
            targetPosition = {(float)GetRandomValue(-10, 10), 0, (float)GetRandomValue(-10, 10)};
            state = AnimalState::WANDERING;
        }
    }


    if (state == AnimalState::WANDERING /*|| state == AnimalState::IDLE*/){
        MoveTowardsTarget(deltaTime);
        float distBetweenSheepAndTarget = Vector3Distance(position, targetPosition);

            if (distBetweenSheepAndTarget < 0.2f){
            state = AnimalState::IDLE;
            stateTimer = 2.0f;
        }        
    }
}

void Sheep::Draw(){
    DrawCube(position, 1.2f, 1.2f, 1.2f, WHITE);
    Vector3 headPos = { position.x + 0.6f, position.y + 0.2f, position.z };
    DrawCube(headPos, 0.4f, 0.4f, 0.4f, BLACK);    
}
