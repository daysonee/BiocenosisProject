#include "wolf.hpp"
#include "sheep.hpp"
#include "../core/world.hpp"
#include <raymath.h>

Wolf::Wolf(Vector3 startPosition) : Animal(startPosition){
    speed = 3.5f;
    state = AnimalState::WANDERING;
    targetPrey = nullptr;
}

void Wolf::Update(float deltaTime, World* world){
    hunger -= 2.0f * deltaTime;

    if (hunger < 90.0f){
        state = AnimalState::HUNGRY;
    }

    if (state == AnimalState::IDLE){
        //...
    }



    if (state == AnimalState::HUNGRY){
        float closestDistance = 9999.0f;
        targetPrey = nullptr;

        for (const auto& entity : world->GetEntities()){
            if (!entity->IsAlive()) continue;

            Sheep* sheep = dynamic_cast<Sheep*>(entity.get());

            if (sheep != nullptr){
                float dist = Vector3Distance(position, sheep->GetPosition());
                if (dist < closestDistance){
                    closestDistance = dist;
                    targetPrey = sheep;
                }
            }

          
        }

        if (targetPrey != nullptr){
            targetPosition = targetPrey->GetPosition();
            MoveTowardsTarget(deltaTime, world);

            if (closestDistance < 1.0f){
                targetPrey -> Die();
                hunger = 100.0f;
                state = AnimalState::IDLE;
            }
        }
    }


    
}

void Wolf::Draw(){
    DrawCube(position, 1.4f, 1.4f, 1.4f, DARKGRAY);

    Vector3 eye1 = {position.x + 0.7f, position.y + 0.3f, position.z - 0.3f};
    Vector3 eye2 = {position.x + 0.7f, position.y + 0.3f, position.z + 0.3f};

    DrawCube(eye1, 0.2f, 0.2f, 0.2f, RED);
    DrawCube(eye2, 0.2f, 0.2f, 0.2f, RED);
}

