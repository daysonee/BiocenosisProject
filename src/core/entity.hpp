#pragma once

#include "raylib.h"

class Entity {
    protected: 
        Vector3 position;
        bool isAlive;
    public:
        Entity(Vector3 startPosition);

        virtual ~Entity() = default;
        virtual void Update(float deltaTime) = 0;
        virtual void Draw() = 0;

        Vector3 GetPosition();
};