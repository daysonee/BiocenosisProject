#pragma warning(disable: 4576)
#include "crab.hpp"
#include "carcass.hpp"
#include "../core/world.hpp"
#include <raymath.h>
 
// ─────────────────────────────────────────────────────────────────────────────
Crab::Crab(Vector3 startPosition) : Animal(startPosition) {
    speed    = Config::Crab::SPEED_WALK;
    hunger   = Config::Crab::HUNGER_MAX;
    state    = AnimalState::WANDERING;
    targetPosition = startPosition;
 
    stateTimer = (float)GetRandomValue(10, 30) / 10.0f;
 
    // Рандомная продолжительность жизни
    lifespan = (float)GetRandomValue(
        (int)Config::Crab::LIFESPAN_MIN,
        (int)Config::Crab::LIFESPAN_MAX);
    ageTimer = 0.0f;
 
    // Крабы стартуют с разным возрастом, чтобы не умирали все разом
    ageTimer = (float)GetRandomValue(0, (int)(lifespan * 0.5f));
 
    matingCooldownTimer = (float)GetRandomValue(5, 20);
}
 
// ─── Вспомогательные ─────────────────────────────────────────────────────────
 
bool Crab::IsOnBeach(float height) const {
    return (height >= Config::World::WATER_LEVEL - 0.2f &&
            height <= Config::World::SAND_LEVEL  + 0.5f);
}
 
void Crab::PickNewBeachTarget(World* world) {
    const int halfMap = Config::World::MAP_SIZE / 2;
    const float wanderRadius = Config::Crab::WANDER_RADIUS;
 
    for (int attempt = 0; attempt < 20; ++attempt) {
        float angle  = (float)GetRandomValue(0, 360) * DEG2RAD;
        float radius = (float)GetRandomValue(50, (int)(wanderRadius * 10)) / 10.0f;
 
        float tx = position.x + cosf(angle) * radius;
        float tz = position.z + sinf(angle) * radius;
        tx = Clamp(tx, -(float)halfMap + 5.0f, (float)halfMap - 5.0f);
        tz = Clamp(tz, -(float)halfMap + 5.0f, (float)halfMap - 5.0f);
 
        float ty = world->GetHeight(tx, tz);
        if (IsOnBeach(ty)) {
            targetPosition = { tx, ty, tz };
            return;
        }
    }
 
    // Фолбэк: остаться на месте
    targetPosition = position;
    state = AnimalState::IDLE;
    stateTimer = (float)GetRandomValue(20, 40) / 10.0f;
}
 
// Ищет ближайший живой труп с едой в пределах всей карты.
// Крабы — мусорщики, они чуют падаль издалека.
Carcass* Crab::FindNearestCarcass(World* world) const {
    Carcass* best    = nullptr;
    float    bestDist = 9999.0f;
 
    for (const auto& entity : world->GetEntities()) {
        if (!entity->IsAlive()) continue;
        Carcass* c = dynamic_cast<Carcass*>(entity.get());
        if (!c || !c->HasFood()) continue;
 
        float d = Vector3Distance(position, c->GetPosition());
        if (d < bestDist) {
            bestDist = d;
            best     = c;
        }
    }
    return best;
}
 
// ─── Update ──────────────────────────────────────────────────────────────────
 
void Crab::Update(float deltaTime, World* world) {
    if (!IsAlive()) return;
 
    // ── 1. ГОЛОД ─────────────────────────────────────────────────────────────
    hunger -= Config::Crab::HUNGER_DECAY * deltaTime;
    if (hunger < 0.0f) hunger = 0.0f;
 
    // ── 2. ВОЗРАСТ ───────────────────────────────────────────────────────────
    ageTimer += deltaTime;
    if (ageTimer >= lifespan) {
        // Смерть от старости: оранжевые частицы
        world->SpawnParticles(position, (Color){200, 100, 50, 255}, 8, false);
        Die();
        return;
    }
 
    // ── 3. ТАЙМЕРЫ ───────────────────────────────────────────────────────────
    if (matingCooldownTimer > 0.0f) matingCooldownTimer -= deltaTime;
    stateTimer -= deltaTime;
 
    // Если целевой труп уже съеден/умер — сбрасываем
    if (targetCarcass && (!targetCarcass->IsAlive() || !targetCarcass->HasFood())) {
        targetCarcass = nullptr;
        if (state == AnimalState::HUNGRY) {
            state      = AnimalState::WANDERING;
            stateTimer = (float)GetRandomValue(10, 30) / 10.0f;
            PickNewBeachTarget(world);
        }
    }
 
    // ── 4. ИНИЦИАЦИЯ РАЗМНОЖЕНИЯ ─────────────────────────────────────────────
    // Ищем партнёра, который тоже готов к спариванию
    if (CanMate() && mateTarget == nullptr) {
        for (const auto& entity : world->GetEntities()) {
            if (!entity->IsAlive() || entity.get() == this) continue;
            Crab* other = dynamic_cast<Crab*>(entity.get());
            if (!other || !other->CanMate()) continue;
            float d = Vector3Distance(position, other->GetPosition());
            if (d < 30.0f) {
                // Взаимная пометка
                mateTarget        = other;
                other->SetMateTarget(this);
                isMating          = true;
                other->isMating   = true;
                matingProgressTimer = 0.0f;
                state = AnimalState::WANDERING;   // пойдём навстречу
                break;
            }
        }
    }
 
    // ── 5. СТРАХОВКА ПЛЯЖА ───────────────────────────────────────────────────
    float currentHeight = world->GetHeight(position.x, position.z);
    if (!IsOnBeach(currentHeight) && state != AnimalState::IDLE && !isMating) {
        PickNewBeachTarget(world);
    }
 
    // ── 6. МАШИНА СОСТОЯНИЙ ──────────────────────────────────────────────────
    switch (state) {
 
        // ── IDLE: просто сидим ──────────────────────────────────────────────
        case AnimalState::IDLE: {
            if (stateTimer <= 0.0f) {
                state = AnimalState::WANDERING;
                PickNewBeachTarget(world);
            }
            break;
        }
 
        // ── WANDERING: прогулка по пляжу / подход к партнёру ───────────────
        case AnimalState::WANDERING: {
            if (isMating && mateTarget != nullptr) {
                // Идём к партнёру
                targetPosition = mateTarget->GetPosition();
                MoveTowardsTarget(deltaTime, world);
 
                float dist = Vector3Distance(position, mateTarget->GetPosition());
                if (dist <= Config::Crab::MATING_APPROACH_DIST) {
                    // Оба рядом — считаем прогресс
                    matingProgressTimer += deltaTime;
                    if (matingProgressTimer >= Config::Crab::MATING_PROCESS_TIME) {
                        // ── РОЖДЕНИЕ ─────────────────────────────────────
                        // Только «инициатор» (тот, кто нашёл партнёра первым) спавнит детёныша
                        if (mateTarget->isMating) {   // оба ещё в процессе
                            Vector3 babyPos = {
                                position.x + (float)GetRandomValue(-20, 20) / 10.0f,
                                position.y,
                                position.z + (float)GetRandomValue(-20, 20) / 10.0f
                            };
                            babyPos.y = world->GetHeight(babyPos.x, babyPos.z);
                            world->QueueEntity(std::make_unique<Crab>(babyPos));
 
                            // Сердечки
                            world->SpawnParticles(position, RED, 6, true);
                            world->SpawnParticles(mateTarget->GetPosition(), RED, 6, true);
                        }
 
                        // Сброс состояния у обоих
                        mateTarget->isMating          = false;
                        mateTarget->mateTarget        = nullptr;
                        mateTarget->matingCooldownTimer = Config::Crab::MATING_COOLDOWN;
 
                        isMating           = false;
                        mateTarget         = nullptr;
                        matingCooldownTimer = Config::Crab::MATING_COOLDOWN;
                        matingProgressTimer = 0.0f;
 
                        state      = AnimalState::IDLE;
                        stateTimer = 2.0f;
                    }
                } else {
                    // Ещё далеко — сбрасываем счётчик
                    matingProgressTimer = 0.0f;
                }
                break;
            }
 
            // Обычная прогулка
            MoveTowardsTarget(deltaTime, world);
 
            float distToTarget = Vector3Distance(position, targetPosition);
            if (distToTarget <= 0.5f || stateTimer <= 0.0f) {
                state      = AnimalState::IDLE;
                stateTimer = (float)GetRandomValue(10, 40) / 10.0f;
            }
 
            // Голодный краб переключается на поиск еды
            if (hunger < Config::Crab::HUNGER_EAT_TRIGGER) {
                targetCarcass = FindNearestCarcass(world);
                if (targetCarcass) {
                    state = AnimalState::HUNGRY;
                }
            }
            break;
        }
 
        // ── HUNGRY: идём к трупу и едим ────────────────────────────────────
        case AnimalState::HUNGRY: {
            // Найти труп, если ещё не нашли
            if (targetCarcass == nullptr) {
                targetCarcass = FindNearestCarcass(world);
                if (targetCarcass == nullptr) {
                    // Трупов нет — бродим
                    state = AnimalState::WANDERING;
                    PickNewBeachTarget(world);
                    break;
                }
            }
 
            targetPosition = targetCarcass->GetPosition();
            MoveTowardsTarget(deltaTime, world);
 
            float dist = Vector3Distance(position, targetCarcass->GetPosition());
            if (dist <= Config::Crab::EAT_RADIUS) {
                // Едим: забираем порцию за этот тик
                float taken = targetCarcass->Eat(Config::Crab::EAT_RATE * deltaTime);
                hunger = fminf(hunger + taken, Config::Crab::HUNGER_MAX);
 
                // Частицы поедания: тёмно-коричневые крошки
                if (taken > 0.0f && GetRandomValue(0, 20) == 0) {
                    world->SpawnParticles(
                        targetCarcass->GetPosition(),
                        (Color){100, 70, 40, 255}, 3, false);
                }
 
                // Наелись или труп доели — уходим
                if (hunger >= Config::Crab::HUNGER_MAX - 5.0f || !targetCarcass->HasFood()) {
                    targetCarcass = nullptr;
                    state         = AnimalState::IDLE;
                    stateTimer    = (float)GetRandomValue(10, 30) / 10.0f;
                    PickNewBeachTarget(world);
                }
            }
            break;
        }
 
        default:
            break;
    }
 
    // ── 7. ФИКСАЦИЯ НА ПОВЕРХНОСТИ ───────────────────────────────────────────
    position.y = world->GetHeight(position.x, position.z);
}
 
// ─── Draw ─────────────────────────────────────────────────────────────────────
 
void Crab::Draw() {
    if (!IsAlive()) return;
 
    // Масштаб: молодые крабы (первая треть жизни) немного меньше
    float ageFrac = Clamp(ageTimer / lifespan, 0.0f, 1.0f);
    float scale   = 0.7f + 0.3f * fminf(ageFrac * 3.0f, 1.0f);  // 0.7 → 1.0 за первую треть
 
    // Цвет: краснеет к старости
    unsigned char r = (unsigned char)(178 + ageFrac * 50);
    unsigned char g = (unsigned char)(34  - ageFrac * 20);
    Color bodyColor = { r, g, 34, 255 };
 
    Vector3 crabSize = { 0.8f * scale, 0.4f * scale, 0.6f * scale };
    DrawCube(position, crabSize.x, crabSize.y, crabSize.z, bodyColor);
    DrawCubeWires(position, crabSize.x, crabSize.y, crabSize.z, ORANGE);
 
    // Глаза (на стебельках)
    float eyeH = 0.3f * scale;
    Vector3 eyeLeft  = { position.x + 0.3f * scale, position.y + eyeH, position.z + 0.2f * scale };
    Vector3 eyeRight = { position.x + 0.3f * scale, position.y + eyeH, position.z - 0.2f * scale };
    DrawSphere(eyeLeft,  0.08f * scale, BLACK);
    DrawSphere(eyeRight, 0.08f * scale, BLACK);
 
    // Клешни (два маленьких куба спереди)
    float cx = position.x + 0.55f * scale;
    float cy = position.y + 0.05f * scale;
    DrawCube({ cx, cy, position.z + 0.38f * scale }, 0.22f * scale, 0.18f * scale, 0.18f * scale, bodyColor);
    DrawCube({ cx, cy, position.z - 0.38f * scale }, 0.22f * scale, 0.18f * scale, 0.18f * scale, bodyColor);
 
    // Когда ест — покачивает клешнями (смещение по Y)
    if (state == AnimalState::HUNGRY && targetCarcass != nullptr) {
        float bob = sinf((float)GetTime() * 8.0f) * 0.06f;
        DrawCube({ cx, cy + bob, position.z + 0.38f * scale }, 0.22f * scale, 0.18f * scale, 0.18f * scale, RED);
        DrawCube({ cx, cy - bob, position.z - 0.38f * scale }, 0.22f * scale, 0.18f * scale, 0.18f * scale, RED);
    }
}
 