#include "raylib.h"
#include "raymath.h"
#include "core/world.hpp"
#include "entities/plant.hpp"
#include "entities/animal.hpp"
#include "entities/sheep.hpp"
#include "entities/wolf.hpp"
#include <memory>
#include <cmath>

// СПАВН ОВЕЦ
void SpawnSheepSmartAndLogical(World& world) {
    int halfMap = Config::World::MAP_SIZE / 2;
    int totalSheepToSpawn = Config::Sheep::INITIAL_COUNT;
    int spawnedCount = 0;

    // Параметры распределения стада
    int minFlockSize = 3;
    int maxFlockSize = 6;
    int maxFlockAttempts = 300; // Быстрый лимит на поиск идеальных прибрежных зон

    // Этап 1: Умный спавн стадами у воды в безопасных зонах
    for (int attempt = 0; attempt < maxFlockAttempts && spawnedCount < totalSheepToSpawn; attempt++) {
        
        // 1. Ищем потенциальный центр для нового стада
        float cx = (float)GetRandomValue(-halfMap, halfMap);
        float cz = (float)GetRandomValue(-halfMap, halfMap);
        float cy = world.GetHeight(cx, cz);

        // Проверяем биом (подходит ли высота под прибрежную зону)
        if (cy <= Config::World::WATER_LEVEL || cy > Config::World::SAND_LEVEL + 0.3f) {
            continue; 
        }

        // 2. Умная проверка: Безопасность стада. Нет ли рядом волков?
        bool isZoneSafe = true;
        for (const auto& entity : world.GetEntities()) {
            // Динамически проверяем, является ли сущность волком
            if (dynamic_cast<Wolf*>(entity.get())) {
                float dist = Vector3Distance((Vector3){cx, cy, cz}, entity->GetPosition());
                if (dist < Config::Sheep::SAFE_ZONE_FROM_WOLVES) {
                    isZoneSafe = false;
                    break; // Зона опасна, выходим из проверки сущностей
                }
            }
        }

        if (!isZoneSafe) continue; // Нашли берег, но там караулит волк — ищем другое место!

        // 3. Зона идеальна! Формируем размер этого конкретного семейства
        int currentFlockSize = GetRandomValue(minFlockSize, maxFlockSize);

        for (int i = 0; i < currentFlockSize; i++) {
            if (spawnedCount >= totalSheepToSpawn) break;

            // Используем полярные координаты для органического кругового распределения
            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            // Случайный радиус от 1.0 до SPAWN_FLOCK_RADIUS
            float radius = (float)GetRandomValue(10, (int)(Config::Sheep::SPAWN_FLOCK_RADIUS * 10)) / 10.0f;

            float sx = cx + cosf(angle) * radius;
            float sz = cz + sinf(angle) * radius;

            // Защита от выхода за границы карты
            if (sx < -halfMap || sx > halfMap || sz < -halfMap || sz > halfMap) continue;

            float sy = world.GetHeight(sx, sz);

            // Финальная проверка: конкретная овечка из стада не должна провалиться под воду
            if (sy > Config::World::WATER_LEVEL) {
                world.AddEntity(std::make_unique<Sheep>((Vector3){ sx, sy, sz }));
                spawnedCount++;
            }
        }
    }

    // Этап 2: Скоростной Фолбэк (Гарантия производительности)
    // Если из-за генерации карты идеальных пляжей не хватило, мгновенно доспавниваем 
    // остаток на любых сухих участках (например, лугах), чтобы игра не зависла.
    if (spawnedCount < totalSheepToSpawn) {
        int fallbackAttempts = 1000;
        while (spawnedCount < totalSheepToSpawn && fallbackAttempts > 0) {
            fallbackAttempts--;
            float rx = (float)GetRandomValue(-halfMap, halfMap);
            float rz = (float)GetRandomValue(-halfMap, halfMap);
            float ry = world.GetHeight(rx, rz);

            if (ry > Config::World::WATER_LEVEL) {
                world.AddEntity(std::make_unique<Sheep>((Vector3){ rx, ry, rz }));
                spawnedCount++;
            }
        }
    }
}

int main(){

    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_FULLSCREEN_MODE);

    int currentMonitor = GetCurrentMonitor();
    int screenWidth = GetMonitorWidth(currentMonitor);
    int screenHeight = GetMonitorHeight(currentMonitor);


    InitWindow(screenWidth, screenHeight, "Simulator");

    DisableCursor();

    SetTargetFPS(60);

    World myWorld;

    myWorld.AddEntity(std::make_unique<Plant>((Vector3){0.0f, 0.0f, 0.0f }));
    myWorld.AddEntity(std::make_unique<Plant>((Vector3){5.0f, 0.0f, -3.0f }));
    myWorld.AddEntity(std::make_unique<Plant>((Vector3){-4.0f, 0.0f, 2.0f }));
    myWorld.AddEntity(std::make_unique<Wolf>((Vector3){ 10.0f, 0.0f, 10.0f }));

    // продвинутый спавн овец
    SpawnSheepSmartAndLogical(myWorld);

    Camera3D camera = { 0 };
    camera.position = (Vector3){10.0f, 10.0f, 10.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    while(!WindowShouldClose()){

        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_LEFT_ALT)) {
            if (IsCursorHidden()) EnableCursor();
            else DisableCursor();
        }

        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        UpdateCamera(&camera, CAMERA_FREE);

        myWorld.Update(dt);

        BeginDrawing();
        ClearBackground(SKYBLUE);
        BeginMode3D(camera);
        DrawGrid(20, 1.0f);
        myWorld.Draw();
        EndMode3D();
        EndDrawing();
    }
    EnableCursor();
    CloseWindow();
    return 0;
}