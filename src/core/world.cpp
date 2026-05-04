#include "world.hpp"

void World::AddEntity(std::unique_ptr<Entity> entity){
    entities.push_back(std::move(entity));
}

void World::Update(float deltaTime){
    for (auto& entity : entities){
        if (entity -> IsAlive()){
            entity->Update(deltaTime, this);
        }
    }
}

void World::Draw(){
    for (auto& entity : entities){
        if (entity->IsAlive()){
            entity->Draw(); 
        }
    }
}