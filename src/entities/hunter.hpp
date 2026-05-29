#pragma once
#include "../core/entity.hpp"
#include "../core/constants.hpp"
#include "raylib.h"

class Wolf;
class Sheep;

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
    
    float abilityTimer = 0.0f;     // Сколько секунд еще действует ускорение
    float abilityCooldown = 0.0f;  // Сколько секунд осталось до новой активации
    
    // Новые переменные для визуала
    float facingAngle; // Запоминает угол поворота, чтобы не сбрасываться на 0
    float smokeTimer;  // Таймер для дыма из трубы

    Texture2D faceTexture;
    Sound killPhraseSounds[3];
    bool killPhraseSoundLoaded[3];
    bool playerControlled;
    Vector3 controlForward;
    Vector3 controlRight;
    Vector3 cameraShootOrigin;
    Vector3 cameraShootDir;
    bool shootRequested;

    void LoadKillPhraseSounds();
    void UnloadKillPhraseSounds();
    void PlayRandomKillPhrase();
    void ShootEntity(Entity* target, World* world);
    Entity* FindShootTargetByRay(World* world, Vector3 rayOrigin, Vector3 rayDir);
    void UpdatePlayerControl(float deltaTime, World* world);

    void MoveTowards(Vector3 target, float speed, float deltaTime, World* world);
    Wolf* FindNearestWolf(World* world);

public:
    Hunter(Vector3 startPosition);
    ~Hunter() override;

    void Update(float deltaTime, World* world) override;
    void Draw() override;
    void PlayShoot(World* world);
    void Draw2D(Camera camera);

    void SetPlayerControlled(bool enabled);
    bool IsPlayerControlled() const;
    void SetPlayerAim(Vector3 forward, Vector3 right, Vector3 cameraOrigin);
    void RequestShoot();
    
    Color GetDeathColor() const override { return MAROON; }
};