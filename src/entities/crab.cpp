#pragma warning(disable: 4576)
#include "crab.hpp"
#include "carcass.hpp"
#include "../core/world.hpp"
#include <raymath.h>
#include <rlgl.h>

// Свой Move для крабов: позволяет ходить даже когда новая клетка
// чуть ниже уровня воды (краб всё равно живёт на полосе пляжа).
// Базовый Animal::MoveTowardsTarget блокирует движение через воду
// и крабы при приливе зависают.
static bool CrabMove(Vector3& position, Vector3 target, float speed,
                     float deltaTime, World* world, float& facingAngle) {
    Vector3 dir = { target.x - position.x, 0.0f, target.z - position.z };
    float dist = Vector3Length(dir);
    if (dist < 0.05f) return false;

    facingAngle = atan2f(dir.x, dir.z) * RAD2DEG;

    Vector3 nd = Vector3Normalize(dir);
    float step = speed * deltaTime;
    if (step > dist) step = dist;

    const int halfMap = Config::World::MAP_SIZE / 2;
    float nx = Clamp(position.x + nd.x * step, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
    float nz = Clamp(position.z + nd.z * step, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);

    // Крабам разрешён мелководный пляж: блокируем только глубокую воду
    float h = world->GetHeight(nx, nz);
    float waterLvl = world->GetCurrentWaterLevel();
    if (h > waterLvl - Config::Crab::WATER_TOLERANCE) {
        position.x = nx;
        position.z = nz;
        return true;
    }
    return false;
}
 
// ─────────────────────────────────────────────────────────────────────────────
Crab::Crab(Vector3 startPosition) : Animal(startPosition) {
    speed    = Config::Crab::SPEED_WALK;
    hunger   = Config::Crab::HUNGER_MAX;
    state    = AnimalState::WANDERING;
    targetPosition = startPosition;
 
    stateTimer = (float)GetRandomValue(5, 15) / 10.0f;
 
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
 
bool Crab::IsOnBeach(float height, World* world) const {
    // Полоса пляжа: от чуть ниже текущего уровня воды до 3.5 блока выше.
    // Двигается вместе с приливом — при подъёме воды "пляж" сдвигается выше.
    float waterLvl = world->GetCurrentWaterLevel();
    return (height >= waterLvl - 0.3f && height <= waterLvl + 3.5f);
}
 
void Crab::PickNewBeachTarget(World* world) {
    const int halfMap = Config::World::MAP_SIZE / 2;

    // С шансом MIGRATION_CHANCE краб отправляется в дальнее путешествие
    // по пляжу — это позволяет крабам ходить по ВСЕМУ побережью карты,
    // а не топтаться в одном месте.
    bool migrate = ((float)GetRandomValue(0, 9999) / 10000.0f)
                   < Config::Crab::MIGRATION_CHANCE;
    float maxRadius = migrate ? Config::Crab::MIGRATION_RADIUS
                              : Config::Crab::WANDER_RADIUS;

    // Больше попыток (50) — выше шанс найти валидную пляжную точку
    for (int attempt = 0; attempt < 50; ++attempt) {
        float angle  = (float)GetRandomValue(0, 360) * DEG2RAD;
        float radius = (float)GetRandomValue(50, (int)(maxRadius * 10)) / 10.0f;

        float tx = position.x + cosf(angle) * radius;
        float tz = position.z + sinf(angle) * radius;
        tx = Clamp(tx, -(float)halfMap + 5.0f, (float)halfMap - 5.0f);
        tz = Clamp(tz, -(float)halfMap + 5.0f, (float)halfMap - 5.0f);

        float ty = world->GetHeight(tx, tz);
        if (IsOnBeach(ty, world)) {
            targetPosition = { tx, ty, tz };
            return;
        }
    }

    // Фолбэк: остаться на месте ненадолго
    targetPosition = position;
    state = AnimalState::IDLE;
    stateTimer = (float)GetRandomValue(5, 12) / 10.0f;
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

    // ── 1a. ГОЛОДНАЯ СМЕРТЬ ──────────────────────────────────────────────────
    // Если hunger=0 дольше 20 сек, краб умирает (тёмно-оранжевые частицы).
    if (hunger <= 0.0f) {
        starvationTimer += deltaTime;
        if (starvationTimer >= Config::Crab::STARVATION_LIMIT) {
            world->SpawnParticles(position, (Color){160, 80, 30, 255}, 6, false);
            Die();
            return;
        }
    } else {
        starvationTimer = 0.0f;
    }

    // ── 2. ВОЗРАСТ ───────────────────────────────────────────────────────────
    ageTimer += deltaTime;
    if (ageTimer >= lifespan) {
        world->SpawnParticles(position, (Color){200, 100, 50, 255}, 8, false);
        Die();
        return;
    }

    // ── 2a. ПЕРЕНАСЕЛЕНИЕ → БОЛЕЗНЬ ──────────────────────────────────────────
    // Раз в 5 сек считаем соседей в радиусе 15. Если > 6 → шанс 15% умереть.
    // Чем плотнее — тем выше шанс (линейный рост).
    crowdCheckTimer -= deltaTime;
    if (crowdCheckTimer <= 0.0f) {
        crowdCheckTimer = Config::Crab::CROWD_CHECK_INTERVAL;
        int neighbors = 0;
        for (const auto& entity : world->GetEntities()) {
            if (!entity->IsAlive() || entity.get() == this) continue;
            Crab* other = dynamic_cast<Crab*>(entity.get());
            if (!other) continue;
            if (Vector3Distance(position, other->GetPosition()) < Config::Crab::CROWD_RADIUS) {
                ++neighbors;
            }
        }
        if (neighbors > Config::Crab::CROWD_THRESHOLD) {
            // Шанс растёт с числом соседей: base * (neighbors / threshold)
            float chance = Config::Crab::CROWD_DEATH_CHANCE
                         * ((float)neighbors / Config::Crab::CROWD_THRESHOLD);
            float roll = (float)GetRandomValue(0, 9999) / 10000.0f;
            if (roll < chance) {
                // "Болезнь" — зелёные частицы
                world->SpawnParticles(position, (Color){80, 140, 60, 255}, 10, false);
                Die();
                return;
            }
        }
    }

    // ── 2b. УТОПЛЕНИЕ ────────────────────────────────────────────────────────
    // Краб, оказавшийся ниже уровня воды, быстро тонет.
    {
        float waterLvl = world->GetCurrentWaterLevel();
        if (position.y < waterLvl - 0.3f) {
            health -= Config::Crab::DROWN_DAMAGE * deltaTime;
            if (health <= 0.0f) {
                world->SpawnParticles(position, (Color){60, 120, 200, 255}, 8, false);
                Die();
                return;
            }
        } else {
            // Восстановление HP на суше
            if (health < 100.0f) health = fminf(health + 10.0f * deltaTime, 100.0f);
        }
    }
 
    // ── 3. ТАЙМЕРЫ ───────────────────────────────────────────────────────────
    if (matingCooldownTimer > 0.0f) matingCooldownTimer -= deltaTime;
    stateTimer -= deltaTime;
 
    // Если целевой труп уже съеден/умер — сбрасываем
    if (targetCarcass && (!targetCarcass->IsAlive() || !targetCarcass->HasFood())) {
        targetCarcass = nullptr;
        if (state == AnimalState::HUNGRY) {
            state      = AnimalState::WANDERING;
            stateTimer = (float)GetRandomValue(5, 15) / 10.0f;
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
 
    // ── 5. СТРАХОВКА ПЛЯЖА (раз в 3 секунды, индивидуальный таймер) ─────────
    beachCheckTimer -= deltaTime;
    if (beachCheckTimer <= 0.0f) {
        beachCheckTimer = 3.0f;
        float currentHeight = world->GetHeight(position.x, position.z);
        if (!IsOnBeach(currentHeight, world) && state != AnimalState::IDLE && !isMating
            && state != AnimalState::HUNGRY) {
            PickNewBeachTarget(world);
        }
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
                CrabMove(position, targetPosition,
                         Config::Crab::SPEED_WALK, deltaTime, world, facingAngle);
 
                float dist = Vector3Distance(position, mateTarget->GetPosition());
                if (dist <= Config::Crab::MATING_APPROACH_DIST) {
                    // Оба рядом — считаем прогресс
                    matingProgressTimer += deltaTime;
                    if (matingProgressTimer >= Config::Crab::MATING_PROCESS_TIME) {
                        // ── РОЖДЕНИЕ нескольких детёнышей ─────────────────
                        // Только «инициатор» (тот, кто ещё в isMating) рожает.
                        if (mateTarget->isMating) {
                            int babies = GetRandomValue(
                                Config::Crab::BABIES_MIN,
                                Config::Crab::BABIES_MAX);
                            for (int i = 0; i < babies; ++i) {
                                Vector3 babyPos = {
                                    position.x + (float)GetRandomValue(-30, 30) / 10.0f,
                                    position.y,
                                    position.z + (float)GetRandomValue(-30, 30) / 10.0f
                                };
                                babyPos.y = world->GetHeight(babyPos.x, babyPos.z);
                                world->QueueEntity(std::make_unique<Crab>(babyPos));
                            }
                            // Сердечки + всплеск
                            world->SpawnParticles(position, RED, 8, true);
                            world->SpawnParticles(mateTarget->GetPosition(), RED, 8, true);
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
            CrabMove(position, targetPosition,
                     Config::Crab::SPEED_WALK, deltaTime, world, facingAngle);
 
            float distToTarget = Vector3Distance(position, targetPosition);
            if (distToTarget <= 0.5f || stateTimer <= 0.0f) {
                state      = AnimalState::IDLE;
                stateTimer = (float)GetRandomValue(5, 15) / 10.0f;
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
            CrabMove(position, targetPosition,
                     Config::Crab::SPEED_WALK, deltaTime, world, facingAngle);
 
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
    float scale   = 0.7f + 0.3f * fminf(ageFrac * 3.0f, 1.0f);
 
    // Цвет: краснеет к старости
    unsigned char r = (unsigned char)(178 + ageFrac * 50);
    unsigned char g = (unsigned char)(34  - ageFrac * 20);
    Color bodyColor = { r, g, 34, 255 };
 
    // Поворот в направлении движения
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);
    rlRotatef(facingAngle, 0.0f, 1.0f, 0.0f);

    Vector3 crabSize = { 0.8f * scale, 0.4f * scale, 0.6f * scale };
    DrawCube({0.0f, 0.0f, 0.0f}, crabSize.x, crabSize.y, crabSize.z, bodyColor);
    DrawCubeWires({0.0f, 0.0f, 0.0f}, crabSize.x, crabSize.y, crabSize.z, ORANGE);
 
    // Глаза — теперь смотрят ВПЕРЁД (по локальной оси +Z)
    float eyeH = 0.3f * scale;
    DrawSphere({-0.15f * scale, eyeH, 0.3f * scale}, 0.08f * scale, BLACK);
    DrawSphere({ 0.15f * scale, eyeH, 0.3f * scale}, 0.08f * scale, BLACK);
 
    // Клешни — спереди (по локальной +Z)
    float cy = 0.05f * scale;
    Color clawColor = (state == AnimalState::HUNGRY && targetCarcass != nullptr) ? RED : bodyColor;
    float bob = (state == AnimalState::HUNGRY && targetCarcass != nullptr)
                ? sinf((float)GetTime() * 8.0f) * 0.06f : 0.0f;
    DrawCube({-0.38f * scale, cy + bob, 0.55f * scale},
             0.18f * scale, 0.18f * scale, 0.22f * scale, clawColor);
    DrawCube({ 0.38f * scale, cy - bob, 0.55f * scale},
             0.18f * scale, 0.18f * scale, 0.22f * scale, clawColor);

    rlPopMatrix();
}