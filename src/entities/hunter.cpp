#include "hunter.hpp"
#include "wolf.hpp"
#include "../core/world.hpp"
#include "raymath.h"
#include "rlgl.h" 

Hunter::Hunter(Vector3 startPosition) : Entity(startPosition) {
    hutPosition = startPosition;
    targetPosition = startPosition;
    state = HunterState::RESTING;
    targetWolf = nullptr;
    stateTimer = Config::Hunter::REST_DURATION;
    shootCooldown = 0.0f;
    facingAngle = 0.0f;
    smokeTimer = 0.0f;
    recoilTimer = 0.0f;
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
        facingAngle = atan2f(direction.x, direction.z) * RAD2DEG;

        Vector3 normDir = Vector3Normalize(direction);
        float stepDist = speed * deltaTime;
        if (stepDist > distance) stepDist = distance;

        float dx = normDir.x * stepDist;
        float dz = normDir.z * stepDist;

        bool moved = false;

        // --- Попытка 1: полный шаг ---
        float nx = position.x + dx;
        float nz = position.z + dz;
        if (world->GetHeight(nx, nz) > world->GetCurrentWaterLevel()) {
            position.x = nx;
            position.z = nz;
            moved = true;
        }

        // --- Попытка 2: скольжение по X ---
        if (!moved && fabsf(dx) > 0.001f) {
            nx = position.x + dx;
            if (world->GetHeight(nx, position.z) > world->GetCurrentWaterLevel()) {
                position.x = nx;
                moved = true;
            }
        }

        // --- Попытка 3: скольжение по Z ---
        if (!moved && fabsf(dz) > 0.001f) {
            nz = position.z + dz;
            if (world->GetHeight(position.x, nz) > world->GetCurrentWaterLevel()) {
                position.z = nz;
                moved = true;
            }
        }

        // === ФИЗИКА ДЕРЕВЬЕВ ===
        if (moved && world != nullptr) {
            // Увеличенный радиус 0.6f, чтобы Железин не проваливался в меши деревьев
            position = world->ResolveTreeCollisions(position, 0.6f); 
        }
    }
    
    position.y = world->GetHeight(position.x, position.z);
}

void Hunter::Update(float deltaTime, World* world) {
    if (shootCooldown > 0.0f) shootCooldown -= deltaTime;
    if (recoilTimer > 0.0f) recoilTimer -= deltaTime; // Списываем время прицеливания

    // === 1. РЕФЛЕКСЫ ОХОТНИКА ===
    if (state != HunterState::RESTING) {
        Wolf* nearest = FindNearestWolf(world);
        
        if (nearest) {
            float dist = Vector3Distance(position, nearest->GetPosition());
            
            if (dist <= Config::Hunter::SHOOT_RANGE && shootCooldown <= 0.0f) {
                // ПОВОРОТ К ЦЕЛИ
                Vector3 wolfPos = nearest->GetPosition();
                facingAngle = atan2f(wolfPos.x - position.x, wolfPos.z - position.z) * RAD2DEG;

                nearest->Die();
                
                world->SpawnParticles(wolfPos, MAROON, 15, false);
                
                Vector3 gunMuzzle = position;
                gunMuzzle.y += 1.1f; 
                Vector3 dirToWolf = Vector3Normalize(Vector3Subtract(wolfPos, position));
                gunMuzzle.x += dirToWolf.x * 1.2f; 
                gunMuzzle.z += dirToWolf.z * 1.2f;
                
                world->SpawnParticles(gunMuzzle, ORANGE, 8, false); 
                world->SpawnParticles(gunMuzzle, GRAY, 12, false);  
                
                shootCooldown = Config::Hunter::SHOOT_COOLDOWN;
                
                // Даем Железину застыть в направлении выстрела на 0.6 секунды
                recoilTimer = 0.6f; 
                
                Wolf* nextWolf = FindNearestWolf(world);
                if (nextWolf) {
                    state = HunterState::CHASING; 
                    targetWolf = nextWolf;
                } else {
                    state = HunterState::RETURNING; 
                    targetWolf = nullptr;
                }
                
                return; 
            } 
            else if (state != HunterState::CHASING) {
                state = HunterState::CHASING;
                targetWolf = nearest;
            }
        } 
        else if (state == HunterState::CHASING) {
            state = HunterState::RETURNING;
            targetWolf = nullptr;
        }
    }

    // === 2. ДВИЖЕНИЕ В ЗАВИСИМОСТИ ОТ СОСТОЯНИЯ ===
    // Железин шагает и поворачивается ТОЛЬКО если не "отходит" от выстрела
    if (recoilTimer <= 0.0f) {
        if (state == HunterState::RESTING) {
            position = hutPosition; 
            stateTimer -= deltaTime;
            smokeTimer -= deltaTime;

            if (smokeTimer <= 0.0f) {
                Vector3 chimneyPos = { hutPosition.x, hutPosition.y + 5.5f, hutPosition.z };
                world->SpawnParticles(chimneyPos, GRAY, 1, false);
                smokeTimer = 0.6f; 
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
        }
        else if (state == HunterState::WANDERING) {
            if (position.y <= Config::World::BIOME_THRESHOLD) {
                stateTimer -= deltaTime;
            }

            MoveTowards(targetPosition, Config::Hunter::SPEED_WALK, deltaTime, world);

            if (stateTimer <= 0.0f) {
                state = HunterState::RETURNING;
            }
            else if (Vector3Distance(position, targetPosition) < 1.0f) {
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
        }
        else if (state == HunterState::CHASING) {
            if (targetWolf) {
                MoveTowards(targetWolf->GetPosition(), Config::Hunter::SPEED_RUN, deltaTime, world);
            }
        }
        else if (state == HunterState::RETURNING) {
            MoveTowards(hutPosition, Config::Hunter::SPEED_WALK, deltaTime, world);

            if (Vector3Distance(position, hutPosition) < 0.8f) {
                state = HunterState::RESTING;
                stateTimer = Config::Hunter::REST_DURATION;
            }
        }
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
    if (state == HunterState::RESTING) return;

    float time = GetTime(); 
    float speed = 0.0f;     
    float intensity = 0.0f; 

    if (state == HunterState::CHASING) {
        speed = 14.0f; 
        intensity = 1.0f;
    } else if (state == HunterState::WANDERING || state == HunterState::RETURNING) {
        speed = 7.0f; 
        intensity = 0.5f;
    }

    float swingSin = sinf(time * speed) * intensity;
    float waddleAngleZ = swingSin * 3.0f; 
    float legSwingX = swingSin * 25.0f;   

    Color jeansColor   = (Color){ 105, 160, 222, 255 }; 
    Color tShirtColor  = WHITE;                        
    Color hairColor    = (Color){ 50, 40, 33, 255 };    
    Color stubbleColor = (Color){ 100, 95, 95, 255 };   

    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z); 
    
    // Используем сохраненный угол! Теперь он не сбрасывается в 0, когда Железин останавливается
    rlRotatef(facingAngle, 0.0f, 1.0f, 0.0f);              
    rlRotatef(waddleAngleZ, 0.0f, 0.0f, 1.0f);       

    // Туловище
    DrawAnimatedPart({0.0f, 1.1f, 0.0f}, {0.7f, 0.8f, 0.4f}, tShirtColor, 0.0f, 0.0f);
    // Голова 
    DrawAnimatedPart({0.0f, 1.7f, 0.0f}, {0.35f, 0.35f, 0.35f}, BEIGE, 0.0f, 0.0f); 
    // Щетина
    DrawCube({0.0f, 1.59f, 0.02f}, 0.36f, 0.12f, 0.36f, stubbleColor);

    // Кудрявые волосы 
    DrawSphere({0.0f, 1.90f, 0.0f}, 0.16f, hairColor);       
    DrawSphere({-0.13f, 1.87f, 0.04f}, 0.11f, hairColor);    
    DrawSphere({0.13f, 1.87f, 0.04f}, 0.11f, hairColor);     
    DrawSphere({0.0f, 1.83f, -0.14f}, 0.13f, hairColor);     
    DrawSphere({-0.14f, 1.76f, -0.06f}, 0.09f, hairColor);   
    DrawSphere({0.14f, 1.76f, -0.06f}, 0.09f, hairColor);    
    DrawSphere({0.0f, 1.89f, 0.11f}, 0.09f, hairColor);      
    DrawSphere({-0.09f, 1.86f, 0.12f}, 0.08f, hairColor);    
    DrawSphere({0.10f, 1.86f, 0.12f}, 0.08f, hairColor);     
    DrawSphere({0.1f, 1.90f, 0.3f}, 0.16f, hairColor);
    DrawSphere({0.2f, 1.90f, 0.25f}, 0.16f, hairColor);
    DrawSphere({0.18f, 1.90f, 0.2f}, 0.16f, hairColor);    
    // Оружие
    DrawCube((Vector3){0.4f, 1.1f, 0.5f}, 0.08f, 0.08f, 1.3f, BLACK);       
    DrawCube((Vector3){0.4f, 1.05f, 0.0f}, 0.1f, 0.15f, 0.4f, DARKBROWN);   

    // Ноги
    DrawAnimatedPart({-0.2f, 0.8f, 0.0f}, {0.2f, 0.8f, 0.2f}, jeansColor, legSwingX, -waddleAngleZ);
    DrawAnimatedPart({ 0.2f, 0.8f, 0.0f}, {0.2f, 0.8f, 0.2f}, jeansColor, -legSwingX, -waddleAngleZ);

    rlPopMatrix(); 
}

void Hunter::Draw2D(Camera camera) {
    if (state == HunterState::RESTING) return; 

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