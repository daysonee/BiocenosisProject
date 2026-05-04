#pragma once

#include "../core/entity.hpp"

class Plant : public Entity {
public:
    Plant(Vector3 startPosition);

    void Update(float deltaTime, World* world) override;
    void Draw() override;

};