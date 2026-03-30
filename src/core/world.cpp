#include "world.hpp"

void World::AddEntity(std::unique_ptr<Entity> entity){
    entities.push_back(std::move(entity));
}

void World::Update(float deltaTime){
    for (auto& entity : entities){
        entity->Update(deltaTime);
    }
}

void World::Draw(){
    for (auto& entity : entities){
        entity->Draw(); 
    }
}