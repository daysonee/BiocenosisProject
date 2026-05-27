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

        virtual Color GetDeathColor() const { return GRAY; }

        // Вызывается World'ом перед удалением dying-сущностей.
        // Каждая Entity должна обнулить все свои указатели на этого dying.
        // Предотвращает dangling pointers между тиками.
        virtual void OnEntityDying(Entity* dying) { (void)dying; }
};