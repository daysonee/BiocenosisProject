#include "animal.hpp"
#include "raymath.h"
#include "../core/world.hpp"

Animal::Animal(Vector3 startPosition) : Entity(startPosition){
    health = 100.0f;
    hunger = 100.0f;
    speed = 2.0f;
    state = AnimalState::IDLE;
    targetPosition = startPosition;
}

void Animal::MoveTowardsTarget(float deltaTime, World* world){

    Vector3 targetFlat = {targetPosition.x, position.y, targetPosition.z}; 
    Vector3 direction = Vector3Subtract(targetFlat, position);

    float distance = Vector3Length(direction);

    if(distance > 0.1f){
        Vector3 normilizedDir = Vector3Normalize(direction);
        Vector3 velocity = Vector3Scale(normilizedDir, speed * deltaTime);
        position.x += velocity.x;
        position.z += velocity.z;
    }
    if (world != nullptr) {
        position.y = world->GetHeight(position.x, position.z);
    }
}
