#pragma warning(disable: 4576)
#include "wolf.hpp"
#include "sheep.hpp"
#include "carcass.hpp"
#include "hunter.hpp"
#include "../core/world.hpp"
#include "../core/constants.hpp"
#include <raymath.h>
#include <rlgl.h>
#include <cmath>

// ═════════════════════════════════════════════════════════════════════════════
//                          КОНСТРУКТОР
// ═════════════════════════════════════════════════════════════════════════════

Wolf::Wolf(Vector3 startPos, Config::Wolf::AgeStage startStage, int wolfPackId)
    : Animal(startPos)
{
    hunger    = 100.0f;
    state     = AnimalState::IDLE;
    packId    = wolfPackId;
    ageStage  = startStage;
    homePos   = startPos;
    fightHealth = Config::Wolf::FIGHT_MAX_HEALTH;

    switch (ageStage) {
        case Config::Wolf::AgeStage::BABY:
            maxAgeInCurrentStage = (float)GetRandomValue(
                (int)Config::Wolf::TIME_TO_GROW_BABY_MIN,
                (int)Config::Wolf::TIME_TO_GROW_BABY_MAX);
            ageTimer = 0.0f;
            break;
        case Config::Wolf::AgeStage::MEDIUM:
            maxAgeInCurrentStage = (float)GetRandomValue(
                (int)Config::Wolf::TIME_TO_GROW_MEDIUM_MIN,
                (int)Config::Wolf::TIME_TO_GROW_MEDIUM_MAX);
            ageTimer = (float)GetRandomValue(0, (int)(maxAgeInCurrentStage * 0.5f));
            break;
        case Config::Wolf::AgeStage::ADULT:
            maxAgeInCurrentStage = Config::Wolf::TIME_ADULT_LIFESPAN;
            ageTimer = (float)GetRandomValue(0, (int)(maxAgeInCurrentStage * 0.5f));
            break;
    }

    matingCooldownTimer = (float)GetRandomValue(10, 40);
    speed = Config::Wolf::SPEED_RUN * CurrentSpeedFactor();
}

// ═════════════════════════════════════════════════════════════════════════════
//                          ВСПОМОГАТЕЛЬНЫЕ
// ═════════════════════════════════════════════════════════════════════════════

float Wolf::CurrentPounceReach() const {
    switch (ageStage) {
        case Config::Wolf::AgeStage::BABY:   return Config::Wolf::POUNCE_REACH;
        case Config::Wolf::AgeStage::MEDIUM: return Config::Wolf::MEDIUM_POUNCE_REACH;
        case Config::Wolf::AgeStage::ADULT:  return Config::Wolf::ADULT_POUNCE_REACH;
    }
    return Config::Wolf::POUNCE_REACH;
}

float Wolf::CurrentLeadTime() const {
    return (ageStage == Config::Wolf::AgeStage::ADULT)
        ? Config::Wolf::ADULT_LEAD_MAX_TIME : Config::Wolf::LEAD_MAX_TIME;
}

float Wolf::CurrentSizeFactor() const {
    switch (ageStage) {
        case Config::Wolf::AgeStage::BABY:   return Config::Wolf::BABY_SIZE_FACTOR;
        case Config::Wolf::AgeStage::MEDIUM: return 1.0f;
        case Config::Wolf::AgeStage::ADULT:  return Config::Wolf::ADULT_SIZE_FACTOR;
    }
    return 1.0f;
}

float Wolf::CurrentSpeedFactor() const {
    return (ageStage == Config::Wolf::AgeStage::BABY)
        ? Config::Wolf::BABY_SPEED_FACTOR : 1.0f;
}

float Wolf::FullCooldown() const {
    return (ageStage == Config::Wolf::AgeStage::BABY)
        ? Config::Wolf::POUNCE_COOLDOWN * Config::Wolf::BABY_COOLDOWN_MULT
        : Config::Wolf::POUNCE_COOLDOWN;
}

float Wolf::CurrentHuntingRadius() const {
    if (isSheepFrenzy) {
        return Config::Wolf::HUNTING_RADIUS_ADULT * 3.0f; // Увеличиваем радиус поиска овец в 3 раза!
    }
    switch (ageStage) {
        case Config::Wolf::AgeStage::BABY:   return Config::Wolf::HUNTING_RADIUS_BABY;
        case Config::Wolf::AgeStage::MEDIUM: return Config::Wolf::HUNTING_RADIUS_MEDIUM;
        case Config::Wolf::AgeStage::ADULT:  return Config::Wolf::HUNTING_RADIUS_ADULT;
    }
    return Config::Wolf::HUNTING_RADIUS_MEDIUM;
}

// Волк находится в лесу если высота между SAND_LEVEL и BIOME_THRESHOLD
bool Wolf::IsInForest(World* world) const {
    float h = world->GetHeight(position.x, position.z);
    return h > Config::World::SAND_LEVEL && h <= Config::World::BIOME_THRESHOLD;
}

void Wolf::PickForestWanderTarget(World* world) {
    const int halfMap = Config::World::MAP_SIZE / 2;
    const float radius = Config::Wolf::HOME_PATROL_RADIUS;

    for (int i = 0; i < 20; ++i) {
        float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
        float r     = (float)GetRandomValue(5, (int)radius);
        float tx = Clamp(homePos.x + cosf(angle) * r, -(float)halfMap + 5, (float)halfMap - 5);
        float tz = Clamp(homePos.z + sinf(angle) * r, -(float)halfMap + 5, (float)halfMap - 5);
        float ty = world->GetHeight(tx, tz);
        if (ty > Config::World::SAND_LEVEL && ty <= Config::World::BIOME_THRESHOLD) {
            targetPosition = { tx, ty, tz };
            return;
        }
    }
    // Фолбэк: homePos не в лесу — просто берём случайную точку рядом с позицией,
    // не сбрасывая цель на саму позицию (это и вызывало топтание)
    float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
    float r     = (float)GetRandomValue(15, 30);
    float tx = Clamp(position.x + cosf(angle) * r, -(float)halfMap + 5, (float)halfMap - 5);
    float tz = Clamp(position.z + sinf(angle) * r, -(float)halfMap + 5, (float)halfMap - 5);
    targetPosition = { tx, world->GetHeight(tx, tz), tz };
}

void Wolf::PickMeadowHuntTarget(World* world) {
    const int halfMap = Config::World::MAP_SIZE / 2;

    // Ищем луга расширяющимися кольцами от текущей позиции
    for (int radius = 20; radius <= 300; radius += 20) {
        for (int attempt = 0; attempt < 8; ++attempt) {
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float r     = (float)GetRandomValue(radius - 15, radius + 15);
            float tx = Clamp(position.x + cosf(angle) * r, -(float)halfMap + 10, (float)halfMap - 10);
            float tz = Clamp(position.z + sinf(angle) * r, -(float)halfMap + 10, (float)halfMap - 10);
            float ty = world->GetHeight(tx, tz);
            if (ty > Config::World::BIOME_THRESHOLD) {
                targetPosition = { tx, ty, tz };
                return;
            }
        }
    }
    // Фолбэк — карта может быть почти без лугов, берём случайную точку подальше
    float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
    float tx = Clamp(position.x + cosf(angle) * 150.0f, -(float)halfMap + 10, (float)halfMap - 10);
    float tz = Clamp(position.z + sinf(angle) * 150.0f, -(float)halfMap + 10, (float)halfMap - 10);
    targetPosition = { tx, world->GetHeight(tx, tz), tz };
}

void Wolf::PromoteToLeader() {
    ageStage             = Config::Wolf::AgeStage::ADULT;
    maxAgeInCurrentStage = Config::Wolf::TIME_ADULT_LIFESPAN;
    ageTimer             = 0.0f;
    isLeader             = true;
}

// ═════════════════════════════════════════════════════════════════════════════
//                          ДВИЖЕНИЕ
// ═════════════════════════════════════════════════════════════════════════════

void Wolf::MoveSwimming(float dt, World* world) {
    Vector3 dir = { targetPosition.x - position.x, 0.0f, targetPosition.z - position.z };
    float dist = Vector3Length(dir);
    if (dist < 0.1f) {
        position.y = fmaxf(world->GetHeight(position.x, position.z),
                           world->GetCurrentWaterLevel());
        return;
    }

    Vector3 norm = Vector3Normalize(dir);
    float terrainH = world->GetHeight(position.x, position.z);
    bool  inWater  = (terrainH <= world->GetCurrentWaterLevel());
    float curSpeed = speed * (inWater ? Config::Wolf::SWIM_SPEED_FACTOR : 1.0f);

    const int halfMap = Config::World::MAP_SIZE / 2;
    const float minB = -(float)halfMap + 2.0f, maxB = (float)halfMap - 2.0f;

    position.x = Clamp(position.x + norm.x * curSpeed * dt, minB, maxB);
    position.z = Clamp(position.z + norm.z * curSpeed * dt, minB, maxB);
    position   = world->ResolveTreeCollisions(position, 0.4f);
    position.x = Clamp(position.x, minB, maxB);
    position.z = Clamp(position.z, minB, maxB);

    float newH = world->GetHeight(position.x, position.z);
    if (newH <= world->GetCurrentWaterLevel()) {
        position.y = world->GetCurrentWaterLevel() + 0.05f;
        if (!wasInWater) wasInWater = true;
        swimSplashTimer -= dt;
        if (swimSplashTimer <= 0.0f) {
            swimSplashTimer = 0.25f;
            world->SpawnParticles(position, (Color){100,160,220,255}, 4, false);
        }
    } else {
        position.y = newH;
        if (wasInWater) {
            shakeTimer = Config::Wolf::SHAKE_DURATION;
            world->SpawnParticles(position, (Color){100,160,220,255}, 16, false);
            wasInWater = false;
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//                          ПРЫЖОК
// ═════════════════════════════════════════════════════════════════════════════

void Wolf::TryStartPounce(float distToPrey, World* world) {
    if (pounceTimer > 0.0f || pounceCooldownTimer > 0.0f) return;
    if (distToPrey > Config::Wolf::POUNCE_RANGE) return;
    if (shakeTimer > 0.0f) return;
    if (world->GetHeight(position.x, position.z) <= world->GetCurrentWaterLevel()) return;

    pounceTimer         = Config::Wolf::POUNCE_DURATION;
    pounceCooldownTimer = FullCooldown();
    world->SpawnParticles(
        { position.x, position.y + 0.1f, position.z },
        (Color){150,130,100,255}, 6, false);
}


void Wolf::SetPlayerControlled(bool enabled) {
    playerControlled = enabled;
    if (playerControlled) {
        targetPrey = nullptr;
        mateTarget = nullptr;
        fightTarget = nullptr;
        isMating = false;
        state = AnimalState::WANDERING;
        targetPosition = position;
    }
}

bool Wolf::IsPlayerControlled() const {
    return playerControlled;
}

void Wolf::SetPlayerAim(Vector3 forward, Vector3 right) {
    if (Vector3Length(forward) > 0.01f) controlForward = Vector3Normalize(forward);
    if (Vector3Length(right) > 0.01f) controlRight = Vector3Normalize(right);

    Vector3 flatForward = { controlForward.x, 0.0f, controlForward.z };
    if (Vector3Length(flatForward) > 0.01f) {
        flatForward = Vector3Normalize(flatForward);
        facingAngle = atan2f(flatForward.x, flatForward.z) * RAD2DEG;
    }
}

void Wolf::UpdatePlayerControl(float dt, World* world) {
    hunger = 100.0f;
    state = AnimalState::WANDERING;

    Vector3 flatForward = { controlForward.x, 0.0f, controlForward.z };
    Vector3 flatRight = { controlRight.x, 0.0f, controlRight.z };
    if (Vector3Length(flatForward) > 0.01f) flatForward = Vector3Normalize(flatForward);
    if (Vector3Length(flatRight) > 0.01f) flatRight = Vector3Normalize(flatRight);

    Vector3 move = { 0.0f, 0.0f, 0.0f };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) move = Vector3Add(move, flatForward);
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) move = Vector3Subtract(move, flatForward);
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move = Vector3Add(move, flatRight);
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) move = Vector3Subtract(move, flatRight);

    float currentSpeed = Config::Wolf::SPEED_RUN * CurrentSpeedFactor();
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) currentSpeed *= 1.8f;

    if (Vector3Length(move) > 0.01f) {
        move = Vector3Normalize(move);
        const int halfMap = Config::World::MAP_SIZE / 2;
        float nx = Clamp(position.x + move.x * currentSpeed * dt, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        float nz = Clamp(position.z + move.z * currentSpeed * dt, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        if (world->GetHeight(nx, nz) > world->GetCurrentWaterLevel()) {
            position.x = nx;
            position.z = nz;
        }
    }
    position.y = world->GetHeight(position.x, position.z);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE)) {
        Sheep* target = FindNearestSheepInRadius(world, 3.0f);
        if (target) {
            Vector3 sheepPos = target->GetPosition();
            world->SpawnParticles(sheepPos, RED, 18, false);
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//                          ВОЗРАСТ
// ═════════════════════════════════════════════════════════════════════════════

void Wolf::UpdateAge(float dt, World* world) {
    ageTimer += dt;
    if (ageTimer < maxAgeInCurrentStage) return;

    if (ageStage == Config::Wolf::AgeStage::BABY) {
        ageStage = Config::Wolf::AgeStage::MEDIUM;
        maxAgeInCurrentStage = (float)GetRandomValue(
            (int)Config::Wolf::TIME_TO_GROW_MEDIUM_MIN,
            (int)Config::Wolf::TIME_TO_GROW_MEDIUM_MAX);
        ageTimer = 0.0f;
        // Проверяем лидера стаи
        bool packHasLeader = false;
        for (const auto& e : world->GetEntities()) {
            if (!e->IsAlive() || e.get() == this) continue;
            Wolf* w = dynamic_cast<Wolf*>(e.get());
            if (w && w->packId == packId && w->ageStage == Config::Wolf::AgeStage::ADULT) {
                packHasLeader = true; break;
            }
        }
        if (!packHasLeader) PromoteToLeader();
    } else if (ageStage == Config::Wolf::AgeStage::MEDIUM) {
        ageStage = Config::Wolf::AgeStage::ADULT;
        maxAgeInCurrentStage = Config::Wolf::TIME_ADULT_LIFESPAN;
        ageTimer = 0.0f;
    } else {
        if (targetGrassIndex != -1) world->SetGrassReserved(targetGrassIndex, false);
        diedOfOldAge = true;
        world->SpawnParticles(position, (Color){180,180,180,255}, 14, false);
        Die(DeathCause::OLD_AGE);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//                          ПОИСК
// ═════════════════════════════════════════════════════════════════════════════

Sheep* Wolf::FindNearestSheepInRadius(World* world, float radius) const {
    Sheep* best = nullptr; float bestD = radius;
    for (const auto& e : world->GetEntities()) {
        if (!e->IsAlive()) continue;
        Sheep* s = dynamic_cast<Sheep*>(e.get());
        if (!s) continue;
        float d = Vector3Distance(position, s->GetPosition());
        if (d < bestD) { bestD = d; best = s; }
    }
    return best;
}

Carcass* Wolf::FindNearestCarcassInRadius(World* world, float radius) const {
    Carcass* best = nullptr; float bestD = radius;
    for (const auto& e : world->GetEntities()) {
        if (!e->IsAlive()) continue;
        Carcass* c = dynamic_cast<Carcass*>(e.get());
        if (!c) continue;
        float d = Vector3Distance(position, c->GetPosition());
        if (d < bestD) { bestD = d; best = c; }
    }
    return best;
}

// Ищет любого чужого волка (другой packId) ADULT или MEDIUM в радиусе
Wolf* Wolf::FindNearestOtherWolf(World* world, float radius) const {
    if (ageStage == Config::Wolf::AgeStage::BABY) return nullptr;
    Wolf* best = nullptr; float bestD = radius;
    for (const auto& e : world->GetEntities()) {
        if (!e->IsAlive()) continue;
        Wolf* w = dynamic_cast<Wolf*>(e.get());
        if (!w || w == this) continue;
        if (w->ageStage == Config::Wolf::AgeStage::BABY) continue;
        if (w->packId == packId) continue; // своя стая
        if (w->fightCooldownTimer > 0.0f) continue;
        if (w->isMating) continue;
        float d = Vector3Distance(position, w->GetPosition());
        if (d < bestD) { bestD = d; best = w; }
    }
    return best;
}

// ═════════════════════════════════════════════════════════════════════════════
//                      СОСТОЯНИЕ: IDLE (сытый, в лесу)
// ═════════════════════════════════════════════════════════════════════════════

void Wolf::UpdateIdle(float dt, World* world) {
    // Если проголодался — на охоту
    if (hunger < Config::Wolf::HUNGER_HUNT_TRIGGER) {
        state = AnimalState::HUNTING;
        PickMeadowHuntTarget(world);
        return;
    }
    // IDLE — просто ждём короткое время, потом бродим.
    // Без таймера состояние переключается каждый кадр и сбрасывает цель.
    idleTimer -= dt;
    if (idleTimer <= 0.0f) {
        idleTimer = (float)GetRandomValue(20, 50) / 10.0f; // 2-5 сек
        state = AnimalState::WANDERING;
        PickForestWanderTarget(world);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//              СОСТОЯНИЕ: WANDERING (бродит по лесу, ищет партнёра)
// ═════════════════════════════════════════════════════════════════════════════

void Wolf::UpdateWandering(float dt, World* world) {
    if (isSheepFrenzy) {
        state = AnimalState::HUNTING;
        return;
    }

    // Голод — прервать и идти на охоту
    if (hunger < Config::Wolf::HUNGER_HUNT_TRIGGER) {
        state = AnimalState::HUNTING;
        PickMeadowHuntTarget(world);
        return;
    }

    // ПОЕДАНИЕ ТРАВЫ ПО ПУТИ (если еще не переел)
    if (ageStage != Config::Wolf::AgeStage::BABY && grassEatenCount < 5 && 
    grassCooldownTimer <= 0.0f) {
        
        // ИСПРАВЛЕНИЕ: Ищем новую траву ТОЛЬКО если у нас сейчас нет активной цели!
        if (targetGrassIndex == -1) {
            Vector3 grassPos;
            Config::Grass::Type grassType;
            int grassIdx = -1;
            float currentRadius = 30.0f; // Начальный радиус
            float maxRadius = 150.0f;    // Максимальный радиус поиска
            float radiusStep = 30.0f;    // Шаг расширения

            // Ищем свободную траву, расширяя радиус
            while (currentRadius <= maxRadius) {
                grassIdx = world->FindNearestGrass(position, currentRadius, grassPos, grassType);
                if (grassIdx != -1) break; // Нашли свободную!
                currentRadius += radiusStep;   
            }

            if (grassIdx >= 0) {
                targetGrassIndex = grassIdx;
                world->SetGrassReserved(targetGrassIndex, true); // Бронируем куст
                targetPosition = grassPos;                       // Запоминаем точные координаты
            }
        }

        // Если цель уже выбрана — уверенно идём к ней
        if (targetGrassIndex != -1) {
            MoveSwimming(dt, world);
            
            // ИСПРАВЛЕНИЕ: 2D дистанция вместо 3D, чтобы не зависать на склонах
            float d2d = Vector2Distance({position.x, position.z}, {targetPosition.x, targetPosition.z});
            
            if (d2d < 1.5f) {
                world->EatGrass(targetGrassIndex); 
                targetGrassIndex = -1;             
                grassEatenCount++;
                grassCooldownTimer = 90.0f;
                if (grassEatenCount >= 5) {
                    isSheepFrenzy = true;
                    state = AnimalState::HUNTING;
                }
            }
            return; 
        }
    }

    // Территориальная атака: овца в лесу — атакуем независимо от голода
    {
        Sheep* intruder = FindNearestSheepInRadius(world, 20.0f);
        if (intruder) {
            float sh = world->GetHeight(
                intruder->GetPosition().x, intruder->GetPosition().z);
            bool sheepInForest = (sh > Config::World::SAND_LEVEL
                               && sh <= Config::World::BIOME_THRESHOLD);
            if (sheepInForest) {
                targetPrey = intruder;
                state      = AnimalState::HUNTING;
                return;
            }
        }
    }

    // Ищем партнёра среди ВСЕХ волков (своя стая + чужие)
    if (CanMate()) {
        // Сначала ищем в своей стае
        Wolf* packMate = nullptr;
        float bestD = Config::Wolf::MATING_SEARCH_RADIUS;
        for (const auto& e : world->GetEntities()) {
            if (!e->IsAlive()) continue;
            Wolf* w = dynamic_cast<Wolf*>(e.get());
            if (!w || w == this || !w->CanMate()) continue;
            if (w->packId != packId) continue;
            float d = Vector3Distance(position, w->GetPosition());
            if (d < bestD) { bestD = d; packMate = w; }
        }

        if (packMate) {
            // Своя стая — всегда спариваемся
            mateTarget         = packMate;
            isMating           = true;
            packMate->mateTarget = this;
            packMate->isMating   = true;
            state = AnimalState::MATING;
            return;
        }

        // Чужой волк поблизости — бросок 60/40
        Wolf* outsider = FindNearestOtherWolf(world, Config::Wolf::MATING_SEARCH_RADIUS);
        if (outsider && outsider->CanMate()) {
            if (GetRandomValue(0, 99) < 80) {
                // 80% — спариваемся
                mateTarget           = outsider;
                isMating             = true;
                outsider->mateTarget = this;
                outsider->isMating   = true;
                state = AnimalState::MATING;
            } else {
                // 20% — атакуем
                if (fightCooldownTimer <= 0.0f && !outsider->isMating) {
                    state              = AnimalState::FIGHTING;
                    fightTarget        = outsider;
                    outsider->state    = AnimalState::FIGHTING;
                    outsider->fightTarget = this;
                }
            }
            return;
        }
    }

    // Двигаемся к текущей цели
    MoveSwimming(dt, world);

    float d2d = Vector2Distance({position.x, position.z},
                                {targetPosition.x, targetPosition.z});
    if (d2d < 2.0f) {
        // Дошли — выбираем новую точку в лесу и коротко отдыхаем
        state     = AnimalState::IDLE;
        idleTimer = (float)GetRandomValue(10, 30) / 10.0f;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//              СОСТОЯНИЕ: HUNTING (голодный, вышел на луга)
// ═════════════════════════════════════════════════════════════════════════════

void Wolf::UpdateHunting(float dt, World* world) {
    // Наелся — возвращаемся в лес и сразу ищем партнёра
    if (hunger >= Config::Wolf::HUNGER_RETURN_THRESHOLD) {
        state = AnimalState::MATING; // сразу пробуем найти партнёра
        matingCooldownTimer = 0.0f;  // небольшой сброс чтобы не ждать
        PickForestWanderTarget(world);
        return;
    }

    float huntR = CurrentHuntingRadius();

    // Приоритет 1: труп — лёгкая еда
    if (!isSheepFrenzy) {
        Carcass* carcass = FindNearestCarcassInRadius(world, huntR);
        if (carcass) {
            targetPosition = carcass->GetPosition();
            MoveSwimming(dt, world);
            if (Vector3Distance(position, carcass->GetPosition()) < 1.5f) {
                float nutrition = carcass->GetNutrition();
                bool  rotten    = carcass->IsRotten();
                hunger = fminf(hunger + nutrition, 100.0f);
                Vector3 fp = carcass->GetPosition(); fp.y += 0.3f;
                world->SpawnParticles(fp, (Color){140,60,50,255}, 10, false);
                world->SpawnParticles(fp, (Color){90,70,50,255},  6,  false);
                carcass->Die();
                if (rotten && GetRandomValue(0, 99) < 50) {
                    world->SpawnParticles(position, (Color){80,130,60,255}, 18, false);
                    Die(DeathCause::OTHER);
                }
            }
            return;
        }
    }

    // Приоритет 1.5: Активный поиск травы, если нужно набрать 5 штук
    if (!isSheepFrenzy && ageStage != Config::Wolf::AgeStage::BABY && grassEatenCount < 5 && 
    grassCooldownTimer <= 0.0f) {
        
        // ИСПРАВЛЕНИЕ: Ищем новую траву ТОЛЬКО если у нас сейчас нет активной цели!
        if (targetGrassIndex == -1) {
            Vector3 grassPos;
            Config::Grass::Type grassType;
            int grassIdx = -1;
            float currentRadius = 30.0f;
            float maxRadius = 150.0f;    
            float radiusStep = 30.0f;

            while (currentRadius <= maxRadius) {
                grassIdx = world->FindNearestGrass(position, currentRadius, grassPos, grassType);
                if (grassIdx != -1) break;
                currentRadius += radiusStep;
            }

            if (grassIdx >= 0) {
                targetGrassIndex = grassIdx;
                world->SetGrassReserved(targetGrassIndex, true);
                targetPosition = grassPos;
            }
        }

        // Если цель уже выбрана — уверенно идём к ней
        if (targetGrassIndex != -1) {
            MoveSwimming(dt, world);
            
            // ИСПРАВЛЕНИЕ: 2D дистанция вместо 3D
            float d2d = Vector2Distance({position.x, position.z}, {targetPosition.x, targetPosition.z});
            
            if (d2d < 1.5f) {
                world->EatGrass(targetGrassIndex); 
                targetGrassIndex = -1;
                grassEatenCount++;
                grassCooldownTimer = 90.0f;
                if (grassEatenCount >= 5) {
                    isSheepFrenzy = true; 
                }
            }
            return; 
        }
    }

    // Приоритет 2: овца
    Sheep* prevTarget = targetPrey;
    targetPrey = FindNearestSheepInRadius(world, huntR);
    if (targetPrey != prevTarget) hasPrevPreyPos = false;

    if (targetPrey) {
        Vector3 preyPos = targetPrey->GetPosition();
        float   dist    = Vector3Distance(position, preyPos);

        // Упреждение
        Vector3 leadPos = preyPos;
        float leadT = CurrentLeadTime();
        if (hasPrevPreyPos && dt > 0.001f) {
            Vector3 vel = {
                (preyPos.x - prevPreyPos.x) / dt, 0.0f,
                (preyPos.z - prevPreyPos.z) / dt
            };
            float t = fminf(dist / fmaxf(speed, 0.1f), leadT);
            leadPos.x = preyPos.x + vel.x * t;
            leadPos.z = preyPos.z + vel.z * t;
            leadPos.y = world->GetHeight(leadPos.x, leadPos.z);
        }
        prevPreyPos    = preyPos;
        hasPrevPreyPos = true;
        targetPosition = leadPos;

        TryStartPounce(dist, world);
        MoveSwimming(dt, world);

        float reach = (pounceTimer > 0.0f) ? CurrentPounceReach() : Config::Wolf::ATTACK_RADIUS;
        if (ageStage == Config::Wolf::AgeStage::ADULT)
            reach = fmaxf(reach, Config::Wolf::ADULT_POUNCE_REACH);

        if (Vector3Distance(position, targetPrey->GetPosition()) < reach) {
            Vector3 killPos = targetPrey->GetPosition(); killPos.y += 0.5f;
            world->SpawnParticles(killPos, (Color){200,30,30,255}, 20, false);
            world->SpawnParticles(killPos, (Color){120,10,10,255}, 10, false);
            targetPrey->Die(DeathCause::EATEN_BY_WOLF);

            if (ageStage == Config::Wolf::AgeStage::BABY) {
                babyKillCount++;
                hunger = fminf(hunger + 60.0f, 100.0f);
                if (babyKillCount >= Config::Wolf::BABY_KILLS_TO_FULL) {
                    babyKillCount = 0;
                    pounceCooldownTimer = FullCooldown();
                }
            } else {
                hunger = fminf(hunger + 60.0f, 100.0f);
                pounceCooldownTimer = FullCooldown();
            }
            targetPrey     = nullptr;
            hasPrevPreyPos = false;
            pounceTimer    = 0.0f;

            // СБРОС СОСТОЯНИЯ ПОСЛЕ СЪЕДЕНИЯ ОВЦЫ
            if (isSheepFrenzy) {
                isSheepFrenzy = false;
                grassEatenCount = 0;
            }

            // Если убили в лесу (территориальная охота) — возвращаемся бродить
            if (IsInForest(world)) {
                state = AnimalState::WANDERING;
                PickForestWanderTarget(world);
            }
        }
        return;
    }

    // Овец нет — продолжаем идти по лугам в поисках
    if (Vector3Distance(position, targetPosition) < 3.0f)
        PickMeadowHuntTarget(world);
    MoveSwimming(dt, world);
}

// ═════════════════════════════════════════════════════════════════════════════
//              СОСТОЯНИЕ: MATING (ищет/идёт к партнёру)
// ═════════════════════════════════════════════════════════════════════════════

void Wolf::UpdateMating(float dt, World* world) {
    // Голод прерывает спаривание
    if (hunger < Config::Wolf::HUNGER_HUNT_TRIGGER) {
        if (mateTarget) {
            mateTarget->isMating   = false;
            mateTarget->mateTarget = nullptr;
        }
        isMating           = false;
        matingProgressTimer = 0.0f;
        mateTarget         = nullptr;
        state = AnimalState::HUNTING;
        PickMeadowHuntTarget(world);
        return;
    }

    // Если нет партнёра — ищем
    if (!mateTarget) {
        if (!CanMate()) {
            // Откат ещё идёт или другое условие — просто бродим
            state = AnimalState::WANDERING;
            PickForestWanderTarget(world);
            return;
        }

        // Поиск: сначала своя стая, потом чужие
        Wolf* best = nullptr; float bestD = Config::Wolf::MATING_SEARCH_RADIUS;
        for (const auto& e : world->GetEntities()) {
            if (!e->IsAlive()) continue;
            Wolf* w = dynamic_cast<Wolf*>(e.get());
            if (!w || w == this || !w->CanMate()) continue;
            float d = Vector3Distance(position, w->GetPosition());
            if (d < bestD) { bestD = d; best = w; }
        }

        if (best) {
            bool samepack = (best->packId == packId);
            bool doMate   = samepack || (GetRandomValue(0, 99) < 60);

            if (doMate) {
                mateTarget       = best;
                isMating         = true;
                best->mateTarget = this;
                best->isMating   = true;
            } else {
                // 40% — атакуем чужого
                if (fightCooldownTimer <= 0.0f && !best->isMating) {
                    state           = AnimalState::FIGHTING;
                    fightTarget     = best;
                    best->state     = AnimalState::FIGHTING;
                    best->fightTarget = this;
                }
                return;
            }
        } else {
            // Партнёра нет — бродим по лесу
            state = AnimalState::WANDERING;
            PickForestWanderTarget(world);
            return;
        }
    }

    // Партнёр мёртв или передумал
    if (!mateTarget->IsAlive() || !mateTarget->isMating
        || mateTarget->state == AnimalState::FIGHTING) {
        isMating           = false;
        matingProgressTimer = 0.0f;
        mateTarget         = nullptr;
        state = AnimalState::WANDERING;
        PickForestWanderTarget(world);
        return;
    }

    // Идём к партнёру
    targetPosition = mateTarget->GetPosition();
    float dist = Vector3Distance(position, mateTarget->GetPosition());

    if (dist > Config::Wolf::MATING_APPROACH_DIST) {
        MoveSwimming(dt, world);
        matingProgressTimer = 0.0f;
    } else {
        matingProgressTimer += dt;

        // Сердечки каждые 0.5 сек
        if (fmodf(matingProgressTimer, 0.5f) < dt) {
            world->SpawnParticles(
                {position.x, position.y + 2.0f, position.z},
                (Color){180,180,180,255}, 3, true);
        }

        if (matingProgressTimer >= Config::Wolf::MATING_PROCESS_TIME && this > mateTarget) {
            // Спавн детёныша
            Vector3 babyPos = {
                (position.x + mateTarget->GetPosition().x) * 0.5f, 0.0f,
                (position.z + mateTarget->GetPosition().z) * 0.5f
            };
            babyPos.y = world->GetHeight(babyPos.x, babyPos.z);

            // Детёныш входит в стаю отца (того кто this > mateTarget, т.е. инициатор)
            auto baby = std::make_unique<Wolf>(babyPos, Config::Wolf::AgeStage::BABY, packId);
            baby->SetHomePos(homePos);
            world->QueueEntity(std::move(baby));

            matingCooldownTimer             = Config::Wolf::MATING_COOLDOWN;
            mateTarget->matingCooldownTimer = Config::Wolf::MATING_COOLDOWN;
            isMating                        = false;
            mateTarget->isMating            = false;
            matingProgressTimer             = 0.0f;
            mateTarget->matingProgressTimer = 0.0f;
            mateTarget->mateTarget          = nullptr;
            mateTarget                      = nullptr;

            state = AnimalState::IDLE;
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//              СОСТОЯНИЕ: FIGHTING (бой)
// ═════════════════════════════════════════════════════════════════════════════

void Wolf::UpdateFighting(float dt, World* world) {
    if (!fightTarget || !fightTarget->IsAlive()) {
        fightTarget        = nullptr;
        fightHealth        = Config::Wolf::FIGHT_MAX_HEALTH;
        fightCooldownTimer = Config::Wolf::FIGHT_COOLDOWN;
        state = AnimalState::WANDERING;
        PickForestWanderTarget(world);
        return;
    }

    targetPosition = fightTarget->GetPosition();
    MoveSwimming(dt, world);

    float dist = Vector3Distance(position, fightTarget->GetPosition());
    if (dist < Config::Wolf::FIGHT_CONTACT_DIST) {
        fightHealth -= Config::Wolf::FIGHT_DAMAGE_RATE * dt;
        if (GetRandomValue(0, 100) < 8) {
            Vector3 fp = {
                (position.x + fightTarget->GetPosition().x) * 0.5f,
                position.y + 0.5f,
                (position.z + fightTarget->GetPosition().z) * 0.5f
            };
            world->SpawnParticles(fp, (Color){140,110,80,255}, 4, false);
        }
        if (fightHealth <= 0.0f) {
            if (targetGrassIndex != -1) world->SetGrassReserved(targetGrassIndex, false);
            world->SpawnParticles(position, (Color){150,60,60,255}, 12, false);
            diedInFight = true;
            Die(DeathCause::FIGHT);
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//                          ОСНОВНОЙ UPDATE
// ═════════════════════════════════════════════════════════════════════════════

void Wolf::Update(float deltaTime, World* world) {
    if (!isAlive) return;

    if (playerControlled) {
        UpdatePlayerControl(deltaTime, world);
        return;
    }

    if (grassCooldownTimer > 0.0f) {
        grassCooldownTimer -= deltaTime;
    }

    // === ИНСТИНКТ САМОСОХРАНЕНИЯ (Побег от охотника) ===
    Hunter* dangerHunter = nullptr;
    float wolfVisionRadius = 35.0f; // Дистанция, на которой волк замечает охотника

    // Ищем охотника среди существ в мире
    for (const auto& entity : world->GetEntities()) {
        if (!entity->IsAlive()) continue;
        Hunter* h = dynamic_cast<Hunter*>(entity.get());
        if (h) {
            float dist = Vector3Distance(position, h->GetPosition());
            if (dist < wolfVisionRadius) {
                dangerHunter = h;
                break; // Нашли ближайшую угрозу
            }
        }
    }

    if (dangerHunter) {
        // Сбрасываем мирные цели, так как началась паника
        targetPrey = nullptr;
        mateTarget = nullptr;
        isMating = false;

        if (targetGrassIndex != -1) {
            world->SetGrassReserved(targetGrassIndex, false);
            targetGrassIndex = -1;
        }
        // Вычисляем вектор направления ОТ охотника
        Vector3 fleeDirection = Vector3Normalize(Vector3Subtract(position, dangerHunter->GetPosition()));
        
        // Скорость панического бега (делаем её выше стандартного бега волка)
        float panicSpeed = Config::Wolf::SPEED_RUN * 1.5f; 
        
        // Точка, в направлении которой волк делает шаг на этом кадре
        Vector3 targetFleePos = Vector3Add(position, Vector3Scale(fleeDirection, 5.0f));
        
        // Передвигаем волка аналогично логике движения охотника
        Vector3 dir = { targetFleePos.x - position.x, 0.0f, targetFleePos.z - position.z };
        float distance = Vector3Length(dir);
        if (distance > 0.1f) {
            facingAngle = atan2f(dir.x, dir.z) * RAD2DEG;
            Vector3 normDir = Vector3Normalize(dir);
            float step = panicSpeed * deltaTime;
            
            float nx = position.x + normDir.x * step;
            float nz = position.z + normDir.z * step;
            
            // Проверка, чтобы волк не забегал глубоко в воду при побеге
            if (world->GetHeight(nx, nz) > world->GetCurrentWaterLevel()) {
                position.x = nx;
                position.z = nz;
            }
        }
        position.y = world->GetHeight(position.x, position.z);

        // Визуальный эффект: паникующий волк иногда оставляет за собой облачка серой пыли
        if (GetRandomValue(0, 6) == 0) {
            world->SpawnParticles(position, GRAY, 1, false);
        }

        // ВАЖНО: Во время побега жизненные параметры волка должны продолжать обновляться!
        hunger -= Config::Wolf::HUNGER_DECAY_RATE * deltaTime;
        ageTimer += deltaTime;
        
        if (hunger <= 0.0f) {
            hunger = 0.0f;
            // Если у вас реализована смерть от истощения прямо в Update, вызовите её:
            // Die(DeathCause::STARVATION);
        }

        return; // Завершаем Update для этого кадра, игнорируя обычный switch(state)
    }
    
    // ── Защита от застревания ────────────────────────────────────────────────
    stuckCheckTimer += deltaTime;
    if (stuckCheckTimer >= 1.0f) {
        stuckCheckTimer = 0.0f;
        if (state == AnimalState::WANDERING || state == AnimalState::HUNTING) {
            if (Vector3Distance(position, posAtLastCheck) < 0.3f) stuckCount++;
            else stuckCount = 0;
        }
        posAtLastCheck = position;
    }
    if (stuckCount >= 3) {
        stuckCount = 0;
        targetPrey = nullptr;
        
        // КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: Если волк застрял по пути к траве (например, она в дереве), сбрасываем цель!
        if (targetGrassIndex != -1) {
            world->SetGrassReserved(targetGrassIndex, false);
            targetGrassIndex = -1;
        }

        if (state == AnimalState::HUNTING) PickMeadowHuntTarget(world);
        else PickForestWanderTarget(world);
    }
    // ── Возраст ──────────────────────────────────────────────────────────────
    UpdateAge(deltaTime, world);
    if (!IsAlive()) return;

    // ── Таймеры ──────────────────────────────────────────────────────────────
    if (pounceCooldownTimer > 0.0f) pounceCooldownTimer -= deltaTime;
    if (matingCooldownTimer > 0.0f) matingCooldownTimer -= deltaTime;
    if (fightCooldownTimer  > 0.0f) fightCooldownTimer  -= deltaTime;
    if (grassCooldownTimer  > 0.0f) grassCooldownTimer  -= deltaTime;

    // ── Голод ────────────────────────────────────────────────────────────────
    hunger -= Config::Wolf::HUNGER_DECAY_RATE * deltaTime;
    if (hunger < 0.0f) hunger = 0.0f;

    // ── Смерть от голода ─────────────────────────────────────────────────────
    if (hunger <= 0.0f) {
        starvationTimer += deltaTime;
        if (starvationTimer >= Config::Wolf::STARVATION_LIMIT) {
            if (targetGrassIndex != -1) world->SetGrassReserved(targetGrassIndex, false);
            world->SpawnParticles(position, (Color){90,90,90,255}, 14, false);
            world->SpawnParticles(position, (Color){60,50,40,255}, 8,  false);
            Die(DeathCause::STARVATION);
            return;
        }
    } else {
        starvationTimer = 0.0f;
    }

    // ── Встряхивание блокирует всё ───────────────────────────────────────────
    if (shakeTimer > 0.0f) {
        shakeTimer -= deltaTime; swimSplashTimer -= deltaTime;
        if (swimSplashTimer <= 0.0f) {
            swimSplashTimer = 0.15f;
            world->SpawnParticles(
                {position.x, position.y + 0.6f, position.z},
                (Color){100,160,220,255}, 5, false);
        }
        return;
    }

    // ── Скорость ─────────────────────────────────────────────────────────────
    float baseSpeed = Config::Wolf::SPEED_RUN * CurrentSpeedFactor();
    if      (pounceTimer        > 0.0f) { pounceTimer -= deltaTime; speed = Config::Wolf::POUNCE_SPEED * CurrentSpeedFactor(); }
    else if (pounceCooldownTimer > 0.0f)  speed = baseSpeed * Config::Wolf::REST_SPEED_FACTOR;
    else                                  speed = baseSpeed;

    // ── Мёртвая жертва ───────────────────────────────────────────────────────
    if (targetPrey && !targetPrey->IsAlive()) {
        targetPrey = nullptr; hasPrevPreyPos = false;
    }

    // ── BABY не охотятся и не дерутся — только растут и иногда идут к воде ───
    if (ageStage == Config::Wolf::AgeStage::BABY) {
        // Ищем ближайшего взрослого из своей стаи — идём за ним
        Wolf* packAdult = nullptr;
        float bestD = 60.0f;
        for (const auto& e : world->GetEntities()) {
            if (!e->IsAlive()) continue;
            Wolf* w = dynamic_cast<Wolf*>(e.get());
            if (!w || w == this) continue;
            if (w->packId != packId) continue;
            if (w->ageStage == Config::Wolf::AgeStage::BABY) continue;
            float d = Vector3Distance(position, w->GetPosition());
            if (d < bestD) { bestD = d; packAdult = w; }
        }

        if (packAdult) {
            // Идём за взрослым — держимся в 3-8 единицах от него
            float distToAdult = Vector3Distance(position, packAdult->GetPosition());
            if (distToAdult > 8.0f) {
                targetPosition = packAdult->GetPosition();
            } else if (distToAdult < 3.0f) {
                // Слишком близко — немного отходим в сторону
                Vector3 away = Vector3Subtract(position, packAdult->GetPosition());
                away.y = 0.0f;
                if (Vector3Length(away) < 0.01f) away = { 1.0f, 0.0f, 0.0f };
                away = Vector3Normalize(away);
                targetPosition = {
                    position.x + away.x * 4.0f,
                    position.y,
                    position.z + away.z * 4.0f
                };
            }
            // Взрослый идёт к воде (вышел из леса) — детёнышу разрешаем следовать
            // даже в воду: MoveSwimming сам обработает плавание
        } else {
            // Взрослых нет рядом — держимся у homePos
            if (Vector3Distance(position, targetPosition) < 2.0f ||
                Vector3Distance(position, homePos) > Config::Wolf::HOME_PATROL_RADIUS * 0.5f) {
                PickForestWanderTarget(world);
            }
        }

        MoveSwimming(deltaTime, world);
        // Снап к рельефу только если не в воде
        float th = world->GetHeight(position.x, position.z);
        if (th > world->GetCurrentWaterLevel()) position.y = th;
        return;
    }

    if (isSheepFrenzy) {
        state = AnimalState::HUNTING; // Принудительно удерживаем состояние охоты
    }

    // ── Машина состояний ─────────────────────────────────────────────────────
    switch (state) {
        case AnimalState::IDLE:     UpdateIdle     (deltaTime, world); break;
        case AnimalState::WANDERING: UpdateWandering(deltaTime, world); break;
        case AnimalState::HUNGRY:   // HUNGRY не используется, переадресуем
        case AnimalState::HUNTING:  UpdateHunting  (deltaTime, world); break;
        case AnimalState::MATING:   UpdateMating   (deltaTime, world); break;
        case AnimalState::FIGHTING: UpdateFighting (deltaTime, world); break;
        case AnimalState::FLEEING:  state = AnimalState::WANDERING; break;
    }

    // ── Снап к рельефу ───────────────────────────────────────────────────────
    const int halfMap = Config::World::MAP_SIZE / 2;
    position.x = Clamp(position.x, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
    position.z = Clamp(position.z, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
    float th = world->GetHeight(position.x, position.z);
    if (th > world->GetCurrentWaterLevel()) position.y = th;
}

// ═════════════════════════════════════════════════════════════════════════════
//                          ОТРИСОВКА
// ═════════════════════════════════════════════════════════════════════════════

void Wolf::Draw() {
    Vector3 drawPos = position;

    if (pounceTimer > 0.0f) {
        float t = 1.0f - (pounceTimer / Config::Wolf::POUNCE_DURATION);
        drawPos.y += sinf(t * PI) * 0.8f;
    }
    if (shakeTimer > 0.0f) {
        float t = shakeTimer / Config::Wolf::SHAKE_DURATION;
        drawPos.x += sinf(t * 60.0f) * Config::Wolf::SHAKE_AMPLITUDE;
        drawPos.z += cosf(t * 55.0f) * Config::Wolf::SHAKE_AMPLITUDE * 0.8f;
        drawPos.y += fabsf(sinf(t * 30.0f)) * Config::Wolf::SHAKE_AMPLITUDE * 0.6f;
    }
    if (state == AnimalState::FIGHTING)
        drawPos.x += sinf(GetTime() * 25.0f) * 0.08f;

    Vector3 delta = { position.x - lastDrawPos.x, 0.0f, position.z - lastDrawPos.z };
    if (Vector3Length(delta) > 0.02f)
        facingAngle = atan2f(delta.x, delta.z) * RAD2DEG;
    lastDrawPos = position;

    float size = 1.4f * CurrentSizeFactor();

    Color body = DARKGRAY;
    if (ageStage == Config::Wolf::AgeStage::ADULT && isLeader)
        body = (Color){55,50,55,255};
    else if (ageStage == Config::Wolf::AgeStage::BABY)
        body = (Color){120,110,110,255};

    float eyeSize = 0.2f * CurrentSizeFactor();
    if (ageStage == Config::Wolf::AgeStage::ADULT && isLeader) eyeSize *= 1.3f;

    rlPushMatrix();
    rlTranslatef(drawPos.x, drawPos.y, drawPos.z);
    rlRotatef(facingAngle, 0.0f, 1.0f, 0.0f);
    DrawCube({0,0,0}, size, size, size, body);
    float eyeOff = 0.3f * CurrentSizeFactor();
    DrawCube({-eyeOff, eyeOff, size*0.5f}, eyeSize, eyeSize, eyeSize, RED);
    DrawCube({ eyeOff, eyeOff, size*0.5f}, eyeSize, eyeSize, eyeSize, RED);
    rlPopMatrix();
}