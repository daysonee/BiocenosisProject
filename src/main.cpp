#pragma warning(disable: 4576)
#include "raylib.h"
#include "raymath.h"
#include "core/world.hpp"
#include "entities/plant.hpp"
#include "entities/animal.hpp"
#include "entities/sheep.hpp"
#include "entities/wolf.hpp"
#include "core/constants.hpp"
#include "camera/CameraSpawn.hpp"
#include "camera/CameraController.hpp"
#include "ui/EcoStatsDisplay.hpp"
#include <memory>
#include <cmath>

// СПАВН ОВЕЦ СТАДАМИ
void SpawnSheepSmartAndLogical(World& world) {
    const int halfMap = Config::World::MAP_SIZE / 2;
    const int totalSheepToSpawn = Config::Sheep::INITIAL_COUNT;
    int spawnedCount = 0;

    const int   minFlockSize = 3;
    const int   maxFlockSize = 6;
    const int   maxFlockAttempts = 1000;

    for (int attempt = 0; attempt < maxFlockAttempts && spawnedCount < totalSheepToSpawn; ++attempt) {
        float cx = (float)GetRandomValue(-halfMap, halfMap);
        float cz = (float)GetRandomValue(-halfMap, halfMap);
        float cy = world.GetHeight(cx, cz);

        if (cy <= Config::World::WATER_LEVEL || cy > Config::World::SAND_LEVEL + 0.3f) {
            continue;
        }

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

        const int currentFlockSize = GetRandomValue(minFlockSize, maxFlockSize);
        const Vector3 flockCenter = { cx, cy, cz };

        for (int i = 0; i < currentFlockSize; ++i) {
            if (spawnedCount >= totalSheepToSpawn) break;

            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float radius = (float)GetRandomValue(10, (int)(Config::Sheep::SPAWN_FLOCK_RADIUS * 10)) / 10.0f;

            float sx = cx + cosf(angle) * radius;
            float sz = cz + sinf(angle) * radius;

            if (sx < -halfMap || sx > halfMap || sz < -halfMap || sz > halfMap) continue;

            float sy = world.GetHeight(sx, sz);
            if (sy <= Config::World::WATER_LEVEL) continue;

            auto sheep = std::make_unique<Sheep>((Vector3) { sx, sy, sz });
            sheep->SetFlockCenter(flockCenter);
            world.AddEntity(std::move(sheep));
            ++spawnedCount;
        }
    }

    if (spawnedCount < totalSheepToSpawn) {
        int fallbackAttempts = 1000;
        while (spawnedCount < totalSheepToSpawn && fallbackAttempts-- > 0) {
            float rx = (float)GetRandomValue(-halfMap, halfMap);
            float rz = (float)GetRandomValue(-halfMap, halfMap);
            float ry = world.GetHeight(rx, rz);

            if (ry > Config::World::WATER_LEVEL) {
                auto sheep = std::make_unique<Sheep>((Vector3) { rx, ry, rz });
                sheep->SetFlockCenter({ rx, ry, rz });
                world.AddEntity(std::move(sheep));
                ++spawnedCount;
            }
        }
    }
}

int main() {
    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_FULLSCREEN_MODE);

    int currentMonitor = GetCurrentMonitor();
    int screenWidth = GetMonitorWidth(currentMonitor);
    int screenHeight = GetMonitorHeight(currentMonitor);

    InitWindow(screenWidth, screenHeight, "BioCynosis - Ecosystem Simulation");

    SetTargetFPS(60);

    // ========== МУЗЫКА ==========
    InitAudioDevice();
    // Пробуем разные форматы (mp3, ogg, wav)
    Music backgroundMusic;

    // Сначала пробуем mp3
    backgroundMusic = LoadMusicStream("ZXKAI_-_NO_BATIDAO_80337833.mp3");

    // Если файл не загрузился, проверяем существует ли он
    if (backgroundMusic.stream.buffer == nullptr) {
        // Пробуем ogg
        backgroundMusic = LoadMusicStream("ZXKAI_-_NO_BATIDAO_80337833.ogg");
    }
    if (backgroundMusic.stream.buffer == nullptr) {
        // Пробуем wav
        backgroundMusic = LoadMusicStream("ZXKAI_-_NO_BATIDAO_80337833.wav");
    }

    backgroundMusic.looping = true;
    PlayMusicStream(backgroundMusic);
    // ============================

    World myWorld;

    myWorld.AddEntity(std::make_unique<Wolf>((Vector3) { 10.0f, 0.0f, 10.0f }));
    SpawnSheepSmartAndLogical(myWorld);

    // НАСТРОЙКА КАМЕРЫ
    Vector3 startPos = { 0.0f, 45.0f, -60.0f };
    float startYaw = 0.0f;
    float startPitch = 25.0f;

    Camera3D camera = { 0 };
    camera.position = startPos;
    camera.target = { 0.0f, 20.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 70.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    CameraSpawn cameraSpawn(startPos, startPitch, startYaw);
    CameraController cameraController(startPos, startYaw, startPitch);

    // СТАТИСТИКА
    EcoStatsDisplay stats(1050, 10);

    double totalSimTime = 0.0;
    int totalSheepBorn = 0;
    int totalWolvesBorn = 0;
    int totalSheepEaten = 0;
    int totalWolvesStarved = 0;

    // ========== НАЧАЛЬНЫЙ ПОДСЧЁТ СТАТИСТИКИ ==========
    int initialSheep = 0, initialWolves = 0, initialGrass = 0;
    for (const auto& entity : myWorld.GetEntities()) {
        if (!entity->IsAlive()) continue;
        if (dynamic_cast<Sheep*>(entity.get())) initialSheep++;
        else if (dynamic_cast<Wolf*>(entity.get())) initialWolves++;
        else if (dynamic_cast<Plant*>(entity.get())) initialGrass++;
    }

    stats.setSheepCount(initialSheep, totalSheepBorn);
    stats.setWolfCount(initialWolves, totalWolvesBorn);
    stats.setGrassCount(initialGrass);
    stats.setSimulationTime(0);
    stats.setSheepEaten(0);
    stats.setStarvedWolves(0);
    // =================================================

    float timeScale = 1.0f;
    Rectangle speedBtn = { 20, 20, 160, 40 };

    bool mouseCaptured = true;
    bool isPaused = false;
    DisableCursor();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.033f) dt = 0.033f;

        // ========== ОБНОВЛЕНИЕ МУЗЫКИ (каждый кадр) ==========
        UpdateMusicStream(backgroundMusic);
        // =====================================================

        // ПАУЗА ПО КЛАВИШЕ P
        if (IsKeyPressed(KEY_P)) {
            isPaused = !isPaused;
        }

        // УПРАВЛЕНИЕ КУРСОРОМ
        if (IsKeyPressed(KEY_ESCAPE)) {
            mouseCaptured = false;
            EnableCursor();
        }

        Vector2 mousePos = GetMousePosition();
        bool isClickOnButton = CheckCollisionPointRec(mousePos, speedBtn);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !isClickOnButton && !mouseCaptured) {
            mouseCaptured = true;
            DisableCursor();
        }

        if (IsKeyPressed(KEY_LEFT_ALT)) {
            mouseCaptured = !mouseCaptured;
            if (mouseCaptured) DisableCursor();
            else EnableCursor();
        }

        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        // КАМЕРА (работает всегда, даже на паузе)
        if (mouseCaptured) {
            cameraController.Update(dt, camera, true);
        }

        cameraSpawn.Update(dt, camera);

        if (cameraSpawn.IsAnimationFinished()) {
            cameraController.SetPosition(camera.position);
        }

        // КНОПКА УСКОРЕНИЯ
        bool isBtnHovered = !mouseCaptured && CheckCollisionPointRec(mousePos, speedBtn);
        if (isBtnHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (timeScale == 1.0f) timeScale = 5.0f;
            else if (timeScale == 5.0f) timeScale = 10.0f;
            else if (timeScale == 10.0f) timeScale = 20.0f;
            else timeScale = 1.0f;
        }

        // МИР (обновляется только если НЕ на паузе)
        if (!isPaused) {
            myWorld.Update(dt * timeScale);
            totalSimTime += dt * timeScale;
        }

        // СБОР СТАТИСТИКИ
        int aliveSheep = 0, aliveWolves = 0, grassPatches = 0;
        for (const auto& entity : myWorld.GetEntities()) {
            if (!entity->IsAlive()) continue;
            if (dynamic_cast<Sheep*>(entity.get())) aliveSheep++;
            else if (dynamic_cast<Wolf*>(entity.get())) aliveWolves++;
            else if (dynamic_cast<Plant*>(entity.get())) grassPatches++;
        }

        static int lastSheepCount = 0;
        if (aliveSheep < lastSheepCount) {
            totalSheepEaten += (lastSheepCount - aliveSheep);
        }
        lastSheepCount = aliveSheep;

        stats.setSheepCount(aliveSheep, totalSheepBorn);
        stats.setWolfCount(aliveWolves, totalWolvesBorn);
        stats.setGrassCount(grassPatches);
        stats.setSimulationTime(totalSimTime);
        stats.setSheepEaten(totalSheepEaten);
        stats.setStarvedWolves(totalWolvesStarved);

        stats.Update(dt);

        // ОТРИСОВКА
        BeginDrawing();
        ClearBackground(SKYBLUE);

        BeginMode3D(camera);
        myWorld.Draw();
        EndMode3D();

        // 2D ИНТЕРФЕЙС
        Color btnColor = isBtnHovered ? LIGHTGRAY : RAYWHITE;
        DrawRectangleRec(speedBtn, btnColor);
        DrawRectangleLinesEx(speedBtn, 2, BLACK);
        DrawText(TextFormat("TIME: %.0fx", timeScale), speedBtn.x + 20, speedBtn.y + 10, 20, BLACK);

        // ИНДИКАТОР ПАУЗЫ
        if (isPaused) {
            DrawText("PAUSED [P]", screenWidth / 2 - 80, 20, 30, RED);
        }

        if (mouseCaptured) {
            DrawText(TextFormat("MOVE SPEED: %s", cameraController.GetSpeedModeText()),
                screenWidth - 200, 20, 18, YELLOW);
        }

        // СТАТИСТИКА
        stats.Draw();

        // ПОДСКАЗКИ
        if (!mouseCaptured) {
            DrawText("Click in window to capture mouse", screenWidth / 2 - 200, screenHeight - 30, 18, DARKGRAY);
        }
        else {
            DrawText("WASD + Mouse | Q/E up/down | Shift/Ctrl speed | P pause | LEFT ALT free cursor",
                screenWidth / 2 - 400, screenHeight - 30, 18, DARKGRAY);
        }

        EndDrawing();
    }

    // ========== ОСВОБОЖДЕНИЕ РЕСУРСОВ МУЗЫКИ ==========
    UnloadMusicStream(backgroundMusic);
    CloseAudioDevice();
    // ===================================================

    EnableCursor();
    CloseWindow();
    return 0;
}