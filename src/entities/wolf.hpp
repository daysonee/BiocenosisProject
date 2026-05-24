#pragma once
#include "animal.hpp"

class Sheep;
class World;

class Wolf : public Animal {
private:
    Sheep* targetPrey;

    // Возвращает true если вода в этом направлении достаточно узкая для переправы.
    // dirX/dirZ — нормализованное направление движения.
    static bool IsNarrowWater(float fromX, float fromZ,
                               float dirX,  float dirZ,
                               World* world);

    // Движение с поддержкой переправы через узкую воду
    void MoveWithWaterCrossing(float deltaTime, World* world);

public:
    Wolf(Vector3 startPosition);
    ~Wolf() override = default;

    void Update(float deltaTime, World* world) override;
    void Draw() override;
};