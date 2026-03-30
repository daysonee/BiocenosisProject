#include "entity.hpp"

Entity::Entity(Vector3 startPosition){
    position = startPosition;
    isAlive = true;
}

Vector3 Entity::GetPosition(){
    return position;
}