#pragma warning(disable: 4576)
#include "sheep.hpp"
#include "wolf.hpp"
#include "plant.hpp"
#include "../core/world.hpp"
#include <raymath.h>
#include <rlgl.h>
#include "../core/constants.hpp"
#include <cmath>
#include <vector>
#include <algorithm>

// Радиус блуждания вокруг центра стада.
// 80 единиц — разумный масштаб для MAP_SIZE=1000.
static constexpr float FLOCK_WANDER_RADIUS = 80.0f;
// Максимальная дистанция от центра стада до «разворота».
static constexpr float FLOCK_MAX_DIST      = 120.0f;

// ─── Вспомогательные методы ───────────────────────────────────────────────

void Sheep::PickNewWanderTarget(World* world) {
    const int halfMap = Config::World::MAP_SIZE / 2;

    for (int attempt = 0; attempt < 10; ++attempt) {
        float angle  = (float)GetRandomValue(0, 360) * DEG2RAD;
        float radius = (float)GetRandomValue(
            (int)(FLOCK_WANDER_RADIUS * 0.3f * 10),
            (int)(FLOCK_WANDER_RADIUS * 10)) / 10.0f;

        float tx = Clamp(flockCenter.x + cosf(angle) * radius,
                         -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        float tz = Clamp(flockCenter.z + sinf(angle) * radius,
                         -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        float ty = world->GetHeight(tx, tz);
        if (ty > world->GetCurrentWaterLevel()) {
            targetPosition = { tx, ty, tz };
            return;
        }
    }
    // Фолбэк: ближняя сухая точка
    for (int attempt = 0; attempt < 10; ++attempt) {
        float tx = Clamp(position.x + (float)GetRandomValue(-80, 80) * 0.5f,
                         -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        float tz = Clamp(position.z + (float)GetRandomValue(-80, 80) * 0.5f,
                         -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        float ty = world->GetHeight(tx, tz);
        if (ty > world->GetCurrentWaterLevel()) {
            targetPosition = { tx, ty, tz };
            return;
        }
    }
}

void Sheep::ForceEscape(World* world) {
    const int halfMap = Config::World::MAP_SIZE / 2;
    for (int i = 0; i < 8; ++i) {
        float angle   = (float)GetRandomValue(0, 360) * DEG2RAD;
        float pushLen = Config::Sheep::BODY_RADIUS * 6.0f;
        float nx = Clamp(position.x + cosf(angle) * pushLen,
                         -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        float nz = Clamp(position.z + sinf(angle) * pushLen,
                         -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        if (world->GetHeight(nx, nz) > world->GetCurrentWaterLevel()) {
            position.x = nx;
            position.z = nz;
            position.y = world->GetHeight(nx, nz);
            break;
        }
    }
    PickNewWanderTarget(world);
    state      = AnimalState::WANDERING;
    stuckCount = 0;
}

// ─── Конструктор ─────────────────────────────────────────────────────────────

Sheep::Sheep(Vector3 startPosition, Config::Sheep::AgeStage startStage)
    : Animal(startPosition)
{
    float runMod          = (float)GetRandomValue(Config::Sheep::RUN_DECREASE,              Config::Sheep::RUN_INCREASE)              / 100.0f;
    float walkMod         = (float)GetRandomValue(Config::Sheep::WALK_DECREASE,             Config::Sheep::WALK_INCREASE)             / 100.0f;
    float maxHungerMod    = (float)GetRandomValue(Config::Sheep::MAX_HUNGER_DECREASE,       Config::Sheep::MAX_HUNGER_INCREASE)       / 100.0f;
    float hungerThreshMod = (float)GetRandomValue(Config::Sheep::HUNGER_THRESHOLD_DECREASE, Config::Sheep::HUNGER_THRESHOLD_INCREASE) / 100.0f;
    float visionMod       = (float)GetRandomValue(Config::Sheep::VISION_DECREASE,           Config::Sheep::VISION_INCREASE)           / 100.0f;

    myWalkSpeed       = Config::Sheep::SPEED_WALK       * walkMod;
    myRunSpeed        = Config::Sheep::SPEED_RUN        * runMod;
    myVisionRadius    = Config::Sheep::VISION_RADIUS    * visionMod;
    myHungerThreshold = Config::Sheep::HUNGER_THRESHOLD * hungerThreshMod;
    myMaxHunger       = Config::Sheep::MAX_HUNGER       * maxHungerMod;

    speed        = myWalkSpeed;
    hunger       = myMaxHunger;
    stateTimer   = (float)GetRandomValue(0, 30) * 0.1f;
    targetHunter = nullptr;
    flockCenter  = startPosition;
    state        = AnimalState::IDLE;
    targetPosition = startPosition;

    ageStage            = startStage;
    ageTimer            = 0.0f;
    matingCooldownTimer = (float)GetRandomValue(10, 40);

    if (ageStage == Config::Sheep::AgeStage::BABY) {
        maxAgeInCurrentStage = (float)GetRandomValue(
            (int)Config::Sheep::TIME_TO_GROW_BABY_MIN,
            (int)Config::Sheep::TIME_TO_GROW_BABY_MAX);
    } else if (ageStage == Config::Sheep::AgeStage::MEDIUM) {
        maxAgeInCurrentStage = (float)GetRandomValue(
            (int)Config::Sheep::TIME_TO_GROW_MEDIUM_MIN,
            (int)Config::Sheep::TIME_TO_GROW_MEDIUM_MAX);
    } else {
        // ADULT: рандомизируем сколько уже «прожила», чтобы не умирали все разом
        maxAgeInCurrentStage = Config::Sheep::TIME_ADULT_LIFESPAN;
        ageTimer = (float)GetRandomValue(0, (int)(Config::Sheep::TIME_ADULT_LIFESPAN * 0.75f));
        hunger   = myMaxHunger * ((float)GetRandomValue(50, 100) / 100.0f);
    }
}

// ─── Update ──────────────────────────────────────────────────────────────────

void Sheep::Update(float deltaTime, World* world) {
    if (!IsAlive()) return;

    // ── 1. ГОЛОД (всегда, даже во время мейтинга) ────────────────────────────
    hunger -= Config::Sheep::HUNGER_DECAY_RATE * deltaTime;
    if (hunger < 0.0f) hunger = 0.0f;

    if (matingCooldownTimer > 0.0f) matingCooldownTimer -= deltaTime;
    if (dodgeCooldownTimer  > 0.0f) dodgeCooldownTimer  -= deltaTime;

    // Динамический flockCenter: овца "вступает" в ближайшее стадо
    flockUpdateTimer -= deltaTime;
    if (flockUpdateTimer <= 0.0f) {
        flockUpdateTimer = 5.0f;
        // Считаем центр массы 5 ближайших овец
        struct Pair { Sheep* s; float d; };
        std::vector<Pair> nearest;
        for (const auto& entity : world->GetEntities()) {
            if (!entity->IsAlive() || entity.get() == this) continue;
            Sheep* s = dynamic_cast<Sheep*>(entity.get());
            if (!s) continue;
            float d = Vector3Distance(position, s->GetPosition());
            if (d < 25.0f) nearest.push_back({s, d});
        }
        // Топ-5 ближайших
        std::sort(nearest.begin(), nearest.end(),
                  [](const Pair& a, const Pair& b){ return a.d < b.d; });
        if ((int)nearest.size() > 5) nearest.resize(5);

        if (nearest.size() >= 3) {
            Vector3 sum = { 0.0f, 0.0f, 0.0f };
            for (auto& p : nearest) {
                sum.x += p.s->GetPosition().x;
                sum.z += p.s->GetPosition().z;
            }
            sum.x /= nearest.size();
            sum.z /= nearest.size();
            sum.y = world->GetHeight(sum.x, sum.z);
            flockCenter = sum;
        }
    }

    // ── 1.5. РЕАКЦИЯ НА ПРИЛИВ ───────────────────────────────────────────────
    // Раз в секунду проверяем, не подбирается ли вода. Если да:
    //   - 92% овец паникуют и бегут на сухую возвышенность
    //   - 8% "замирают" (роковая ошибка) и в итоге тонут
    tideCheckTimer -= deltaTime;
    if (tideCheckTimer <= 0.0f) {
        tideCheckTimer = 1.0f;
        float waterLvl = world->GetCurrentWaterLevel();
        float aboveWater = position.y - waterLvl;
        // Опасно: либо уже почти в воде, либо рядом с поднимающейся водой
        if (aboveWater < Config::Sheep::TIDE_DETECTION_DIST && aboveWater > -0.1f) {
            if (!tideEvalDone) {
                tideEvalDone = true;
                // Однократный жребий: замираем или паникуем
                float roll = (float)GetRandomValue(0, 9999) / 10000.0f;
                if (roll < Config::Sheep::TIDE_FREEZE_CHANCE) {
                    tideFrozen = true;
                }
            }
            if (!tideFrozen) {
                // Паника: ищем самую высокую точку в радиусе 25
                Vector3 bestEscape = position;
                float bestHeight = position.y;
                for (int i = 0; i < 12; ++i) {
                    float a = (float)GetRandomValue(0, 360) * DEG2RAD;
                    float r = (float)GetRandomValue(8, 25);
                    float tx = position.x + cosf(a) * r;
                    float tz = position.z + sinf(a) * r;
                    float ty = world->GetHeight(tx, tz);
                    if (ty > bestHeight && ty > waterLvl + 2.0f) {
                        bestHeight = ty;
                        bestEscape = { tx, ty, tz };
                    }
                }
                if (bestHeight > position.y + 0.5f) {
                    targetPosition = bestEscape;
                    state          = AnimalState::WANDERING;
                    speed          = myRunSpeed; // бежим в панике
                }
            }
        } else if (aboveWater > Config::Sheep::TIDE_DETECTION_DIST + 2.0f) {
            // Угроза миновала
            tideEvalDone = false;
            tideFrozen   = false;
        }
    }

    // Замёрзшая овца тонет на месте — ничего не делает
    if (tideFrozen) {
        if (position.y <= world->GetCurrentWaterLevel()) {
            health -= 25.0f * deltaTime;
            if (health <= 0.0f) {
                Die();
                return;
            }
        }
        // Снап к рельефу и выход (не двигаемся, не финтим)
        position.y = world->GetHeight(position.x, position.z);
        return;
    }

    // ── 2. ВОЗРАСТ ───────────────────────────────────────────────────────────
    ageTimer += deltaTime;
    if (ageTimer >= maxAgeInCurrentStage) {
        ageTimer = 0.0f;
        if (ageStage == Config::Sheep::AgeStage::BABY) {
            ageStage = Config::Sheep::AgeStage::MEDIUM;
            maxAgeInCurrentStage = (float)GetRandomValue(
                (int)Config::Sheep::TIME_TO_GROW_MEDIUM_MIN,
                (int)Config::Sheep::TIME_TO_GROW_MEDIUM_MAX);
        } else if (ageStage == Config::Sheep::AgeStage::MEDIUM) {
            ageStage = Config::Sheep::AgeStage::ADULT;
            maxAgeInCurrentStage = Config::Sheep::TIME_ADULT_LIFESPAN;
        } else {
            // ADULT — смерть от старости
            world->SpawnParticles(position, (Color){180, 180, 180, 255}, 12, false);
            Die();
            return;
        }
    }

    // ── 3. ОБНАРУЖЕНИЕ УГРОЗ ─────────────────────────────────────────────────
    // Сбрасываем каждый тик и ищем заново (OnEntityDying гарантирует что
    // указатель не dangling, но защищаемся ещё и пересчётом).
    targetHunter = nullptr;

    bool wolfNearby = false;
    Wolf* closestWolf = nullptr;
    float closestWolfDist = myVisionRadius;
    for (const auto& entity : world->GetEntities()) {
        if (!entity->IsAlive()) continue;
        Wolf* wolf = dynamic_cast<Wolf*>(entity.get());
        if (!wolf) continue;
        float d = Vector3Distance(position, wolf->GetPosition());
        if (d < closestWolfDist) {
            closestWolfDist = d;
            closestWolf = wolf;
            wolfNearby = true;
        }
    }
    if (wolfNearby) targetHunter = closestWolf;

    // Волк рядом — сбрасываем мейтинг
    if (wolfNearby) {
        isMating             = false;
        matingProgressTimer  = 0.0f;
        if (mateTarget) { mateTarget->SetMateTarget(nullptr); mateTarget->isMating = false; }
        mateTarget = nullptr;
        state      = AnimalState::FLEEING;
    } else if (state == AnimalState::FLEEING) {
        targetHunter     = nullptr;
        state            = AnimalState::IDLE;
        stateTimer       = 1.5f;
        // Сбрасываем застрявшие финт-таймеры
        lockedFleeTimer  = 0.0f;
        dodgeBoostTimer  = 0.0f;
    } else if (hunger < myHungerThreshold) {
        // Голодная — прерываем мейтинг
        if (isMating) {
            isMating = false; matingProgressTimer = 0.0f;
            if (mateTarget) { mateTarget->SetMateTarget(nullptr); mateTarget->isMating = false; }
            mateTarget = nullptr;
        }
        state = AnimalState::HUNGRY;
    }

    // ── 4. МЕЙТИНГ (при сытости, покое, взрослом возрасте) ───────────────────
    if (state != AnimalState::FLEEING && state != AnimalState::HUNGRY) {
        if (CanMate()) {
            // Ищем свободного партнёра. Радиус больше обычного зрения,
            // т.к. стадо рассеивается во время поиска еды
            Sheep* bestMate  = nullptr;
            float  bestDist  = 30.0f;

            for (const auto& entity : world->GetEntities()) {
                Sheep* candidate = dynamic_cast<Sheep*>(entity.get());
                if (!candidate || candidate == this || !candidate->CanMate()) continue;
                float d = Vector3Distance(position, candidate->GetPosition());
                if (d < bestDist) { bestDist = d; bestMate = candidate; }
            }

            if (bestMate) {
                mateTarget             = bestMate;
                isMating               = true;
                bestMate->mateTarget   = this;   // взаимная блокировка
                bestMate->isMating     = true;
            }
        }

        // Идём к партнёру / ждём рядом
        if (mateTarget) {
            // Партнёр уже мёртв или передумал. Проверка IsAlive() безопасна,
            // т.к. OnEntityDying очистил бы указатель если уже удалён.
            if (!mateTarget->IsAlive()) {
                isMating = false; matingProgressTimer = 0.0f;
                mateTarget = nullptr;
            } else if (mateTarget->hunger < Config::Sheep::MATING_HUNGER_THRESHOLD) {
                isMating = false; matingProgressTimer = 0.0f;
                mateTarget->isMating    = false;
                mateTarget->mateTarget  = nullptr;
                mateTarget              = nullptr;
            } else {
                targetPosition = mateTarget->GetPosition();
                float dist     = Vector3Distance(position, targetPosition);
                speed          = myWalkSpeed * 0.7f;

                if (dist > Config::Sheep::MATING_APPROACH_DIST) {
                    MoveTowardsTarget(deltaTime, world);
                    matingProgressTimer = 0.0f;
                } else {
                    // Стоим рядом — таймер любви
                    matingProgressTimer += deltaTime;

                    // Сердечки каждые ~0.5 секунды
                    if (matingProgressTimer > 0.0f &&
                        fmodf(matingProgressTimer, 0.5f) < deltaTime) {
                        world->SpawnParticles(
                            { position.x, position.y + 2.0f, position.z },
                            RED, 3, true);
                    }

                    // Только ОДИН из пары рожает (тот, чей указатель больше)
                    if (matingProgressTimer >= Config::Sheep::MATING_PROCESS_TIME
                        && this > mateTarget)
                    {
                        // ♥ Рождение ягнёнка
                        Vector3 babyPos = {
                            (position.x + mateTarget->GetPosition().x) * 0.5f,
                            position.y,
                            (position.z + mateTarget->GetPosition().z) * 0.5f
                        };
                        auto baby = std::make_unique<Sheep>(babyPos, Config::Sheep::AgeStage::BABY);
                        baby->SetFlockCenter(flockCenter);
                        world->QueueEntity(std::move(baby));   // безопасный spawn

                        // Сброс обоих
                        hunger         -= 30.0f;
                        matingCooldownTimer = Config::Sheep::MATING_COOLDOWN;
                        isMating            = false;
                        matingProgressTimer = 0.0f;

                        mateTarget->hunger              -= 30.0f;
                        mateTarget->matingCooldownTimer  = Config::Sheep::MATING_COOLDOWN;
                        mateTarget->isMating             = false;
                        mateTarget->matingProgressTimer  = 0.0f;
                        mateTarget->mateTarget           = nullptr;
                        mateTarget                       = nullptr;

                        state      = AnimalState::IDLE;
                        stateTimer = 3.0f;
                    }
                }
                // Пока есть партнёр — пропускаем обычный ИИ
                goto post_ai;
            }
        }
    }

    // ── 5. СТЕЙТ-МАШИНА ──────────────────────────────────────────────────────
    {
        switch (state) {

            case AnimalState::FLEEING: {
                speed = myRunSpeed;
                if (!targetHunter) {
                    MoveTowardsTarget(deltaTime, world);
                    break;
                }

                // Скоростной буст сразу после финта — отрывает овцу от волка
                if (dodgeBoostTimer > 0.0f) {
                    dodgeBoostTimer -= deltaTime;
                    speed = myRunSpeed * Config::Sheep::DODGE_SPEED_BOOST;
                }

                // ── 1. БАЗОВОЕ НАПРАВЛЕНИЕ "ОТ ВОЛКА" ────────────────
                Vector3 fromWolf;
                if (lockedFleeTimer > 0.0f) {
                    // Финт ещё активен — держим перпендикулярный курс,
                    // не пересчитывая "от волка" каждый тик
                    lockedFleeTimer -= deltaTime;
                    fromWolf = lockedFleeDir;
                } else {
                    fromWolf = Vector3Subtract(position, targetHunter->GetPosition());
                    fromWolf.y = 0.0f;
                    if (Vector3Length(fromWolf) < 0.01f) {
                        float a = (float)GetRandomValue(0, 360) * DEG2RAD;
                        fromWolf = { cosf(a), 0.0f, sinf(a) };
                    }
                    fromWolf = Vector3Normalize(fromWolf);
                }

                float distToWolf = Vector3Distance(position, targetHunter->GetPosition());

                // ── 2. РАЗБЕГАНИЕ ОТ БЛИЖАЙШЕЙ ОВЦЫ ─────────────────
                // При панике овцы РАЗБЕГАЮТСЯ в разные стороны
                // (а не группируются). Это создаёт хаос и даёт шанс
                // переместиться в чужое стадо.
                if (lockedFleeTimer <= 0.0f) {
                    Vector3 nearestNeighborPos = position;
                    float nearestDist = 99999.0f;
                    bool foundNeighbor = false;
                    for (const auto& entity : world->GetEntities()) {
                        if (!entity->IsAlive() || entity.get() == this) continue;
                        Sheep* neighbor = dynamic_cast<Sheep*>(entity.get());
                        if (!neighbor) continue;
                        float d = Vector3Distance(position, neighbor->GetPosition());
                        if (d < nearestDist && d < 4.0f) {
                            nearestDist = d;
                            nearestNeighborPos = neighbor->GetPosition();
                            foundNeighbor = true;
                        }
                    }
                    if (foundNeighbor && nearestDist > 0.1f) {
                        // Отталкивание от ближайшей овцы — лёгкое смещение
                        Vector3 awayFromNeighbor = {
                            position.x - nearestNeighborPos.x, 0.0f,
                            position.z - nearestNeighborPos.z
                        };
                        awayFromNeighbor = Vector3Normalize(awayFromNeighbor);
                        float w = 0.25f;  // слабее чем основное направление
                        fromWolf.x = fromWolf.x * (1.0f - w) + awayFromNeighbor.x * w;
                        fromWolf.z = fromWolf.z * (1.0f - w) + awayFromNeighbor.z * w;
                        fromWolf = Vector3Normalize(fromWolf);
                    }
                }

                // ── 3. ФИНТ или ЗИГЗАГ ──────────────────────────────
                // Финт триггерится только когда не залочены прошлым финтом
                bool didDodge = false;
                if (lockedFleeTimer <= 0.0f
                    && distToWolf < Config::Sheep::DODGE_RANGE
                    && dodgeCooldownTimer <= 0.0f)
                {
                    Vector3 packCenter = { 0.0f, 0.0f, 0.0f };
                    int packCount = 0;
                    for (const auto& entity : world->GetEntities()) {
                        if (!entity->IsAlive() || entity.get() == targetHunter) continue;
                        Wolf* w = dynamic_cast<Wolf*>(entity.get());
                        if (!w) continue;
                        float wd = Vector3Distance(position, w->GetPosition());
                        if (wd < 15.0f) {
                            packCenter.x += w->GetPosition().x;
                            packCenter.z += w->GetPosition().z;
                            ++packCount;
                        }
                    }

                    float sign;
                    if (packCount > 0) {
                        packCenter.x = packCenter.x / packCount - position.x;
                        packCenter.z = packCenter.z / packCount - position.z;
                        Vector3 perpPlus = { -fromWolf.z, 0.0f, fromWolf.x };
                        float dot = perpPlus.x * packCenter.x + perpPlus.z * packCenter.z;
                        sign = (dot > 0.0f) ? -1.0f : 1.0f;
                    } else {
                        sign = (GetRandomValue(0, 1) == 0) ? -1.0f : 1.0f;
                    }

                    float angle = sign * Config::Sheep::DODGE_ANGLE_DEG * DEG2RAD;
                    float cs = cosf(angle), sn = sinf(angle);
                    Vector3 rotated = {
                        fromWolf.x * cs - fromWolf.z * sn, 0.0f,
                        fromWolf.x * sn + fromWolf.z * cs
                    };
                    fromWolf = rotated;
                    // КЛЮЧЕВОЕ: блокируем направление на DODGE_LOCK_DURATION сек
                    lockedFleeDir   = fromWolf;
                    lockedFleeTimer = Config::Sheep::DODGE_LOCK_DURATION;
                    dodgeBoostTimer = Config::Sheep::DODGE_LOCK_DURATION;
                    dodgeCooldownTimer = Config::Sheep::DODGE_COOLDOWN;
                    zigzagAngle = 0.0f;
                    zigzagTimer = Config::Sheep::ZIGZAG_INTERVAL_MAX;
                    didDodge = true;
                }

                if (!didDodge && lockedFleeTimer <= 0.0f) {
                    zigzagTimer -= deltaTime;
                    if (zigzagTimer <= 0.0f) {
                        float degMin = -Config::Sheep::ZIGZAG_ANGLE_DEG;
                        float degMax =  Config::Sheep::ZIGZAG_ANGLE_DEG;
                        float deg    = (float)GetRandomValue((int)degMin, (int)degMax);
                        zigzagAngle  = deg * DEG2RAD;
                        zigzagTimer  = (float)GetRandomValue(
                            (int)(Config::Sheep::ZIGZAG_INTERVAL_MIN * 100),
                            (int)(Config::Sheep::ZIGZAG_INTERVAL_MAX * 100)) / 100.0f;
                    }
                    float cs = cosf(zigzagAngle), sn = sinf(zigzagAngle);
                    Vector3 rotated = {
                        fromWolf.x * cs - fromWolf.z * sn, 0.0f,
                        fromWolf.x * sn + fromWolf.z * cs
                    };
                    fromWolf = rotated;
                }

                // ── 4. ЦЕЛЕВАЯ ТОЧКА И ДВИЖЕНИЕ ─────────────────────
                int halfMap = Config::World::MAP_SIZE / 2;
                float tx = Clamp(position.x + fromWolf.x * 15.0f,
                                 -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
                float tz = Clamp(position.z + fromWolf.z * 15.0f,
                                 -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
                targetPosition = { tx, world->GetHeight(tx, tz), tz };

                MoveTowardsTarget(deltaTime, world);
                break;
            }

            case AnimalState::HUNGRY: {
                speed = myWalkSpeed;

                // ГЛОБАЛЬНЫЙ поиск ближайшей травы — голодная овца идёт
                // целеустремлённо, как по запаху, даже если еда вне радиуса
                // обычного зрения. Иначе при низкой плотности травы (1200
                // кустов на 1000×1000) овцы не успевают найти еду в радиусе 5.
                Vector3 grassPos;
                Config::Grass::Type grassType;
                int grassIdx = world->FindNearestGrass(position, 9999.0f,
                                                       grassPos, grassType);

                if (grassIdx >= 0) {
                    targetPosition = grassPos;
                    bool moved = MoveTowardsTarget(deltaTime, world);

                    // Если путь заблокирован (вода) — пытаемся обойти
                    // через wander, иначе зависнем у воды
                    if (!moved) {
                        PickNewWanderTarget(world);
                    }

                    // Проверка дистанции в 2D — y-разница на склоне не
                    // должна мешать поеданию травы, стоящей прямо перед носом
                    Vector2 fp = { position.x, position.z };
                    Vector2 fg = { grassPos.x, grassPos.z };
                    float dist2d = Vector2Distance(fp, fg);
                    if (dist2d < Config::Sheep::EAT_RADUIS) {
                        float nutrition = world->EatGrass(grassIdx);
                        hunger = fminf(hunger + nutrition, myMaxHunger);
                        state      = AnimalState::IDLE;
                        stateTimer = Config::Sheep::IDLE_TIME;
                    }
                } else {
                    // Травы на карте больше не осталось — бродим
                    Vector2 fp = { position.x, position.z };
                    Vector2 ft = { targetPosition.x, targetPosition.z };
                    if (Vector2Distance(fp, ft) < Config::Sheep::REACH_TARGET_DIST)
                        PickNewWanderTarget(world);
                    if (!MoveTowardsTarget(deltaTime, world))
                        PickNewWanderTarget(world);
                }
                break;
            }

            case AnimalState::IDLE: {
                stateTimer -= deltaTime;
                if (stateTimer <= 0.0f) {
                    float distToFlock = Vector2Distance(
                        { position.x, position.z },
                        { flockCenter.x, flockCenter.z });

                    if (distToFlock > FLOCK_MAX_DIST) {
                        // Возврат к стаду
                        flockCenter.y  = world->GetHeight(flockCenter.x, flockCenter.z);
                        targetPosition = flockCenter;
                    } else {
                        // Центр стада дрейфует к текущей позиции → стадо мигрирует
                        flockCenter.x = flockCenter.x * 0.85f + position.x * 0.15f;
                        flockCenter.z = flockCenter.z * 0.85f + position.z * 0.15f;
                        PickNewWanderTarget(world);
                    }
                    state = AnimalState::WANDERING;
                }
                break;
            }

            case AnimalState::WANDERING: {
                speed = myWalkSpeed;
                if (!MoveTowardsTarget(deltaTime, world)) {
                    PickNewWanderTarget(world);
                    break;
                }
                Vector2 fp = { position.x, position.z };
                Vector2 ft = { targetPosition.x, targetPosition.z };
                if (Vector2Distance(fp, ft) < Config::Sheep::REACH_TARGET_DIST) {
                    state      = AnimalState::IDLE;
                    stateTimer = Config::Sheep::IDLE_TIME;
                }
                break;
            }
        }
    }

    post_ai:

    // ── 6. РАСТАЛКИВАНИЕ ─────────────────────────────────────────────────────
    {
        const float minDist  = Config::Sheep::BODY_RADIUS * 2.0f;
        int         overlapN = 0;

        for (const auto& entity : world->GetEntities()) {
            if (entity.get() == this || !entity->IsAlive()) continue;
            Sheep* other = dynamic_cast<Sheep*>(entity.get());
            if (!other) continue;
            if (isMating && other == mateTarget) continue;

            float dist = Vector3Distance(position, other->GetPosition());
            if (dist < minDist) {
                Vector3 push;
                if (dist < 0.001f) {
                    float a = (float)GetRandomValue(0, 360) * DEG2RAD;
                    push = { cosf(a), 0.0f, sinf(a) };
                } else {
                    push   = Vector3Subtract(position, other->GetPosition());
                    push.y = 0.0f;
                    push   = Vector3Normalize(push);
                }
                float half = (minDist - dist) * 0.5f;
                position.x += push.x * half;
                position.z += push.z * half;
                ++overlapN;
            }
        }

        if (overlapN >= 2 && state == AnimalState::WANDERING) {
            state      = AnimalState::IDLE;
            stateTimer = (float)GetRandomValue(3, 10) * 0.1f;
        }
    }

    // ── 7. ДЕТЕКТОР ЗАСТРЕВАНИЯ ───────────────────────────────────────────────
    {
        const float STUCK_CHECK_INTERVAL = 1.5f;
        const float STUCK_THRESHOLD      = 0.4f;
        const int   STUCK_LIMIT          = 2;

        stuckCheckTimer -= deltaTime;
        if (stuckCheckTimer <= 0.0f) {
            stuckCheckTimer = STUCK_CHECK_INTERVAL;
            float moved = Vector2Distance({ position.x, position.z },
                                          { posAtLastCheck.x, posAtLastCheck.z });
            if (moved < STUCK_THRESHOLD
                && (state == AnimalState::WANDERING || state == AnimalState::HUNGRY)) {
                if (++stuckCount >= STUCK_LIMIT) ForceEscape(world);
            } else {
                stuckCount = 0;
            }
            posAtLastCheck = position;
        }
    }

    // ── 8. РЕЛЬЕФ + УТОПАНИЕ ─────────────────────────────────────────────────
    position.y = world->GetHeight(position.x, position.z);

    // Утопание с учётом приливов: при подъёме уровня воды
    // прибрежные овцы оказываются в воде и тонут.
    float waterLvl = world->GetCurrentWaterLevel();
    if (position.y <= waterLvl) {
        health -= 20.0f * deltaTime;
        if (health <= 0.0f) Die();
    } else {
        if (health < 100.0f) health = fminf(health + 5.0f * deltaTime, 100.0f);
    }

    // ── 9. НАПРАВЛЕНИЕ ВЗГЛЯДА (поворот модели в Draw) ───────────────────────
    Vector3 movedDelta = {
        position.x - lastPosition.x, 0.0f,
        position.z - lastPosition.z
    };
    float movedDist = Vector3Length(movedDelta);
    // Обновляем угол только если действительно двигались (не дрожание)
    if (movedDist > 0.02f) {
        facingAngle = atan2f(movedDelta.x, movedDelta.z) * RAD2DEG;
    }
    lastPosition = position;
}

// ─── Draw ─────────────────────────────────────────────────────────────────────

void Sheep::Draw() {
    float scale = 1.0f;
    Color bodyColor = WHITE;

    switch (ageStage) {
        case Config::Sheep::AgeStage::BABY:
            scale     = 0.4f;
            bodyColor = WHITE;
            break;
        case Config::Sheep::AgeStage::MEDIUM:
            scale     = 0.75f;
            bodyColor = WHITE;
            break;
        case Config::Sheep::AgeStage::ADULT: {
            scale = 1.2f;
            // Цвет стареет от белого к серому за время жизни
            float t = Clamp(ageTimer / maxAgeInCurrentStage, 0.0f, 1.0f);
            unsigned char g = (unsigned char)(225 - t * 65);
            bodyColor = { g, g, g, 255 };
            break;
        }
    }

    // Поворот в направлении движения — голова смотрит куда бежим
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(facingAngle, 0.0f, 1.0f, 0.0f);

    DrawCube({0.0f, 0.0f, 0.0f}, 1.2f * scale, 1.2f * scale, 1.2f * scale, bodyColor);
    // Голова теперь спереди (по локальной оси +Z)
    DrawCube({0.0f, 0.2f * scale, 0.6f * scale},
             0.4f * scale, 0.4f * scale, 0.4f * scale, BLACK);

    rlPopMatrix();
}