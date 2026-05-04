#include "plant.hpp"

Plant::Plant(Vector3 startPosition) : Entity(startPosition){

}

void Plant::Update(float deltaTime, World* world){

}

void Plant::Draw(){
    DrawCube(position, 1.0f, 1.0f, 1.0f, GREEN);
}