#pragma warning(disable: 4576)
#include "hunter.hpp"
#include "wolf.hpp"
#include "sheep.hpp"
#include "../core/world.hpp"
#include "raymath.h"
#include "rlgl.h" 

// Положи свои mp3 сюда. Пути считаются от папки, откуда запускается игра.
// Например: resources/sounds/hunter_obnulenie.mp3
static const char* HUNTER_KILL_PHRASE_PATHS[3] = {
    "resources/sounds/hunter_obnulenie.mp3",
    "resources/sounds/hunter_halava.mp3",
    "resources/sounds/hunter_opazdayu.mp3"
};

Hunter::Hunter(Vector3 startPosition) : Entity(startPosition) {
    hutPosition = startPosition;
    targetPosition = startPosition;
    state = HunterState::RESTING;
    targetWolf = nullptr;
    stateTimer = Config::Hunter::REST_DURATION;
    shootCooldown = 0.0f;
    facingAngle = 0.0f;
    smokeTimer = 0.0f;
    abilityTimer = 0.0f;
    abilityCooldown = 0.0f;
    playerControlled = false;
    controlForward = { 0.0f, 0.0f, 1.0f };
    controlRight = { 1.0f, 0.0f, 0.0f };
    cameraShootOrigin = startPosition;
    cameraShootDir = { 0.0f, 0.0f, 1.0f };
    shootRequested = false;

    for (int i = 0; i < 3; ++i) {
        killPhraseSounds[i] = { 0 };
        killPhraseSoundLoaded[i] = false;
    }
    LoadKillPhraseSounds();
}

Hunter::~Hunter() {
    UnloadKillPhraseSounds();
}

void Hunter::LoadKillPhraseSounds() {
    for (int i = 0; i < 3; ++i) {
        if (FileExists(HUNTER_KILL_PHRASE_PATHS[i])) {
            killPhraseSounds[i] = LoadSound(HUNTER_KILL_PHRASE_PATHS[i]);
            killPhraseSoundLoaded[i] = (killPhraseSounds[i].stream.buffer != nullptr);
        }
    }
}

void Hunter::UnloadKillPhraseSounds() {
    for (int i = 0; i < 3; ++i) {
        if (killPhraseSoundLoaded[i]) {
            UnloadSound(killPhraseSounds[i]);
            killPhraseSoundLoaded[i] = false;
        }
    }
}

void Hunter::PlayRandomKillPhrase() {
    int loadedIndexes[3];
    int loadedCount = 0;

    for (int i = 0; i < 3; ++i) {
        if (killPhraseSoundLoaded[i]) {
            loadedIndexes[loadedCount++] = i;
        }
    }

    if (loadedCount == 0) return;

    int randomIndex = loadedIndexes[GetRandomValue(0, loadedCount - 1)];
    PlaySound(killPhraseSounds[randomIndex]);
}


void Hunter::SetPlayerControlled(bool enabled) {
    playerControlled = enabled;
    if (playerControlled) {
        state = HunterState::WANDERING;
        targetWolf = nullptr;
        targetPosition = position;
    }
}

bool Hunter::IsPlayerControlled() const {
    return playerControlled;
}


void Hunter::SetPlayerAim(Vector3 forward, Vector3 right, Vector3 cameraOrigin) {
    if (Vector3Length(forward) > 0.01f) controlForward = Vector3Normalize(forward);
    if (Vector3Length(right) > 0.01f) controlRight = Vector3Normalize(right);
    cameraShootOrigin = cameraOrigin;
    cameraShootDir = controlForward;

    Vector3 flatForward = { controlForward.x, 0.0f, controlForward.z };
    if (Vector3Length(flatForward) > 0.01f) {
        flatForward = Vector3Normalize(flatForward);
        facingAngle = atan2f(flatForward.x, flatForward.z) * RAD2DEG;
    }
}

void Hunter::RequestShoot() {
    shootRequested = true;
}

Entity* Hunter::FindShootTargetByRay(World* world, Vector3 rayOrigin, Vector3 rayDir) {
    if (Vector3Length(rayDir) <= 0.01f) return nullptr;
    rayDir = Vector3Normalize(rayDir);

    Entity* best = nullptr;
    float bestT = Config::Hunter::SHOOT_RANGE * 3.0f;
    const float HIT_RADIUS = 1.6f;

    for (const auto& entity : world->GetEntities()) {
        if (!entity->IsAlive()) continue;

        Entity* target = entity.get();
        bool canShoot = (dynamic_cast<Wolf*>(target) != nullptr) ||
                        (dynamic_cast<Sheep*>(target) != nullptr);
        if (!canShoot) continue;

        Vector3 targetPos = target->GetPosition();
        targetPos.y += 0.8f;
        Vector3 toTarget = Vector3Subtract(targetPos, rayOrigin);
        float t = Vector3DotProduct(toTarget, rayDir);
        if (t < 0.0f || t > bestT) continue;

        Vector3 closestPoint = Vector3Add(rayOrigin, Vector3Scale(rayDir, t));
        float missDist = Vector3Distance(closestPoint, targetPos);
        if (missDist <= HIT_RADIUS) {
            bestT = t;
            best = target;
        }
    }

    return best;
}

void Hunter::ShootEntity(Entity* target, World* world) {
    if (!target || shootCooldown > 0.0f) return;

    Vector3 targetPos = target->GetPosition();
    Vector3 dirToTarget = Vector3Subtract(targetPos, position);
    if (Vector3Length(dirToTarget) > 0.01f) {
        facingAngle = atan2f(dirToTarget.x, dirToTarget.z) * RAD2DEG;
    }

    target->Die();
    PlayRandomKillPhrase();

    Color hitColor = MAROON;
    if (dynamic_cast<Sheep*>(target) != nullptr) hitColor = RED;
    world->SpawnParticles(targetPos, hitColor, 15, false);

    Vector3 gunMuzzle = position;
    gunMuzzle.y += 1.1f;
    dirToTarget = Vector3Normalize(Vector3Subtract(targetPos, position));
    gunMuzzle.x += dirToTarget.x * 1.2f;
    gunMuzzle.z += dirToTarget.z * 1.2f;

    world->SpawnParticles(gunMuzzle, ORANGE, 8, false);
    world->SpawnParticles(gunMuzzle, GRAY, 12, false);

    shootCooldown = Config::Hunter::SHOOT_COOLDOWN;
}

void Hunter::UpdatePlayerControl(float deltaTime, World* world) {
    state = HunterState::WANDERING;
    targetWolf = nullptr;

    Vector3 flatForward = { controlForward.x, 0.0f, controlForward.z };
    Vector3 flatRight = { controlRight.x, 0.0f, controlRight.z };
    if (Vector3Length(flatForward) > 0.01f) flatForward = Vector3Normalize(flatForward);
    if (Vector3Length(flatRight) > 0.01f) flatRight = Vector3Normalize(flatRight);

    Vector3 move = { 0.0f, 0.0f, 0.0f };

    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) move = Vector3Add(move, flatForward);
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) move = Vector3Subtract(move, flatForward);
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move = Vector3Add(move, flatRight);
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) move = Vector3Subtract(move, flatRight);

    if (Vector3Length(move) > 0.01f) {
        move = Vector3Normalize(move);

        float currentSpeed = Config::Hunter::SPEED_WALK;
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
            currentSpeed *= 3.0f;
        }

        Vector3 nextPos = position;
        nextPos.x += move.x * currentSpeed * deltaTime;
        nextPos.z += move.z * currentSpeed * deltaTime;

        const int halfMap = Config::World::MAP_SIZE / 2;
        nextPos.x = Clamp(nextPos.x, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);
        nextPos.z = Clamp(nextPos.z, -(float)halfMap + 2.0f, (float)halfMap - 2.0f);

        if (world->GetHeight(nextPos.x, nextPos.z) > world->GetCurrentWaterLevel()) {
            position.x = nextPos.x;
            position.z = nextPos.z;
        }
        position.y = world->GetHeight(position.x, position.z);
    }

    if (shootRequested && shootCooldown <= 0.0f) {
        ShootEntity(FindShootTargetByRay(world, cameraShootOrigin, cameraShootDir), world);
    }
    shootRequested = false;
}


Wolf* Hunter::FindNearestWolf(World* world) {
    Wolf* nearest = nullptr;
    float minDist = Config::Hunter::VISION_RADIUS;

    for (const auto& entity : world->GetEntities()) {
        if (!entity->IsAlive()) continue;
        
        Wolf* wolf = dynamic_cast<Wolf*>(entity.get());
        if (wolf) {
            float dist = Vector3Distance(position, wolf->GetPosition());
            if (dist < minDist) {
                minDist = dist;
                nearest = wolf;
            }
        }
    }
    return nearest;
}

void Hunter::MoveTowards(Vector3 target, float speed, float deltaTime, World* world) {
    Vector3 direction = { target.x - position.x, 0.0f, target.z - position.z };
    float distance = Vector3Length(direction);
    
    if (distance > 0.1f) {
        // Обновляем угол поворота только если мы двигаемся
        facingAngle = atan2f(direction.x, direction.z) * RAD2DEG;

        Vector3 normDir = Vector3Normalize(direction);
        float step = speed * deltaTime;
        if (step > distance) step = distance;

        float nx = position.x + normDir.x * step;
        float nz = position.z + normDir.z * step;

        if (world->GetHeight(nx, nz) > world->GetCurrentWaterLevel()) {
            position.x = nx;
            position.z = nz;
        }
    }
    position.y = world->GetHeight(position.x, position.z);
}

void Hunter::Update(float deltaTime, World* world) {
    if (shootCooldown > 0.0f) shootCooldown -= deltaTime;
    
    if (abilityCooldown > 0.0f) abilityCooldown -= deltaTime;
    if (abilityTimer > 0.0f) abilityTimer -= deltaTime;

    if (playerControlled) {
        UpdatePlayerControl(deltaTime, world);
        return;
    }

    // === 1. РЕФЛЕКСЫ ОХОТНИКА (Глобальная проверка) ===
    // Если Железин не спит в хижине, он всегда сканирует местность
    if (state != HunterState::RESTING) {
        Wolf* nearest = FindNearestWolf(world);
        
        if (nearest) {
            float dist = Vector3Distance(position, nearest->GetPosition());
            
            // Если волк в зоне поражения и ружье заряжено — стреляем рефлекторно из любого состояния!
            if (dist <= Config::Hunter::SHOOT_RANGE && shootCooldown <= 0.0f) {
                ShootEntity(nearest, world);
                
                // Оцениваем обстановку после выстрела
                Wolf* nextWolf = FindNearestWolf(world);
                if (nextWolf) {
                    state = HunterState::CHASING; // Рядом еще волки - продолжаем бой
                    targetWolf = nextWolf;
                } else {
                    state = HunterState::RETURNING; // Никого нет - идем домой
                    targetWolf = nullptr;
                }
                return; // Прерываем Update, чтобы в этом кадре больше не шагать
            } 
            // Если волк далеко, но мы его видим — бросаем все дела и начинаем погоню
            else if (state != HunterState::CHASING) {
                state = HunterState::CHASING;
                targetWolf = nearest;
            }
        } 
        // Если мы гнались за волком, но он скрылся из виду (убежал)
        else if (state == HunterState::CHASING) {
            state = HunterState::RETURNING;
            targetWolf = nullptr;
        }
    }

    // === 2. ДВИЖЕНИЕ В ЗАВИСИМОСТИ ОТ СОСТОЯНИЯ ===
    switch (state) {
        case HunterState::RESTING:
            position = hutPosition; 
            stateTimer -= deltaTime;
            smokeTimer -= deltaTime;

            if (smokeTimer <= 0.0f) {
                // Труба находится в (hp.x + 1.0, hp.y + 5.35, hp.z) — высота над верхом трубы
                Vector3 chimneyPos = { hutPosition.x + 1.6f, hutPosition.y + 6.6f, hutPosition.z-0.2f };
                world->SpawnParticles(chimneyPos, GRAY, 1, false);
                smokeTimer = 0.5f; 
            }

            if (stateTimer <= 0.0f) {
                state = HunterState::WANDERING;
                stateTimer = Config::Hunter::HUNT_TIMEOUT;
                
                int halfMap = Config::World::MAP_SIZE / 2;
                for (int i = 0; i < 50; i++) {
                    float rx = (float)GetRandomValue(-halfMap, halfMap);
                    float rz = (float)GetRandomValue(-halfMap, halfMap);
                    float ry = world->GetHeight(rx, rz);
                    
                    if (ry > Config::World::SAND_LEVEL && ry <= Config::World::BIOME_THRESHOLD) {
                        targetPosition = { rx, ry, rz };
                        break;
                    }
                }
            }
            break;

        case HunterState::WANDERING:
            if (position.y <= Config::World::BIOME_THRESHOLD) {
                stateTimer -= deltaTime;
            }

            MoveTowards(targetPosition, Config::Hunter::SPEED_WALK, deltaTime, world);

            if (stateTimer <= 0.0f) {
                state = HunterState::RETURNING;
                break;
            }

            if (Vector3Distance(position, targetPosition) < 1.0f) {
                int halfMap = Config::World::MAP_SIZE / 2;
                for (int i = 0; i < 50; i++) {
                    float rx = (float)GetRandomValue(-halfMap, halfMap);
                    float rz = (float)GetRandomValue(-halfMap, halfMap);
                    float ry = world->GetHeight(rx, rz);
                    
                    if (ry > Config::World::SAND_LEVEL && ry <= Config::World::BIOME_THRESHOLD) {
                        targetPosition = { rx, ry, rz };
                        break;
                    }
                }
            }
            break;

        case HunterState::CHASING:
            if (targetWolf) {
                float dist = Vector3Distance(position, targetWolf->GetPosition());
                
                // АКТИВАЦИЯ СПРИНТА: если волк далеко и способность готова
                if (dist > 20.0f && abilityCooldown <= 0.0f && abilityTimer <= 0.0f) {
                    abilityTimer = 5.0f;       // Ускорение на 5 секунд
                    abilityCooldown = 20.0f;   // Кулдаун 20 секунд
                    world->SpawnParticles(position, ORANGE, 25, false); // Взрыв частиц азарта
                }

                // Базовая скорость охотника при погоне — 14.0f
                float currentSpeed = 14.0f; 
                
                // Если спринт активен, разгоняем охотника до ~24.0f
                if (abilityTimer > 0.0f) {
                    currentSpeed *= 1.7f;
                    if (GetRandomValue(0, 2) == 0) {
                        world->SpawnParticles(position, YELLOW, 2, false); // Искры под ногами
                    }
                }

                // Движение к волку
                MoveTowards(targetWolf->GetPosition(), currentSpeed, deltaTime, world);
            }
            break;

        case HunterState::RETURNING:
            MoveTowards(hutPosition, Config::Hunter::SPEED_WALK, deltaTime, world);

            if (Vector3Distance(position, hutPosition) < 0.8f) {
                state = HunterState::RESTING;
                stateTimer = Config::Hunter::REST_DURATION;
            }
            break;
    }
}

void DrawAnimatedPart(Vector3 offset, Vector3 size, Color color, float rotateX, float rotateZ) {
    rlPushMatrix();
    rlTranslatef(offset.x, offset.y, offset.z); 
    rlRotatef(rotateX, 1.0f, 0.0f, 0.0f);      
    rlRotatef(rotateZ, 0.0f, 0.0f, 1.0f);      
    DrawCube({0.0f, 0.0f, 0.0f}, size.x, size.y, size.z, color);
    DrawCubeWires({0.0f, 0.0f, 0.0f}, size.x, size.y, size.z, (Color){0, 0, 0, 100}); 
    rlPopMatrix();
}

void Hunter::Draw() {
    if (state == HunterState::RESTING && !playerControlled) return;

    float time = GetTime(); 
    float speed = 0.0f;     
    float intensity = 0.0f; 

    if (playerControlled && (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT))) {
        speed = 14.0f;
        intensity = 0.8f;
    } else if (state == HunterState::CHASING) {
        // Задаем базовую скорость анимации ног при погоне
        speed = 18.0f; 
        
        // Если способность сейчас активна, заставляем ноги двигаться еще быстрее!
        if (abilityTimer > 0.0f) {
            speed *= 1.3f; 
        }
        intensity = 1.0f;
    } else if (state == HunterState::WANDERING || state == HunterState::RETURNING) {
        speed = 11.0f; 
        intensity = 0.5f;
    }

    // Физическое перемещение MoveTowards отсюда удалено, так как оно уже работает в Update()

    float swingSin = sinf(time * speed) * intensity;
    float waddleAngleZ = swingSin * 3.0f; 
    float legSwingX = swingSin * 25.0f;   

    Color jeansColor   = (Color){ 105, 160, 222, 255 }; 
    Color tShirtColor  = WHITE;                        
    Color hairColor    = (Color){ 50, 40, 33, 255 };    
    Color stubbleColor = (Color){ 100, 95, 95, 255 };   

    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z); 
    
    // Используем сохраненный угол, чтобы охотник не сбрасывал поворот при остановке
    rlRotatef(facingAngle, 0.0f, 1.0f, 0.0f);              
    rlRotatef(waddleAngleZ, 0.0f, 0.0f, 1.0f);       

    // Туловище
    DrawAnimatedPart({0.0f, 1.1f, 0.0f}, {0.7f, 0.8f, 0.4f}, tShirtColor, 0.0f, 0.0f);
    // Голова 
    DrawAnimatedPart({0.0f, 1.7f, 0.0f}, {0.35f, 0.35f, 0.35f}, BEIGE, 0.0f, 0.0f); 
    // Щетина
    DrawCube({0.0f, 1.59f, 0.02f}, 0.36f, 0.12f, 0.36f, stubbleColor);

    // Кудрявые волосы
    DrawSphere({0.0f, 1.90f, 0.0f}, 0.18f, hairColor);       // макушка
    DrawSphere({-0.13f, 1.87f, 0.04f}, 0.13f, hairColor);    // верх-лев
    DrawSphere({0.13f, 1.87f, 0.04f}, 0.13f, hairColor);     // верх-прав
    DrawSphere({0.0f, 1.83f, -0.14f}, 0.14f, hairColor);     // затылок
    DrawSphere({-0.18f, 1.78f, 0.0f}, 0.14f, hairColor);     // левый бок
    DrawSphere({0.18f, 1.78f, 0.0f}, 0.14f, hairColor);      // правый бок
    DrawSphere({-0.18f, 1.78f, -0.1f}, 0.12f, hairColor);    // левый бок-зад
    DrawSphere({0.18f, 1.78f, -0.1f}, 0.12f, hairColor);     // правый бок-зад
    DrawSphere({-0.16f, 1.72f, 0.05f}, 0.10f, hairColor);    // нижний-лев
    DrawSphere({0.16f, 1.72f, 0.05f}, 0.10f, hairColor);     // нижний-прав
    // Чёлка
    DrawSphere({0.0f, 1.89f, 0.13f}, 0.10f, hairColor);
    DrawSphere({-0.09f, 1.86f, 0.14f}, 0.09f, hairColor);
    DrawSphere({0.10f, 1.86f, 0.14f}, 0.09f, hairColor);

    // Оружие
    DrawCube((Vector3){0.35f, 1.1f, 0.5f}, 0.08f, 0.08f, 1.3f, BLACK);       
    DrawCube((Vector3){0.35f, 1.05f, 0.0f}, 0.1f, 0.15f, 0.4f, DARKBROWN);   

    // Ноги
    DrawAnimatedPart({-0.2f, 0.8f, 0.0f}, {0.2f, 0.8f, 0.2f}, jeansColor, legSwingX, -waddleAngleZ);
    DrawAnimatedPart({ 0.2f, 0.8f, 0.0f}, {0.2f, 0.8f, 0.2f}, jeansColor, -legSwingX, -waddleAngleZ);

    rlPopMatrix(); 
}

void Hunter::Draw2D(Camera camera) {
    if (state == HunterState::RESTING && !playerControlled) return; 

    Vector3 tagWorldPos = { position.x, position.y + 2.5f, position.z };
    Vector3 camToHunter = Vector3Subtract(tagWorldPos, camera.position);
    
    if (Vector3Length(camToHunter) > 50.0f) return; 

    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    if (Vector3DotProduct(camForward, Vector3Normalize(camToHunter)) < 0.0f) return; 

    Vector2 screenPos = GetWorldToScreen(tagWorldPos, camera);
    const char* name = "Mr. Zhelezyn"; 
    int fontSize = 20;
    int textWidth = MeasureText(name, fontSize);

    DrawRectangle(screenPos.x - textWidth / 2 - 6, screenPos.y - 10, textWidth + 12, 24, (Color){0, 0, 0, 140});
    DrawText(name, screenPos.x - textWidth / 2, screenPos.y - 7, fontSize, GOLD);
}