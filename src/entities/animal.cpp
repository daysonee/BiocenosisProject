#include "animal.hpp"
#include "raymath.h"

Animal::Animal(Vector3 startPosition) : Entity(startPosition){
    health = 100.0f;
    hunger = 100.0f;
    speed = 2.0f;
    state = AnimalState::IDLE;
    targetPosition = startPosition;
}

void Animal::MoveTowardsTarget(float deltaTime){
    Vector3 direction = Vector3Subtract(targetPosition, position);

    float distance = Vector3Length(direction);

    if(distance > 0.1f){
        Vector3 normilizedDir = Vector3Normalize(direction);

        Vector3 velocity = Vector3Scale(normilizedDir, speed * deltaTime);
        position = Vector3Add(position, velocity);
    }
}
