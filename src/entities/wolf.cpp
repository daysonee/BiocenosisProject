#include "wolf.hpp"
#include "sheep.hpp"
#include "../core/world.hpp"
#include "../core/constants.hpp"
#include <raymath.h>
#include <cmath>

// Старый сканер «узкой воды» — оставлен для совместимости интерфейса,
// но больше не используется: волк плавает через любую воду.
static constexpr float WOLF_MAX_WATER_CROSSING = 8.0f;
static constexpr float WOLF_SCAN_STEP = 0.6f;

Wolf::Wolf(Vector3 startPosition,
           Config::Wolf::AgeStage startStage,
           int wolfPackId) : Animal(startPosition) {
    hunger     = 100.0f;
    state      = AnimalState::WANDERING;
    targetPrey = nullptr;
    packId     = wolfPackId;
    ageStage   = startStage;

    // Времена возраста
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

    // Случайный кулдаун размножения на старте, чтобы не плодились мгновенно
    matingCooldownTimer = (float)GetRandomValue(20, 60);

    speed = Config::Wolf::SPEED_RUN * CurrentSpeedFactor();
    targetPosition = startPosition;
}

bool Wolf::IsNarrowWater(float, float, float, float, World*) {
    return true; // не используется
}

float Wolf::CurrentPounceReach() const {
    return (ageStage == Config::Wolf::AgeStage::ADULT)
           ? Config::Wolf::ADULT_POUNCE_REACH
           : Config::Wolf::POUNCE_REACH;
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
        && shakeTimer           <= 0.0f
        && isAlive;
}

void Wolf::TryStartPounce(float distToPrey, World* world) {
    if (pounceTimer > 0.0f) return;
    if (pounceCooldownTimer > 0.0f) return;
    if (distToPrey > Config::Wolf::POUNCE_RANGE) return;
    if (shakeTimer > 0.0f) return;
    // Не прыгаем из воды
    if (world->GetHeight(position.x, position.z) <= Config::World::WATER_LEVEL) return;

    pounceTimer         = Config::Wolf::POUNCE_DURATION;
    pounceCooldownTimer = FullCooldown();

    Vector3 dustPos = { position.x, position.y + 0.1f, position.z };
    world->SpawnParticles(dustPos, (Color){150, 130, 100, 255}, 6, false);
}

void Wolf::UpdateAge(float deltaTime, World* world) {
    ageTimer += deltaTime;
    if (ageTimer < maxAgeInCurrentStage) return;

    if (ageStage == Config::Wolf::AgeStage::BABY) {
        ageStage = Config::Wolf::AgeStage::MEDIUM;
        maxAgeInCurrentStage = (float)GetRandomValue(
            (int)Config::Wolf::TIME_TO_GROW_MEDIUM_MIN,
            (int)Config::Wolf::TIME_TO_GROW_MEDIUM_MAX);
        ageTimer = 0.0f;

        // «Драки прекращаются если волчонок стал среднего возраста»:
        // если у стаи нет вожака — этот новый MEDIUM сразу его наследует
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
        // ADULT — смерть от старости
        diedOfOldAge = true;
        world->SpawnParticles(position, (Color){170, 170, 170, 255}, 14, false);
        Die();
    }
}

void Wolf::MoveSwimming(float deltaTime, World* world) {
    Vector3 direction = {
        targetPosition.x - position.x, 0.0f,
        targetPosition.z - position.z
    };
    float distance = Vector3Length(direction);
    if (distance < 0.1f) {
        position.y = fmaxf(world->GetHeight(position.x, position.z),
                           Config::World::WATER_LEVEL);
        return;
    }

    Vector3 normDir = Vector3Normalize(direction);

    // Где сейчас находимся (по рельефу)
    float terrainHere = world->GetHeight(position.x, position.z);
    bool inWaterNow = (terrainHere <= Config::World::WATER_LEVEL);

    float curSpeed = speed * (inWaterNow ? Config::Wolf::SWIM_SPEED_FACTOR : 1.0f);
    float step = curSpeed * deltaTime;

    float nx = position.x + normDir.x * step;
    float nz = position.z + normDir.z * step;

    position.x = nx;
    position.z = nz;
    position = world->ResolveTreeCollisions(position, 0.4f);

    float terrainNew = world->GetHeight(position.x, position.z);
    if (terrainNew <= Config::World::WATER_LEVEL) {
        position.y = Config::World::WATER_LEVEL + 0.05f; // плывёт по поверхности
        if (!wasInWater) wasInWater = true;

        // Брызги воды каждые ~0.25 сек
        swimSplashTimer -= deltaTime;
        if (swimSplashTimer <= 0.0f) {
            swimSplashTimer = 0.25f;
            world->SpawnParticles(position, (Color){100, 160, 220, 255}, 4, false);
        }
    } else {
        position.y = terrainNew;
        if (wasInWater) {
            // Вышли из воды — встряхиваемся
            shakeTimer = Config::Wolf::SHAKE_DURATION;
            world->SpawnParticles(position, (Color){100, 160, 220, 255}, 16, false);
            wasInWater = false;
        }
    }
}

void Wolf::TryAttemptMating(float deltaTime, World* world) {
    // Поиск партнёра среди ADULT-волков в широком радиусе
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
            mateTarget         = bestMate;
            isMating           = true;
            bestMate->mateTarget = this;
            bestMate->isMating   = true;
        }
    }

    if (mateTarget && mateTarget->IsAlive()) {
        // Сбрасываемся если партнёр заголодал
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

            // Серые сердечки каждые ~0.5 сек
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
                world->QueueEntity(std::move(baby));

                // Сброс обоих
                matingCooldownTimer            = Config::Wolf::MATING_COOLDOWN;
                mateTarget->matingCooldownTimer = Config::Wolf::MATING_COOLDOWN;
                isMating                       = false;
                mateTarget->isMating           = false;
                matingProgressTimer            = 0.0f;
                mateTarget->matingProgressTimer = 0.0f;
                mateTarget->mateTarget         = nullptr;
                mateTarget                     = nullptr;
                state                          = AnimalState::IDLE;
            }
        }
    } else if (mateTarget) {
        // партнёр умер
        isMating = false; matingProgressTimer = 0.0f;
        mateTarget = nullptr;
    }
}

void Wolf::Update(float deltaTime, World* world) {
    if (!IsAlive()) return;

    // ── ВОЗРАСТ ──────────────────────────────────────────────────
    UpdateAge(deltaTime, world);
    if (!IsAlive()) return;

    // ── ТАЙМЕРЫ ──────────────────────────────────────────────────
    if (pounceCooldownTimer > 0.0f) pounceCooldownTimer -= deltaTime;
    if (matingCooldownTimer > 0.0f) matingCooldownTimer -= deltaTime;

    // ── ВСТРЯХИВАНИЕ ─────────────────────────────────────────────
    if (shakeTimer > 0.0f) {
        shakeTimer -= deltaTime;
        // Брызги воды
        swimSplashTimer -= deltaTime;
        if (swimSplashTimer <= 0.0f) {
            swimSplashTimer = 0.15f;
            Vector3 sp = { position.x, position.y + 0.6f, position.z };
            world->SpawnParticles(sp, (Color){100, 160, 220, 255}, 5, false);
        }
        return; // во время встряхивания ничего не делаем
    }

    // Скорость с учётом возраста и прыжка/усталости
    float baseSpeed = Config::Wolf::SPEED_RUN * CurrentSpeedFactor();
    if (pounceTimer > 0.0f) {
        pounceTimer -= deltaTime;
        speed = Config::Wolf::POUNCE_SPEED * CurrentSpeedFactor();
    } else if (pounceCooldownTimer > 0.0f) {
        speed = baseSpeed * Config::Wolf::REST_SPEED_FACTOR;
    } else {
        speed = baseSpeed;
    }

    // ── ГОЛОД ────────────────────────────────────────────────────
    hunger -= 2.0f * deltaTime;
    if (hunger < 0.0f) hunger = 0.0f;

    if (targetPrey != nullptr && !targetPrey->IsAlive()) {
        targetPrey = nullptr;
        hasPrevPreyPos = false;
    }

    if (hunger < 90.0f && state != AnimalState::HUNGRY && !isMating) {
        state = AnimalState::HUNGRY;
    }

    // ── РАЗМНОЖЕНИЕ (только ADULT, не голодные) ──────────────────
    if (state != AnimalState::HUNGRY) {
        TryAttemptMating(deltaTime, world);
        if (mateTarget) return; // уже двигаемся к партнёру
    }

    // ── СТЕЙТ-МАШИНА ─────────────────────────────────────────────
    switch (state) {
        case AnimalState::IDLE: {
            state = AnimalState::WANDERING;
            int halfMap = Config::World::MAP_SIZE / 2;
            for (int i = 0; i < 10; ++i) {
                float rx = (float)GetRandomValue(-halfMap, halfMap);
                float rz = (float)GetRandomValue(-halfMap, halfMap);
                targetPosition = { rx, world->GetHeight(rx, rz), rz };
                break;
            }
            break;
        }

        case AnimalState::WANDERING: {
            MoveSwimming(deltaTime, world);
            Vector2 flatPos    = { position.x, position.z };
            Vector2 flatTarget = { targetPosition.x, targetPosition.z };
            if (Vector2Distance(flatPos, flatTarget) < 0.5f) {
                state = AnimalState::IDLE;
            }
            break;
        }

        case AnimalState::HUNGRY: {
            Sheep* prevTarget = targetPrey;
            float closestDistance = 9999.0f;
            targetPrey = nullptr;
            for (const auto& entity : world->GetEntities()) {
                if (!entity->IsAlive()) continue;
                Sheep* sheep = dynamic_cast<Sheep*>(entity.get());
                if (sheep) {
                    float dist = Vector3Distance(position, sheep->GetPosition());
                    if (dist < closestDistance) {
                        closestDistance = dist;
                        targetPrey = sheep;
                    }
                }
            }

            if (targetPrey != prevTarget) hasPrevPreyPos = false;

            if (targetPrey) {
                Vector3 preyPos = targetPrey->GetPosition();
                Vector3 leadPos = preyPos;
                float leadT = CurrentLeadTime();
                if (hasPrevPreyPos && deltaTime > 0.001f) {
                    Vector3 preyVel = {
                        (preyPos.x - prevPreyPos.x) / deltaTime, 0.0f,
                        (preyPos.z - prevPreyPos.z) / deltaTime
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
                MoveSwimming(deltaTime, world);

                float reach = (pounceTimer > 0.0f)
                              ? CurrentPounceReach()
                              : Config::Wolf::ATTACK_RADIUS;
                // Вожак: гарантированная хватка — широкая всегда
                if (ageStage == Config::Wolf::AgeStage::ADULT) {
                    reach = fmaxf(reach, Config::Wolf::ADULT_POUNCE_REACH);
                }

                float distAfterMove = Vector3Distance(position, targetPrey->GetPosition());
                if (distAfterMove < reach) {
                    Vector3 killPos = targetPrey->GetPosition();
                    killPos.y += 0.5f;
                    world->SpawnParticles(killPos, (Color){200, 30, 30, 255}, 20, false);
                    world->SpawnParticles(killPos, (Color){120, 10, 10, 255}, 10, false);

                    targetPrey->Die();

                    // BABY-волчонок ест 2 овцы подряд, остальные — одну
                    if (ageStage == Config::Wolf::AgeStage::BABY) {
                        babyKillCount++;
                        hunger = fminf(hunger + 50.0f, 100.0f);
                        if (babyKillCount >= Config::Wolf::BABY_KILLS_TO_FULL) {
                            babyKillCount = 0;
                            pounceCooldownTimer = FullCooldown();
                            state = AnimalState::IDLE;
                        } else {
                            // Короткий кулдаун между съеданиями
                            pounceCooldownTimer = Config::Wolf::POUNCE_COOLDOWN * 0.5f;
                            // Сразу ищет следующую жертву на следующем тике
                        }
                    } else {
                        hunger = 100.0f;
                        pounceCooldownTimer = FullCooldown();
                        state = AnimalState::IDLE;
                    }

                    targetPrey = nullptr;
                    hasPrevPreyPos = false;
                    pounceTimer = 0.0f;
                }
            } else {
                hasPrevPreyPos = false;
                state = AnimalState::WANDERING;
            }
            break;
        }

        case AnimalState::FLEEING:
            state = AnimalState::WANDERING;
            break;
    }

    // Снапим к рельефу только если не в воде (MoveSwimming сам управляет y)
    float th = world->GetHeight(position.x, position.z);
    if (th > Config::World::WATER_LEVEL) position.y = th;
}

void Wolf::Draw() {
    Vector3 drawPos = position;

    // Прыжок по дуге
    if (pounceTimer > 0.0f) {
        float t = 1.0f - (pounceTimer / Config::Wolf::POUNCE_DURATION);
        drawPos.y += sinf(t * PI) * 0.8f;
    }

    // Тряска при встряхивании
    if (shakeTimer > 0.0f) {
        float t = shakeTimer / Config::Wolf::SHAKE_DURATION;
        float jitter = sinf(t * 50.0f) * 0.12f;
        drawPos.x += jitter;
        drawPos.z += jitter * 0.7f;
    }

    float size = 1.4f * CurrentSizeFactor();

    // Цвет: вожак темнее, волчонок чуть светлее
    Color body = DARKGRAY;
    if (ageStage == Config::Wolf::AgeStage::ADULT && isLeader) body = (Color){55, 50, 55, 255};
    else if (ageStage == Config::Wolf::AgeStage::BABY)        body = (Color){120, 110, 110, 255};

    DrawCube(drawPos, size, size, size, body);

    // Глаза — у вожака горящие красные побольше
    float eyeOff = 0.3f * CurrentSizeFactor();
    float eyeSize = 0.2f * CurrentSizeFactor();
    if (ageStage == Config::Wolf::AgeStage::ADULT && isLeader) eyeSize *= 1.3f;

    Vector3 eye1 = { drawPos.x + size * 0.5f, drawPos.y + eyeOff, drawPos.z - eyeOff };
    Vector3 eye2 = { drawPos.x + size * 0.5f, drawPos.y + eyeOff, drawPos.z + eyeOff };
    DrawCube(eye1, eyeSize, eyeSize, eyeSize, RED);
    DrawCube(eye2, eyeSize, eyeSize, eyeSize, RED);
}