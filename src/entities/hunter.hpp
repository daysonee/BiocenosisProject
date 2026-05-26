#pragma once
#include "../core/entity.hpp"
#include "../core/constants.hpp"

class Wolf;

enum class HunterState {
    RESTING,    
    HUNTING,    
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