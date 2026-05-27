#include "wolf.hpp"
#include "sheep.hpp"
#include "carcass.hpp"
#include "../core/world.hpp"
#include "../core/constants.hpp"
#include <raymath.h>
#include <cmath>

// =====================================================================
//                    КОНСТРУКТОР И ВСПОМОГАТЕЛЬНЫЕ
// =====================================================================

Wolf::Wolf(Vector3 startPosition,
           Config::Wolf::AgeStage startStage,
           int wolfPackId) : Animal(startPosition) {
    hunger     = 100.0f;
    state      = AnimalState::WANDERING;
    targetPrey = nullptr;
    packId     = wolfPackId;
    ageStage   = startStage;
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

    matingCooldownTimer = (float)GetRandomValue(20, 60);
    speed = Config::Wolf::SPEED_RUN * CurrentSpeedFactor();
    homePos = startPosition;

    // Сразу выбираем первую патрульную точку
    float a = (float)GetRandomValue(0, 360) * DEG2RAD;
    float r = Config::Wolf::HOME_PATROL_RADIUS * 0.4f
            + (float)GetRandomValue(0, (int)(Config::Wolf::HOME_PATROL_RADIUS * 0.6f));
    targetPosition.x = startPosition.x + cosf(a) * r;
    targetPosition.z = startPosition.z + sinf(a) * r;
    targetPosition.y = startPosition.y;
}

bool Wolf::IsNarrowWater(float, float, float, float, World*) { return true; }

float Wolf::CurrentPounceReach() const {
    switch (ageStage) {
        case Config::Wolf::AgeStage::BABY:   return Config::Wolf::POUNCE_REACH;
        case Config::Wolf::AgeStage::MEDIUM: return Config::Wolf::MEDIUM_POUNCE_REACH;
        case Config::Wolf::AgeStage::ADULT:  return Config::Wolf::ADULT_POUNCE_REACH;
    }
    return Config::Wolf::POUNCE_REACH;
}

float Wolf::CurrentHuntingRadius() const {
    switch (ageStage) {
        case Config::Wolf::AgeStage::BABY:   return Config::Wolf::HUNTING_RADIUS_BABY;
        case Config::Wolf::AgeStage::MEDIUM: return Config::Wolf::HUNTING_RADIUS_MEDIUM;
        case Config::Wolf::AgeStage::ADULT:  return Config::Wolf::HUNTING_RADIUS_ADULT;
    }
    return Config::Wolf::HUNTING_RADIUS_MEDIUM;
}

float Wolf::CurrentLeadTime() const {
    return (ageStage == Config::Wolf::AgeStage::ADULT)
           ? Config::Wolf::ADULT_LEAD_MAX_TIME
           : Config::Wolf::LEAD_MAX_TIME;
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
    if (ageStage == Config::Wolf::AgeStage::BABY) return Config::Wolf::BABY_SPEED_FACTOR;
    return 1.0f;
}

float Wolf::FullCooldown() const {
    if (ageStage == Config::Wolf::AgeStage::BABY)
        return Config::Wolf::POUNCE_COOLDOWN * Config::Wolf::BABY_COOLDOWN_MULT;
    return Config::Wolf::POUNCE_COOLDOWN;
}

void Wolf::PromoteToLeader() {
    ageStage             = Config::Wolf::AgeStage::ADULT;
    maxAgeInCurrentStage = Config::Wolf::TIME_ADULT_LIFESPAN;
    ageTimer             = 0.0f;
    isLeader             = true;
}

bool Wolf::CanMate() const {
    return ageStage             == Config::Wolf::AgeStage::ADULT
        && hunger               >= Config::Wolf::MATING_HUNGER_THRESHOLD
        && matingCooldownTimer  <= 0.0f
        && mateTarget           == nullptr
        && !isMating
        && state                != AnimalState::HUNGRY
        && state                != AnimalState::FIGHTING
        && shakeTimer           <= 0.0f
        && isAlive;
}

// =====================================================================
//                              ДВИЖЕНИЕ
// =====================================================================

void Wolf::MoveSwimming(float deltaTime, World* world) {
    Vector3 direction = {
        targetPosition.x - position.x, 0.0f,
        targetPosition.z - position.z
    };
    float distance = Vector3Length(direction);
    if (distance < 0.1f) {
        position.y = fmaxf(world->GetHeight(position.x, position.z),
                           world->GetCurrentWaterLevel());
        return;
    }

    Vector3 normDir = Vector3Normalize(direction);
    float terrainHere = world->GetHeight(position.x, position.z);
    bool inWaterNow = (terrainHere <= world->GetCurrentWaterLevel());

    float curSpeed = speed * (inWaterNow ? Config::Wolf::SWIM_SPEED_FACTOR : 1.0f);
    float step = curSpeed * deltaTime;

    // Границы карты — волк никогда не выходит за пределы
    const int halfMap = Config::World::MAP_SIZE / 2;
    const float minB = -(float)halfMap + 2.0f;
    const float maxB =  (float)halfMap - 2.0f;

    position.x = Clamp(position.x + normDir.x * step, minB, maxB);
    position.z = Clamp(position.z + normDir.z * step, minB, maxB);
    position = world->ResolveTreeCollisions(position, 0.4f);
    position.x = Clamp(position.x, minB, maxB);
    position.z = Clamp(position.z, minB, maxB);

    float terrainNew = world->GetHeight(position.x, position.z);
    if (terrainNew <= world->GetCurrentWaterLevel()) {
        position.y = world->GetCurrentWaterLevel() + 0.05f;
        if (!wasInWater) wasInWater = true;
        swimSplashTimer -= deltaTime;
        if (swimSplashTimer <= 0.0f) {
            swimSplashTimer = 0.25f;
            world->SpawnParticles(position, (Color){100, 160, 220, 255}, 4, false);
        }
    } else {
        position.y = terrainNew;
        if (wasInWater) {
            shakeTimer = Config::Wolf::SHAKE_DURATION;
            world->SpawnParticles(position, (Color){100, 160, 220, 255}, 16, false);
            wasInWater = false;
        }
    }
}

// =====================================================================
//                              ПРЫЖОК
// =====================================================================

void Wolf::TryStartPounce(float distToPrey, World* world) {
    if (pounceTimer > 0.0f) return;
    if (pounceCooldownTimer > 0.0f) return;
    if (distToPrey > Config::Wolf::POUNCE_RANGE) return;
    if (shakeTimer > 0.0f) return;
    if (world->GetHeight(position.x, position.z) <= world->GetCurrentWaterLevel()) return;

    pounceTimer         = Config::Wolf::POUNCE_DURATION;
    pounceCooldownTimer = FullCooldown();

    Vector3 dustPos = { position.x, position.y + 0.1f, position.z };
    world->SpawnParticles(dustPos, (Color){150, 130, 100, 255}, 6, false);
}

// =====================================================================
//                              ВОЗРАСТ
// =====================================================================

void Wolf::UpdateAge(float deltaTime, World* world) {
    ageTimer += deltaTime;
    if (ageTimer < maxAgeInCurrentStage) return;

    if (ageStage == Config::Wolf::AgeStage::BABY) {
        ageStage = Config::Wolf::AgeStage::MEDIUM;
        maxAgeInCurrentStage = (float)GetRandomValue(
            (int)Config::Wolf::TIME_TO_GROW_MEDIUM_MIN,
            (int)Config::Wolf::TIME_TO_GROW_MEDIUM_MAX);
        ageTimer = 0.0f;

        // Если у стаи нет вожака — новоиспечённый MEDIUM сразу его наследует
        bool packHasLeader = false;
        for (const auto& e : world->GetEntities()) {
            if (!e->IsAlive() || e.get() == this) continue;
            Wolf* w = dynamic_cast<Wolf*>(e.get());
            if (w && w->packId == packId
                && w->ageStage == Config::Wolf::AgeStage::ADULT) {
                packHasLeader = true; break;
            }
        }
        if (!packHasLeader) PromoteToLeader();
    } else if (ageStage == Config::Wolf::AgeStage::MEDIUM) {
        ageStage = Config::Wolf::AgeStage::ADULT;
        maxAgeInCurrentStage = Config::Wolf::TIME_ADULT_LIFESPAN;
        ageTimer = 0.0f;
    } else {
        // ADULT — смерть от старости. Серые партиклы (как у овец).
        diedOfOldAge = true;
        world->SpawnParticles(position, (Color){180, 180, 180, 255}, 14, false);
        Die();
    }
}

// =====================================================================
//                            РАЗМНОЖЕНИЕ
// =====================================================================

void Wolf::TryAttemptMating(float deltaTime, World* world) {
    if (CanMate()) {
        Wolf* bestMate = nullptr;
        float bestDist = Config::Wolf::MATING_SEARCH_RADIUS;
        for (const auto& entity : world->GetEntities()) {
            Wolf* candidate = dynamic_cast<Wolf*>(entity.get());
            if (!candidate || candidate == this || !candidate->CanMate()) continue;
            float d = Vector3Distance(position, candidate->GetPosition());
            if (d < bestDist) { bestDist = d; bestMate = candidate; }
        }
        if (bestMate) {
            mateTarget = bestMate;
            isMating = true;
            bestMate->mateTarget = this;
            bestMate->isMating = true;
        }
    }

    if (mateTarget && mateTarget->IsAlive()) {
        if (mateTarget->hunger < Config::Wolf::MATING_HUNGER_THRESHOLD) {
            isMating = false; matingProgressTimer = 0.0f;
            mateTarget->isMating = false;
            mateTarget->mateTarget = nullptr;
            mateTarget = nullptr;
            return;
        }

        targetPosition = mateTarget->GetPosition();
        float dist = Vector3Distance(position, mateTarget->GetPosition());

        if (dist > Config::Wolf::MATING_APPROACH_DIST) {
            MoveSwimming(deltaTime, world);
            matingProgressTimer = 0.0f;
        } else {
            matingProgressTimer += deltaTime;
            if (fmodf(matingProgressTimer, 0.5f) < deltaTime) {
                Vector3 heartPos = { position.x, position.y + 2.0f, position.z };
                world->SpawnParticles(heartPos, (Color){180, 180, 180, 255}, 3, true);
            }

            if (matingProgressTimer >= Config::Wolf::MATING_PROCESS_TIME
                && this > mateTarget)
            {
                Vector3 babyPos = {
                    (position.x + mateTarget->GetPosition().x) * 0.5f,
                    0.0f,
                    (position.z + mateTarget->GetPosition().z) * 0.5f
                };
                babyPos.y = world->GetHeight(babyPos.x, babyPos.z);

                auto baby = std::make_unique<Wolf>(
                    babyPos, Config::Wolf::AgeStage::BABY, packId);
                baby->SetHomePos(homePos); // наследует дом стаи
                world->QueueEntity(std::move(baby));

                matingCooldownTimer = Config::Wolf::MATING_COOLDOWN;
                mateTarget->matingCooldownTimer = Config::Wolf::MATING_COOLDOWN;
                isMating = false;
                mateTarget->isMating = false;
                matingProgressTimer = 0.0f;
                mateTarget->matingProgressTimer = 0.0f;
                mateTarget->mateTarget = nullptr;
                mateTarget = nullptr;
                state = AnimalState::IDLE;
            }
        }
    } else if (mateTarget) {
        isMating = false; matingProgressTimer = 0.0f;
        mateTarget = nullptr;
    }
}

// =====================================================================
//                              ПОИСК
// =====================================================================

Sheep* Wolf::FindNearestSheepInRadius(World* world, float radius) const {
    Sheep* best = nullptr;
    float bestDist = radius;
    for (const auto& entity : world->GetEntities()) {
        if (!entity->IsAlive()) continue;
        Sheep* sheep = dynamic_cast<Sheep*>(entity.get());
        if (!sheep) continue;
        float d = Vector3Distance(position, sheep->GetPosition());
        if (d < bestDist) { bestDist = d; best = sheep; }
    }
    return best;
}

Carcass* Wolf::FindNearestCarcassInRadius(World* world, float radius) const {
    Carcass* best = nullptr;
    float bestDist = radius;
    for (const auto& entity : world->GetEntities()) {
        if (!entity->IsAlive()) continue;
        Carcass* c = dynamic_cast<Carcass*>(entity.get());
        if (!c) continue;
        float d = Vector3Distance(position, c->GetPosition());
        if (d < bestDist) { bestDist = d; best = c; }
    }
    return best;
}

Wolf* Wolf::FindEnemyWolfNearby(World* world) const {
    // Только ADULT/MEDIUM, той же возрастной категории (BABY не дерутся)
    if (ageStage == Config::Wolf::AgeStage::BABY) return nullptr;
    if (fightCooldownTimer > 0.0f) return nullptr;

    Wolf* best = nullptr;
    float bestDist = Config::Wolf::FIGHT_TRIGGER_RADIUS;
    for (const auto& entity : world->GetEntities()) {
        if (!entity->IsAlive()) continue;
        Wolf* w = dynamic_cast<Wolf*>(entity.get());
        if (!w || w == this) continue;
        if (w->packId == packId) continue;                  // своя стая
        if (w->ageStage == Config::Wolf::AgeStage::BABY) continue; // дети не дерутся
        if (w->fightCooldownTimer > 0.0f) continue;
        float d = Vector3Distance(position, w->GetPosition());
        if (d < bestDist) { bestDist = d; best = w; }
    }
    return best;
}

// =====================================================================
//                          ПАТРУЛИРОВАНИЕ
// =====================================================================

void Wolf::PickNewPatrolTarget(World* world) {
    int halfMap = Config::World::MAP_SIZE / 2;
    // 5 попыток найти точку на суше
    for (int i = 0; i < 5; ++i) {
        float a = (float)GetRandomValue(0, 360) * DEG2RAD;
        float r = (float)GetRandomValue(
            (int)(Config::Wolf::HOME_PATROL_RADIUS * 0.2f),
            (int)Config::Wolf::HOME_PATROL_RADIUS);
        float tx = homePos.x + cosf(a) * r;
        float tz = homePos.z + sinf(a) * r;
        tx = Clamp(tx, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        tz = Clamp(tz, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        float ty = world->GetHeight(tx, tz);
        if (ty > world->GetCurrentWaterLevel()) {
            targetPosition = { tx, ty, tz };
            return;
        }
    }
    // Если не нашли — целимся прямо в homePos
    targetPosition = { homePos.x, world->GetHeight(homePos.x, homePos.z), homePos.z };
}

// =====================================================================
//                          STATE: IDLE / WANDER
// =====================================================================

void Wolf::UpdateIdle(float dt, World* world) {
    // IDLE = всегда быстро уходит в патруль с новой целью
    PickNewPatrolTarget(world);
    state = AnimalState::WANDERING;
}

void Wolf::UpdateWander(float dt, World* world) {
    MoveSwimming(dt, world);
    Vector2 flatPos    = { position.x, position.z };
    Vector2 flatTarget = { targetPosition.x, targetPosition.z };
    if (Vector2Distance(flatPos, flatTarget) < 0.7f) {
        // Достигли — выбираем новую сразу, не через IDLE-возврат
        PickNewPatrolTarget(world);
    }
}

// =====================================================================
//                          STATE: HUNGRY
// =====================================================================

void Wolf::UpdateHunt(float dt, World* world) {
    float huntR = CurrentHuntingRadius();

    // ════════════════════════════════════════════════════════════════
    // ПРИОРИТЕТ 1: ТРУП. Самый высокий — волки идут к нему первым делом.
    // На трупы НЕ прыгают, просто подходят и едят.
    // ════════════════════════════════════════════════════════════════
    Carcass* nearestCarcass = FindNearestCarcassInRadius(world, huntR);
    if (nearestCarcass) {
        Vector3 carcPos = nearestCarcass->GetPosition();
        targetPosition  = carcPos;

        // Пока бежим к трупу — никаких прыжков
        MoveSwimming(dt, world);

        float dist = Vector3Distance(position, carcPos);
        if (dist < 1.5f) {
            // Едим труп
            float nutrition = nearestCarcass->GetNutrition();
            bool rotten     = nearestCarcass->IsRotten();

            hunger = fminf(hunger + nutrition, 100.0f);

            // Партиклы поедания (тёмно-красные брызги + куски)
            Vector3 fp = carcPos; fp.y += 0.3f;
            world->SpawnParticles(fp, (Color){140, 60, 50, 255}, 10, false);
            world->SpawnParticles(fp, (Color){90, 70, 50, 255}, 6,  false);

            nearestCarcass->Die();

            // 50% шанс отравления от несвежего трупа
            if (rotten && GetRandomValue(0, 99) < 50) {
                // Зелёные «гнилостные» партиклы + смерть
                world->SpawnParticles(position, (Color){80, 130, 60, 255}, 18, false);
                world->SpawnParticles(position, (Color){60, 90, 40, 255}, 10, false);
                Die();
                return;
            }

            // После еды — снова приоритеты: есть ли ещё пища рядом?
            // (на следующем тике HUNGRY сам пересчитает)
            if (hunger >= 90.0f) {
                // Достаточно сыт — возврат домой
                targetPosition = {
                    homePos.x, world->GetHeight(homePos.x, homePos.z), homePos.z
                };
                state = AnimalState::WANDERING;
            }
            // Иначе остаёмся в HUNGRY и продолжаем искать
        }
        return;
    }

    // ════════════════════════════════════════════════════════════════
    // ПРИОРИТЕТ 2: ОВЦА. Активная охота — преследование, прыжок.
    // ════════════════════════════════════════════════════════════════
    Sheep* prevTarget = targetPrey;
    targetPrey = FindNearestSheepInRadius(world, huntR);

    if (targetPrey != prevTarget) hasPrevPreyPos = false;

    if (targetPrey) {
        Vector3 preyPos = targetPrey->GetPosition();
        float closestDistance = Vector3Distance(position, preyPos);

        Vector3 leadPos = preyPos;
        float leadT = CurrentLeadTime();
        if (hasPrevPreyPos && dt > 0.001f) {
            Vector3 preyVel = {
                (preyPos.x - prevPreyPos.x) / dt, 0.0f,
                (preyPos.z - prevPreyPos.z) / dt
            };
            float t = closestDistance / fmaxf(speed, 0.1f);
            if (t > leadT) t = leadT;
            leadPos.x = preyPos.x + preyVel.x * t;
            leadPos.z = preyPos.z + preyVel.z * t;
            leadPos.y = world->GetHeight(leadPos.x, leadPos.z);
        }
        prevPreyPos = preyPos;
        hasPrevPreyPos = true;

        targetPosition = leadPos;
        TryStartPounce(closestDistance, world);
        MoveSwimming(dt, world);

        float reach = (pounceTimer > 0.0f)
                      ? CurrentPounceReach()
                      : Config::Wolf::ATTACK_RADIUS;
        if (ageStage == Config::Wolf::AgeStage::ADULT) {
            reach = fmaxf(reach, Config::Wolf::ADULT_POUNCE_REACH);
        }

        float distAfter = Vector3Distance(position, targetPrey->GetPosition());
        if (distAfter < reach) {
            Vector3 killPos = targetPrey->GetPosition();
            killPos.y += 0.5f;
            world->SpawnParticles(killPos, (Color){200, 30, 30, 255}, 20, false);
            world->SpawnParticles(killPos, (Color){120, 10, 10, 255}, 10, false);
            targetPrey->Die();

            if (ageStage == Config::Wolf::AgeStage::BABY) {
                babyKillCount++;
                hunger = fminf(hunger + 50.0f, 100.0f); // 5/10
                if (babyKillCount >= Config::Wolf::BABY_KILLS_TO_FULL) {
                    babyKillCount = 0;
                    pounceCooldownTimer = FullCooldown();
                    state = AnimalState::WANDERING;
                    PickNewPatrolTarget(world);
                } else {
                    pounceCooldownTimer = Config::Wolf::POUNCE_COOLDOWN * 0.5f;
                }
            } else {
                hunger = fminf(hunger + 50.0f, 100.0f); // 5/10
                pounceCooldownTimer = FullCooldown();

                if (hunger < 90.0f) {
                    // Ещё голодны после одной овцы — ищем ещё
                    // (на следующем тике HUNGRY переоценит приоритеты)
                } else {
                    targetPosition = {
                        homePos.x, world->GetHeight(homePos.x, homePos.z), homePos.z
                    };
                    state = AnimalState::WANDERING;
                }
            }

            targetPrey = nullptr;
            hasPrevPreyPos = false;
            pounceTimer = 0.0f;
        }
        return;
    }

    // ════════════════════════════════════════════════════════════════
    // ПРИОРИТЕТ 3: ТРАВА. Только когда и трупов, и овец нет рядом.
    // На траву НЕ прыгают. При сильном голоде ищем шире.
    // ════════════════════════════════════════════════════════════════
    float grassR = huntR;
    if (hunger < Config::Wolf::DESPERATE_HUNGER) {
        // Очень голодный — ищет траву далеко
        grassR = fmaxf(grassR, Config::Wolf::GRASS_DESPERATE_RADIUS);
    }
    Vector3 grassPos;
    Config::Grass::Type grassType;
    int grassIdx = world->FindNearestGrass(position, grassR, grassPos, grassType);
    if (grassIdx >= 0) {
        targetPosition = grassPos;
        MoveSwimming(dt, world);

        Vector2 fp = { position.x, position.z };
        Vector2 fg = { grassPos.x, grassPos.z };
        if (Vector2Distance(fp, fg) < 1.0f) {
            world->EatGrass(grassIdx);  // зелёные партиклы уже спавнятся в EatGrass
            hunger = fminf(hunger + 10.0f, 100.0f); // 1/10
            // Эффект травы: голод теперь падает быстрее на 30 сек
            grassEffectTimer = Config::Wolf::GRASS_EFFECT_DURATION;

            if (hunger >= 70.0f) {
                targetPosition = {
                    homePos.x, world->GetHeight(homePos.x, homePos.z), homePos.z
                };
                state = AnimalState::WANDERING;
            }
        }
        return;
    }

    // Совсем ничего нет — возврат домой
    hasPrevPreyPos = false;
    targetPosition = { homePos.x, world->GetHeight(homePos.x, homePos.z), homePos.z };
    state = AnimalState::WANDERING;
}

// =====================================================================
//                       STATE: FIGHTING (конфликт стай)
// =====================================================================

void Wolf::UpdateFight(float dt, World* world) {
    if (!fightTarget || !fightTarget->IsAlive()) {
        // Противник уже мёртв — мы победили
        fightTarget = nullptr;
        fightHealth = Config::Wolf::FIGHT_MAX_HEALTH;
        fightCooldownTimer = Config::Wolf::FIGHT_COOLDOWN;
        state = AnimalState::WANDERING;
        PickNewPatrolTarget(world);
        return;
    }

    Vector3 oppPos = fightTarget->GetPosition();
    float dist = Vector3Distance(position, oppPos);

    targetPosition = oppPos;
    MoveSwimming(dt, world);

    if (dist < Config::Wolf::FIGHT_CONTACT_DIST) {
        // Взаимный урон
        fightHealth -= Config::Wolf::FIGHT_DAMAGE_RATE * dt;
        // Партиклы потасовки (коричнево-серые)
        if (GetRandomValue(0, 100) < 8) {
            Vector3 fp = {
                (position.x + oppPos.x) * 0.5f,
                position.y + 0.5f,
                (position.z + oppPos.z) * 0.5f
            };
            world->SpawnParticles(fp, (Color){140, 110, 80, 255}, 4, false);
        }

        if (fightHealth <= 0.0f) {
            // Мы проиграли. Победитель (fightTarget) обработает merge
            // в своём UpdateFight на следующем тике.
            world->SpawnParticles(position, (Color){150, 60, 60, 255}, 12, false);
            diedInFight = true;
            Die();
        }
    }
}

// =====================================================================
//                              ОСНОВНОЙ UPDATE
// =====================================================================

void Wolf::Update(float deltaTime, World* world) {
    if (!IsAlive()) return;

    // 1. Возраст
    UpdateAge(deltaTime, world);
    if (!IsAlive()) return;

    // 2. Таймеры
    if (pounceCooldownTimer > 0.0f)  pounceCooldownTimer  -= deltaTime;
    if (matingCooldownTimer > 0.0f)  matingCooldownTimer  -= deltaTime;
    if (fightCooldownTimer  > 0.0f)  fightCooldownTimer   -= deltaTime;

    // 3. Голод. После еды травы decay умножается на GRASS_HUNGER_MULT
    float decayRate = Config::Wolf::HUNGER_DECAY_RATE;
    if (grassEffectTimer > 0.0f) {
        grassEffectTimer -= deltaTime;
        decayRate *= Config::Wolf::GRASS_HUNGER_MULT;
    }
    hunger -= decayRate * deltaTime;
    if (hunger < 0.0f) hunger = 0.0f;

    // 4. Смерть от голода
    if (hunger <= 0.0f) {
        starvationTimer += deltaTime;
        if (starvationTimer >= Config::Wolf::STARVATION_LIMIT) {
            world->SpawnParticles(position, (Color){90, 90, 90, 255}, 14, false);
            world->SpawnParticles(position, (Color){60, 50, 40, 255}, 8,  false);
            Die();
            return;
        }
    } else {
        starvationTimer = 0.0f;
    }

    // 5. Встряхивание блокирует всё
    if (shakeTimer > 0.0f) {
        shakeTimer -= deltaTime;
        swimSplashTimer -= deltaTime;
        if (swimSplashTimer <= 0.0f) {
            swimSplashTimer = 0.15f;
            Vector3 sp = { position.x, position.y + 0.6f, position.z };
            world->SpawnParticles(sp, (Color){100, 160, 220, 255}, 5, false);
        }
        return;
    }

    // 6. Скорость
    float baseSpeed = Config::Wolf::SPEED_RUN * CurrentSpeedFactor();
    if (pounceTimer > 0.0f) {
        pounceTimer -= deltaTime;
        speed = Config::Wolf::POUNCE_SPEED * CurrentSpeedFactor();
    } else if (pounceCooldownTimer > 0.0f) {
        speed = baseSpeed * Config::Wolf::REST_SPEED_FACTOR;
    } else {
        speed = baseSpeed;
    }

    // 7. Мёртвая жертва — забываем
    if (targetPrey != nullptr && !targetPrey->IsAlive()) {
        targetPrey = nullptr;
        hasPrevPreyPos = false;
    }

    // 8. Конфликт стай: проверяется ПЕРВЫМ для ADULT/MEDIUM,
    // но не если уже охотимся или паримся
    if (ageStage != Config::Wolf::AgeStage::BABY
        && state != AnimalState::FIGHTING
        && state != AnimalState::HUNGRY
        && !isMating)
    {
        Wolf* enemy = FindEnemyWolfNearby(world);
        if (enemy) {
            state = AnimalState::FIGHTING;
            fightTarget = enemy;
            enemy->state = AnimalState::FIGHTING;
            enemy->fightTarget = this;
        }
    }

    // 9. Размножение — только если не в активных состояниях
    if (state != AnimalState::HUNGRY && state != AnimalState::FIGHTING) {
        TryAttemptMating(deltaTime, world);
        if (mateTarget) {
            // Снап к рельефу
            float th = world->GetHeight(position.x, position.z);
            if (th > world->GetCurrentWaterLevel()) position.y = th;
            return;
        }
    }

    // 10. Принудительный переход в охоту при сильном голоде (< 50 = 5/10),
    // НО только если еда есть в радиусе. Иначе продолжаем патруль.
    if (hunger < Config::Wolf::HUNGER_HUNT_TRIGGER
        && state != AnimalState::HUNGRY
        && state != AnimalState::FIGHTING
        && !isMating)
    {
        if (FindNearestCarcassInRadius(world, CurrentHuntingRadius())
            || FindNearestSheepInRadius(world, CurrentHuntingRadius()))
        {
            state = AnimalState::HUNGRY;
        }
    }

    // 11. Пассивное обнаружение во время патруля:
    // — На ТРУП реагируют всегда (легкая добыча)
    // — На ОВЦУ только если голодны (< HUNGER_HUNT_TRIGGER)
    if ((state == AnimalState::IDLE || state == AnimalState::WANDERING))
    {
        // Труп — всегда (даже сытый волк не упустит готовую еду)
        if (FindNearestCarcassInRadius(world, CurrentHuntingRadius())) {
            state = AnimalState::HUNGRY;
        }
        // Овцы — только если хочется есть
        else if (hunger < Config::Wolf::HUNGER_HUNT_TRIGGER) {
            Sheep* found = FindNearestSheepInRadius(world, CurrentHuntingRadius());
            if (found) {
                state = AnimalState::HUNGRY;
                targetPrey = found;
            }
        }
    }

    // 12. State machine
    switch (state) {
        case AnimalState::IDLE:     UpdateIdle(deltaTime, world);   break;
        case AnimalState::WANDERING:UpdateWander(deltaTime, world); break;
        case AnimalState::HUNGRY:   UpdateHunt(deltaTime, world);   break;
        case AnimalState::FIGHTING: UpdateFight(deltaTime, world);  break;
        case AnimalState::FLEEING:  state = AnimalState::WANDERING; break;
    }

    // 13. Снап к рельефу и удержание в границах карты
    {
        const int halfMap = Config::World::MAP_SIZE / 2;
        const float minB = -(float)halfMap + 2.0f;
        const float maxB =  (float)halfMap - 2.0f;
        position.x = Clamp(position.x, minB, maxB);
        position.z = Clamp(position.z, minB, maxB);
    }
    float th = world->GetHeight(position.x, position.z);
    if (th > world->GetCurrentWaterLevel()) position.y = th;
}

// =====================================================================
//                              ОТРИСОВКА
// =====================================================================

void Wolf::Draw() {
    Vector3 drawPos = position;

    // Прыжок
    if (pounceTimer > 0.0f) {
        float t = 1.0f - (pounceTimer / Config::Wolf::POUNCE_DURATION);
        drawPos.y += sinf(t * PI) * 0.8f;
    }

    // Тряска
    if (shakeTimer > 0.0f) {
        float t = shakeTimer / Config::Wolf::SHAKE_DURATION;
        float amp = Config::Wolf::SHAKE_AMPLITUDE;
        drawPos.x += sinf(t * 60.0f) * amp;
        drawPos.z += cosf(t * 55.0f) * amp * 0.8f;
        drawPos.y += fabsf(sinf(t * 30.0f)) * amp * 0.6f;
    }

    // Бой: лёгкое трясение от ударов
    if (state == AnimalState::FIGHTING) {
        drawPos.x += sinf(GetTime() * 25.0f) * 0.08f;
    }

    float size = 1.4f * CurrentSizeFactor();

    Color body = DARKGRAY;
    if (ageStage == Config::Wolf::AgeStage::ADULT && isLeader)
        body = (Color){55, 50, 55, 255};
    else if (ageStage == Config::Wolf::AgeStage::BABY)
        body = (Color){120, 110, 110, 255};

    DrawCube(drawPos, size, size, size, body);

    float eyeOff = 0.3f * CurrentSizeFactor();
    float eyeSize = 0.2f * CurrentSizeFactor();
    if (ageStage == Config::Wolf::AgeStage::ADULT && isLeader) eyeSize *= 1.3f;

    Vector3 eye1 = { drawPos.x + size * 0.5f, drawPos.y + eyeOff, drawPos.z - eyeOff };
    Vector3 eye2 = { drawPos.x + size * 0.5f, drawPos.y + eyeOff, drawPos.z + eyeOff };
    DrawCube(eye1, eyeSize, eyeSize, eyeSize, RED);
    DrawCube(eye2, eyeSize, eyeSize, eyeSize, RED);
}