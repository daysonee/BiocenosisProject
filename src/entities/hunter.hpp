#pragma once
#include "../core/entity.hpp"
#include "../core/constants.hpp"

class Wolf;

enum class HunterState {
    RESTING,    
    WANDERING,   // Идет в лес и ищет волков 
    CHASING,    
    RETURNING 
};

class Hunter : public Entity {
private:
    Vector3 hutPosition;
    Vector3 targetPosition;
    HunterState state;
    Wolf* targetWolf;

    float stateTimer;
    float shootCooldown;
    float recoilTimer;
    
    // Новые переменные для визуала
    float facingAngle; // Запоминает угол поворота, чтобы не сбрасываться на 0
    float smokeTimer;  // Таймер для дыма из трубы

    void MoveTowards(Vector3 target, float speed, float deltaTime, World* world);
    Wolf* FindNearestWolf(World* world);

public:
    Hunter(Vector3 startPosition);
    ~Hunter() override = default;

    void Update(float deltaTime, World* world) override;
    void Draw() override;
    
    void Draw2D(Camera camera);
    
    Color GetDeathColor() const override { return MAROON; }
};