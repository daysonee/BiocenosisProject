#pragma once

#include "animal.hpp"
#include "../core/constants.hpp"

class Sheep : public Animal {
private:
    float myWalkSpeed;
    float myRunSpeed;
    float myVisionRadius;
    float myHungerThreshold;
    float myMaxHunger;

    // Центр родного стада — овечка держится рядом
    Vector3 flockCenter;

    // Таймер между переходами IDLE -> WANDERING
    float stateTimer = 0.0f;

    // Указатель на охотника (волка). nullptr если нет угрозы
    Entity* targetHunter = nullptr;

    // --- Детектор застревания ---
    float   stuckCheckTimer   = 0.0f;   // до следующей проверки
    Vector3 posAtLastCheck    = {};     // позиция на момент последней проверки
    int     stuckCount        = 0;     // сколько интервалов подряд не двигались

    // Подбирает случайную СУХУЮ точку вблизи центра стада
    void PickNewWanderTarget(World* world);
    // Аварийный выход из кучи: толчок + новая цель
    void ForceEscape(World* world);

    Config::Sheep::AgeStage ageStage;
    float maxAgeInCurrentStage; // Сколько нужно прожить в текущей стадии
    float ageTimer = 0.0f;

    float matingCooldownTimer = 0.0f;
    float matingProgressTimer = 0.0f;
    Sheep* mateTarget = nullptr; // Ссылка на выбранного партнера

public:
    bool isMating = false;

    ~Sheep() override = default;

    Sheep(Vector3 startPosition, Config::Sheep::AgeStage startStage = Config::Sheep::AgeStage::BABY);
    
    Config::Sheep::AgeStage GetAgeStage() const { return ageStage; }
    bool CanMate() const { 
        return ageStage == Config::Sheep::AgeStage::ADULT && 
               hunger >= Config::Sheep::MATING_HUNGER_THRESHOLD && 
               matingCooldownTimer <= 0.0f && 
               isAlive; 
    }
    
    void SetMateTarget(Sheep* partner) { mateTarget = partner; }

    void Update(float deltaTime, World* world) override;
    void Draw() override;

    void SetFlockCenter(Vector3 center) { flockCenter = center; }
};