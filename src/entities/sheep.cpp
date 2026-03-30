#include "sheep.hpp"
#include <raymath.h>

Sheep::Sheep(Vector3 startPosition) : Animal(startPosition){
    speed = 1.5f;
    state = AnimalState::WANDERING;

    targetPosition = (Vector3){5.0f, 0.0f, 5.0f};
}

void Sheep::Update(float deltaTime){
    hunger -= 1.0f * deltaTime;

    if (hunger < 50.0f){
        state = AnimalState::HUNGRY;
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
