#pragma once

#include "raylib.h"

class World;

class Entity {
    protected: 
        Vector3 position;
        bool isAlive;
    public:
        Entity(Vector3 startPosition);

        virtual ~Entity() = default;
        virtual void Update(float deltaTime, World *world) = 0;
        virtual void Draw() = 0;

        Vector3 GetPosition();

        bool IsAlive() const {return isAlive; }
        void Die() { isAlive = false; }
};