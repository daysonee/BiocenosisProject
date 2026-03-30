#pragma once

#include <vector>
#include <memory>
#include "entity.hpp"

class World{
private:
    std::vector<std::unique_ptr<Entity>> entities;

public:
    void AddEntity(std::unique_ptr<Entity> entity);

    void Update(float deltaTime);
    void Draw();
};