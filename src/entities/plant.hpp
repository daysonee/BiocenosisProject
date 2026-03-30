#pragma once

#include "../core/entity.hpp"

class Plant : public Entity {
public:
    Plant(Vector3 startPosition);

    void Update(float deltaTime) override;
    void Draw() override;

};