#include "hunter.hpp"
#include "wolf.hpp"
#include "../core/world.hpp"
#include "raymath.h"
#include "rlgl.h" // Нужен для rlPushMatrix / rlRotatef

Hunter::Hunter(Vector3 startPosition) : Entity(startPosition) {
    hutPosition = startPosition;
    targetPosition = startPosition;
    state = HunterState::RESTING;
    targetWolf = nullptr;
    stateTimer = Config::Hunter::REST_DURATION;
    shootCooldown = 0.0f;
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
        Vector3 normDir = Vector3Normalize(direction);
        float step = speed * deltaTime;
        if (step > distance) step = distance;

        float nx = position.x + normDir.x * step;
        float nz = position.z + normDir.z * step;

        if (world->GetHeight(nx, nz) > Config::World::WATER_LEVEL) {
            position.x = nx;
            position.z = nz;
        }
    }
    position.y = world->GetHeight(position.x, position.z);
}

void Hunter::Update(float deltaTime, World* world) {
    if (shootCooldown > 0.0f) shootCooldown -= deltaTime;

    switch (state) {
        case HunterState::RESTING:
            position = hutPosition; 
            stateTimer -= deltaTime;
            if (stateTimer <= 0.0f) {
                state = HunterState::HUNTING;
                stateTimer = Config::Hunter::HUNT_TIMEOUT;
                
                int halfMap = Config::World::MAP_SIZE / 2;
                float rx = (float)GetRandomValue(-halfMap, halfMap);
                float rz = (float)GetRandomValue(-halfMap, halfMap);
                targetPosition = { rx, world->GetHeight(rx, rz), rz };
            }
            break;

        case HunterState::HUNTING:
            stateTimer -= deltaTime;
            MoveTowards(targetPosition, Config::Hunter::SPEED_WALK, deltaTime, world);

            targetWolf = FindNearestWolf(world);
            if (targetWolf) {
                state = HunterState::CHASING;
                break;
            }

            if (Vector3Distance(position, targetPosition) < 1.0f || stateTimer <= 0.0f) {
                state = HunterState::RETURNING;
            }
            break;

        case HunterState::CHASING:
            if (!targetWolf || !targetWolf->IsAlive()) {
                state = HunterState::HUNTING; 
                targetWolf = nullptr;
                break;
            }

            {
                Vector3 wolfPos = targetWolf->GetPosition();
                float dist = Vector3Distance(position, wolfPos);

                if (dist > Config::Hunter::VISION_RADIUS * 1.3f) {
                    state = HunterState::HUNTING;
                    targetWolf = nullptr;
                    break;
                }

                MoveTowards(wolfPos, Config::Hunter::SPEED_RUN, deltaTime, world);

                // --- ЛОГИКА ВЫСТРЕЛА ---
                if (dist <= Config::Hunter::SHOOT_RANGE && shootCooldown <= 0.0f) {
                    targetWolf->Die();
                    
                    // Партиклы попадания по волку (Кровь)
                    world->SpawnParticles(wolfPos, MAROON, 15, false);

                    // Партиклы выстрела из ружья охотника (Вспышка и дым)
                    Vector3 gunMuzzle = position;
                    gunMuzzle.y += 1.1f; // Высота рук охотника
                    Vector3 dirToWolf = Vector3Normalize(Vector3Subtract(wolfPos, position));
                    gunMuzzle.x += dirToWolf.x * 1.2f; // Выносим дуло вперед
                    gunMuzzle.z += dirToWolf.z * 1.2f;

                    world->SpawnParticles(gunMuzzle, ORANGE, 8, false); // Вспышка огня
                    world->SpawnParticles(gunMuzzle, GRAY, 12, false);  // Дым от пороха
                    
                    shootCooldown = Config::Hunter::SHOOT_COOLDOWN;
                    targetWolf = nullptr;
                    state = HunterState::RETURNING; 
                }
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
    rlTranslatef(offset.x, offset.y, offset.z); // Сдвиг к точке крепления
    rlRotatef(rotateX, 1.0f, 0.0f, 0.0f);      // Вращение для ходьбы
    rlRotatef(rotateZ, 0.0f, 0.0f, 1.0f);      // Вращение для покачивания
    DrawCube({0.0f, 0.0f, 0.0f}, size.x, size.y, size.z, color);
    DrawCubeWires({0.0f, 0.0f, 0.0f}, size.x, size.y, size.z, (Color){0, 0, 0, 100}); // Лёгкий контур
    rlPopMatrix();
}

void Hunter::Draw() {
    if (state == HunterState::RESTING) return;

    // --- 1. ВЫЧИСЛЕНИЕ ПАРАМЕТРОВ АНИМАЦИИ (Походка вперевалку) ---
    float time = GetTime(); 
    float speed = 0.0f;     
    float intensity = 0.0f; 

    if (state == HunterState::CHASING) {
        speed = 14.0f; 
        intensity = 1.0f;
    } else if (state == HunterState::HUNTING || state == HunterState::RETURNING) {
        speed = 7.0f; 
        intensity = 0.5f;
    }

    float swingSin = sinf(time * speed) * intensity;
    float swingCos = cosf(time * speed) * intensity;

    float waddleAngleZ = swingSin * 3.0f; 
    float legSwingX = swingSin * 25.0f;   

    // --- 2. ВЫЧИСЛЕНИЕ ПОВОРОТА ТЕЛА В СТОРОНУ ДВИЖЕНИЯ ---
    float angle = 0.0f;
    if (state == HunterState::CHASING && targetWolf) {
        Vector3 wp = targetWolf->GetPosition();
        angle = atan2f(wp.x - position.x, wp.z - position.z) * RAD2DEG;
    } else if (state == HunterState::HUNTING || state == HunterState::RETURNING) {
        angle = atan2f(targetPosition.x - position.x, targetPosition.z - position.z) * RAD2DEG;
    }

    // --- 3. ЦВЕТОВАЯ ПАЛИТРА ПЕРСОНАЖА ---
    Color jeansColor   = (Color){ 105, 160, 222, 255 }; // Светло-голубой деним
    Color tShirtColor  = WHITE;                        // Белая футболка
    Color hairColor    = (Color){ 50, 40, 33, 255 };    // Темно-коричневые волосы
    Color stubbleColor = (Color){ 100, 95, 95, 255 };   // Серая текстура щетины

    // === НАЧАЛО ОТРИСОВКИ МОДЕЛИ ===
    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z); // Глобальная позиция
    rlRotatef(angle, 0.0f, 1.0f, 0.0f);              // Поворот по направлению движения
    rlRotatef(waddleAngleZ, 0.0f, 0.0f, 1.0f);       // Анимация покачивания тела

    // -- Туловище (Белая футболка) --
    DrawAnimatedPart({0.0f, 1.1f, 0.0f}, {0.7f, 0.8f, 0.4f}, tShirtColor, 0.0f, 0.0f);

    // -- Голова (Лицо базового телесного цвета) --
    DrawAnimatedPart({0.0f, 1.7f, 0.0f}, {0.35f, 0.35f, 0.35f}, BEIGE, 0.0f, 0.0f); 

    // -- Щетина (Аккуратная плашка, облегающая нижнюю челюсть) --
    DrawCube({0.0f, 1.59f, 0.02f}, 0.36f, 0.12f, 0.36f, stubbleColor);

    // -- Кудрявые волосы (Стилизованное облако из сфер поверх головы) --
    DrawSphere({0.0f, 1.90f, 0.0f}, 0.16f, hairColor);       // Макушка (центр)
    DrawSphere({-0.13f, 1.87f, 0.04f}, 0.11f, hairColor);    // Левая верхняя сторона
    DrawSphere({0.13f, 1.87f, 0.04f}, 0.11f, hairColor);     // Правая верхняя сторона
    DrawSphere({0.0f, 1.83f, -0.14f}, 0.13f, hairColor);     // Затылок
    DrawSphere({-0.14f, 1.76f, -0.06f}, 0.09f, hairColor);   // Левый висок
    DrawSphere({0.14f, 1.76f, -0.06f}, 0.09f, hairColor);    // Правый висок
    DrawSphere({0.0f, 1.89f, 0.11f}, 0.09f, hairColor);      // Челка (центр)
    DrawSphere({-0.09f, 1.86f, 0.12f}, 0.08f, hairColor);    // Челка (левее)
    DrawSphere({0.10f, 1.86f, 0.12f}, 0.08f, hairColor);     // Челка (правее)

    // -- Оружие (Дробовик в руках) --
    DrawCube((Vector3){0.25f, 1.1f, 0.5f}, 0.08f, 0.08f, 1.3f, BLACK);       // Ствол
    DrawCube((Vector3){0.25f, 1.05f, 0.0f}, 0.1f, 0.15f, 0.4f, DARKBROWN);   // Приклад

    // -- Ноги (Светло-голубые джинсы) --
    // Правая нога (Анимируется по оси X)
    DrawAnimatedPart({-0.2f, 0.8f, 0.0f}, {0.2f, 0.8f, 0.2f}, jeansColor, legSwingX, -waddleAngleZ);
    
    // Левая нога (В противофазе)
    DrawAnimatedPart({ 0.2f, 0.8f, 0.0f}, {0.2f, 0.8f, 0.2f}, jeansColor, -legSwingX, -waddleAngleZ);

    rlPopMatrix(); // Конец изоляции матриц
    // ==========================================
}

void Hunter::Draw2D(Camera camera) {
    if (state == HunterState::RESTING) return; 

    Vector3 tagWorldPos = { position.x, position.y + 2.5f, position.z };

    // --- 1. ОГРАНИЧЕНИЕ ПО ДИСТАНЦИИ ---
    Vector3 camToHunter = Vector3Subtract(tagWorldPos, camera.position);
    float dist = Vector3Length(camToHunter);
    if (dist > 50.0f) return; // Скрываем имя, если охотник дальше 50 метров

    // --- 2. ПРОВЕРКА ОТСЕЧЕНИЯ КАМЕРЫ (ФРУСТУМ) ---
    // Вычисляем вектор взгляда камеры
    Vector3 camForward = Vector3Subtract(camera.target, camera.position);
    camForward = Vector3Normalize(camForward);
    Vector3 camToHunterNorm = Vector3Normalize(camToHunter);
    
    // Если Dot Product меньше 0, значит точка находится за спиной камеры
    if (Vector3DotProduct(camForward, camToHunterNorm) < 0.0f) return; 

    Vector2 screenPos = GetWorldToScreen(tagWorldPos, camera);

    // Временно используем английское имя. (Инструкция по русификации ниже).
    const char* name = "Mr. Zhelezyn"; 
    int fontSize = 20;
    int textWidth = MeasureText(name, fontSize);

    DrawRectangle(screenPos.x - textWidth / 2 - 6, screenPos.y - 10, textWidth + 12, 24, (Color){0, 0, 0, 140});
    DrawText(name, screenPos.x - textWidth / 2, screenPos.y - 7, fontSize, GOLD);
}