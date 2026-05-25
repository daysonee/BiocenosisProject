#include "raylib.h"
#include "raymath.h"
#include "core/world.hpp"
#include "entities/plant.hpp"
#include "entities/animal.hpp"
#include "entities/sheep.hpp"
#include "entities/wolf.hpp"
#include "core/constants.hpp"
#include <memory>
#include <cmath>
 
// СПАВН ОВЕЦ СТАДАМИ
void SpawnSheepSmartAndLogical(World& world) {
    const int halfMap           = Config::World::MAP_SIZE / 2;
    const int totalSheepToSpawn = Config::Sheep::INITIAL_COUNT;
    int spawnedCount            = 0;
 
    const int   minFlockSize     = 3;
    const int   maxFlockSize     = 6;
    const int   maxFlockAttempts = 1000;
 
    // --- Этап 1: спавн стадами на прибрежных зонах вдали от волков ---
    for (int attempt = 0; attempt < maxFlockAttempts && spawnedCount < totalSheepToSpawn; ++attempt) {
 
        // Выбираем потенциальный центр стада
        float cx = (float)GetRandomValue(-halfMap, halfMap);
        float cz = (float)GetRandomValue(-halfMap, halfMap);
        float cy = world.GetHeight(cx, cz);
 
        // Только прибрежные зоны (песок / низкие луга)
        if (cy <= Config::World::WATER_LEVEL || cy > Config::World::SAND_LEVEL + 0.3f) {
            continue;
        }
 
        // Нет ли рядом волков?
        bool isZoneSafe = true;
        for (const auto& entity : world.GetEntities()) {
            if (dynamic_cast<Wolf*>(entity.get())) {
                if (Vector3Distance({ cx, cy, cz }, entity->GetPosition()) < Config::Sheep::SAFE_ZONE_FROM_WOLVES) {
                    isZoneSafe = false;
                    break;
                }
            }
        }
        if (!isZoneSafe) continue;
 
        // Формируем стадо вокруг найденного центра
        const int currentFlockSize = GetRandomValue(minFlockSize, maxFlockSize);
        const Vector3 flockCenter  = { cx, cy, cz };
 
        for (int i = 0; i < currentFlockSize; ++i) {
            if (spawnedCount >= totalSheepToSpawn) break;
 
            // Полярные координаты — органичное круговое распределение
            float angle  = (float)GetRandomValue(0, 360) * DEG2RAD;
            float radius = (float)GetRandomValue(10, (int)(Config::Sheep::SPAWN_FLOCK_RADIUS * 10)) / 10.0f;
 
            float sx = cx + cosf(angle) * radius;
            float sz = cz + sinf(angle) * radius;
 
            if (sx < -halfMap || sx > halfMap || sz < -halfMap || sz > halfMap) continue;
 
            float sy = world.GetHeight(sx, sz);
            if (sy <= Config::World::WATER_LEVEL) continue;
 
            // Создаём овечку и сразу назначаем ей центр стада
            auto sheep = std::make_unique<Sheep>((Vector3){ sx, sy, sz });
            sheep->SetFlockCenter(flockCenter);
            world.AddEntity(std::move(sheep));
            ++spawnedCount;
        }
    }
 
    // --- Этап 2: фолбэк — доспавниваем оставшихся на любой суше ---
    if (spawnedCount < totalSheepToSpawn) {
        int fallbackAttempts = 1000;
        while (spawnedCount < totalSheepToSpawn && fallbackAttempts-- > 0) {
            float rx = (float)GetRandomValue(-halfMap, halfMap);
            float rz = (float)GetRandomValue(-halfMap, halfMap);
            float ry = world.GetHeight(rx, rz);
 
            if (ry > Config::World::WATER_LEVEL) {
                auto sheep = std::make_unique<Sheep>((Vector3){ rx, ry, rz });
                // Для фолбэк-овец центр стада = точка спавна (одиночки)
                sheep->SetFlockCenter({ rx, ry, rz });
                world.AddEntity(std::move(sheep));
                ++spawnedCount;
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

    myWorld.AddEntity(std::make_unique<Wolf>((Vector3){ 10.0f, 0.0f, 10.0f }));

    // продвинутый спавн овец
    SpawnSheepSmartAndLogical(myWorld);

    Camera3D camera = { 0 };
    camera.position = (Vector3){10.0f, 10.0f, 10.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

// --- НОВЫЕ ПЕРЕМЕННЫЕ ДЛЯ УПРАВЛЕНИЯ ВРЕМЕНЕМ ---
    float timeScale = 1.0f;
    Rectangle speedBtn = { 20, 20, 160, 40 }; // x, y, width, height

    while(!WindowShouldClose()){

        float dt = GetFrameTime();

        // 1. Управление курсором и фулскрином
        if (IsKeyPressed(KEY_LEFT_ALT)) {
            if (IsCursorHidden()) EnableCursor();
            else DisableCursor();
        }

        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        // 2. Логика кнопки ускорения времени
        Vector2 mousePos = GetMousePosition();
        bool isBtnHovered = !IsCursorHidden() && CheckCollisionPointRec(mousePos, speedBtn);

        // Переключаем скорость по клику левой кнопкой мыши
        if (isBtnHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (timeScale == 1.0f) timeScale = 5.0f;
            else if (timeScale == 5.0f) timeScale = 10.0f;
            else if (timeScale == 10.0f) timeScale = 20.0f; // Новое состояние
            else timeScale = 1.0f;
        }

        UpdateCamera(&camera, CAMERA_FREE);

        // 3. УМНОЖАЕМ dt НА timeScale (магия ускорения здесь!)
        myWorld.Update(dt * timeScale);

        // 4. Отрисовка
        BeginDrawing();
        ClearBackground(SKYBLUE);
        
        // Рисуем 3D мир
        BeginMode3D(camera);
        myWorld.Draw();
        EndMode3D();

        // --- 2D ИНТЕРФЕЙС (рисуется ПОВЕРХ 3D) ---
        Color btnColor = isBtnHovered ? LIGHTGRAY : RAYWHITE;
        
        // Фон кнопки и рамка
        DrawRectangleRec(speedBtn, btnColor);
        DrawRectangleLinesEx(speedBtn, 2, BLACK);
        
        // Текст на кнопке (TextFormat позволяет вставить переменную прямо в строку)
        DrawText(TextFormat("TIME: %.0fx", timeScale), speedBtn.x + 20, speedBtn.y + 10, 20, BLACK);
        
        // Подсказка, если курсор скрыт
        if (IsCursorHidden()) {
            DrawText("Press LEFT ALT to use UI", 20, 70, 20, DARKGRAY);
        }

        EndDrawing();
    }

    EnableCursor();
    CloseWindow();
    return 0;
}