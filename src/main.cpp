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
#include "entities/crab.hpp"
#include "entities/hunter.hpp"
#include "ui/MainMenu.hpp"
#include <memory>
#include <cmath>

// СПАВН ВОЛКОВ СТАЯМИ В ЛЕСУ
void SpawnWolfPacksInForest(World& world, const MenuSettings& settings) {
    const int halfMap = Config::World::MAP_SIZE / 2;
    
    // Вместо фиксированных стай считаем, сколько волков НАДО спавнить всего
    int wolvesToSpawn = settings.wolfCount; 
    int wolvesSpawned = 0;

    const float FOREST_MIN = Config::World::SAND_LEVEL;
    const float FOREST_MAX = Config::World::BIOME_THRESHOLD;

    int currentPackId = 1;
    const int maxAttempts = 5000;

    for (int attempt = 0; attempt < maxAttempts && wolvesSpawned < wolvesToSpawn; ++attempt) {
        float rx = (float)GetRandomValue(-halfMap, halfMap);
        float rz = (float)GetRandomValue(-halfMap, halfMap);
        float height = world.GetHeight(rx, rz);

        if (height >= FOREST_MIN && height <= FOREST_MAX) {
            // Определяем, сколько волков заспавнить в текущую стаю (например, от 3 до 4, но не больше, чем осталось)
            int remainingWolves = wolvesToSpawn - wolvesSpawned;
            int packSize = GetRandomValue(Config::Wolf::PACK_SIZE_MIN, Config::Wolf::PACK_SIZE_MAX);
            if (packSize > remainingWolves) packSize = remainingWolves;

            Vector3 packCenter = { rx, height, rz };

            // Спавним членов стаи вокруг центра
            for (int i = 0; i < packSize; ++i) {
                float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
                float radius = (float)GetRandomValue(0, (int)(Config::Wolf::PACK_RADIUS * 10)) / 10.0f;

                Vector3 spawnPos = {
                    packCenter.x + cosf(angle) * radius,
                    0.0f,
                    packCenter.z + sinf(angle) * radius
                };
                spawnPos.y = world.GetHeight(spawnPos.x, spawnPos.z);

                // Первый волк в стае будет вожаком (ADULT), остальные — MEDIUM или BABY
                Config::Wolf::AgeStage stage = Config::Wolf::AgeStage::MEDIUM;
                if (i == 0) {
                    stage = Config::Wolf::AgeStage::ADULT;
                } else if (GetRandomValue(0, 100) < 25) { // 25% шанс на ребенка
                    stage = Config::Wolf::AgeStage::BABY;
                }

                auto wolf = std::make_unique<Wolf>(spawnPos, stage, currentPackId);
                wolf->SetHomePos(packCenter);
                
                if (i == 0) {
                    wolf->PromoteToLeader();
                }

                world.AddEntity(std::move(wolf));
                wolvesSpawned++;
            }
            
            currentPackId++; // Переходим к следующей стае
        }
    }
}

// СПАВН ОВЕЦ СТАДАМИ ИСКЛЮЧИТЕЛЬНО НА ЯРКО-ЗЕЛЕНЫХ ЛУГАХ
void SpawnSheepSmartAndLogical(World& world, int countToSpawn) {
    const int halfMap = Config::World::MAP_SIZE / 2;
    int spawnedCount = 0;

    const int   minFlockSize = 3;
    const int   maxFlockSize = 6;
    const int   maxFlockAttempts = 3000;

    const float MEADOW_MIN = Config::World::BIOME_THRESHOLD;
    const float MEADOW_MAX = Config::World::PLANT_LEVEL;

    for (int attempt = 0; attempt < maxFlockAttempts && spawnedCount < countToSpawn; ++attempt) {
        float cx = (float)GetRandomValue(-halfMap, halfMap);
        float cz = (float)GetRandomValue(-halfMap, halfMap);
        float cy = world.GetHeight(cx, cz);

        if (cy <= MEADOW_MIN || cy > MEADOW_MAX) continue;

        const int currentFlockSize = GetRandomValue(minFlockSize, maxFlockSize);
        const Vector3 flockCenter = { cx, cy, cz };

        for (int i = 0; i < currentFlockSize; ++i) {
            if (spawnedCount >= countToSpawn) break;

            float angle = (float)GetRandomValue(0, 360) * DEG2RAD;
            float radius = (float)GetRandomValue(10, (int)(Config::Sheep::SPAWN_FLOCK_RADIUS * 10)) / 10.0f;

            float sx = cx + cosf(angle) * radius;
            float sz = cz + sinf(angle) * radius;

            if (sx < -halfMap || sx > halfMap || sz < -halfMap || sz > halfMap) continue;

            float sy = world.GetHeight(sx, sz);

            if (sy <= MEADOW_MIN || sy > MEADOW_MAX) continue;

            auto sheep = std::make_unique<Sheep>((Vector3) { sx, sy, sz });
            sheep->SetFlockCenter(flockCenter);
            world.AddEntity(std::move(sheep));
            ++spawnedCount;
        }
    }

    if (spawnedCount < countToSpawn) {
        int fallbackAttempts = 2000;
        while (spawnedCount < countToSpawn && fallbackAttempts-- > 0) {
            float rx = (float)GetRandomValue(-halfMap, halfMap);
            float rz = (float)GetRandomValue(-halfMap, halfMap);
            float ry = world.GetHeight(rx, rz);

            if (ry > MEADOW_MIN && ry <= MEADOW_MAX) {
                auto sheep = std::make_unique<Sheep>((Vector3) { rx, ry, rz });
                sheep->SetFlockCenter({ rx, ry, rz });
                world.AddEntity(std::move(sheep));
                ++spawnedCount;
            }
        }
    }
}

void SpawnWolves(World& world, int countToSpawn) {
    const int halfMap = Config::World::MAP_SIZE / 2;
    int spawned = 0;
    int attempts = 2000;

    for (int i = 0; i < attempts && spawned < countToSpawn; ++i) {
        float rx = (float)GetRandomValue(-halfMap, halfMap);
        float rz = (float)GetRandomValue(-halfMap, halfMap);
        float ry = world.GetHeight(rx, rz);

        if (ry > Config::World::WATER_LEVEL) {
            world.AddEntity(std::make_unique<Wolf>((Vector3) { rx, ry, rz }));
            spawned++;
        }
    }
}

void SpawnCrabsOnBeaches(World& world, int countToSpawn) {
    const int halfMap = Config::World::MAP_SIZE / 2;
    int spawned = 0;
    int attempts = 3000;

    for (int i = 0; i < attempts && spawned < countToSpawn; ++i) {
        float rx = (float)GetRandomValue(-halfMap, halfMap);
        float rz = (float)GetRandomValue(-halfMap, halfMap);
        float ry = world.GetHeight(rx, rz);

        if (ry >= Config::World::WATER_LEVEL && ry <= Config::World::SAND_LEVEL) {
            world.AddEntity(std::make_unique<Crab>((Vector3) { rx, ry, rz }));
            spawned++;
        }
    }
}

// ========== СИСТЕМА ФИКСИРОВАННЫХ КАМЕР ==========
struct FixedCamera {
    Vector3 position;
    Vector3 target;
    const char* name;
};

const FixedCamera fixedCameras[] = {
    { { 0.0f, 80.0f, -150.0f }, { 0.0f, 30.0f, 0.0f }, "OVERVIEW" },
    { { -200.0f, 50.0f, -100.0f }, { -150.0f, 30.0f, -80.0f }, "WEST FOREST" },
    { { 200.0f, 50.0f, -100.0f }, { 150.0f, 30.0f, -80.0f }, "EAST FOREST" },
    { { -100.0f, 40.0f, 150.0f }, { -80.0f, 25.0f, 100.0f }, "SOUTH BEACH" },
    { { 100.0f, 60.0f, 120.0f }, { 80.0f, 35.0f, 80.0f }, "NORTH MEADOW" },
    { { 0.0f, 90.0f, -80.0f }, { 0.0f, 20.0f, 0.0f }, "CENTER" }
};

const int FIXED_CAMERA_COUNT = sizeof(fixedCameras) / sizeof(fixedCameras[0]);

class FixedCameraSystem {
private:
    int currentCameraIndex = 0;
    bool isActive = false;
    Camera3D savedCamera;
    bool hasSavedCamera = false;

public:
    void Activate(const Camera3D& currentCamera) {
        if (!isActive) {
            savedCamera = currentCamera;
            hasSavedCamera = true;
            isActive = true;
            currentCameraIndex = 0;
        }
    }

    void Deactivate() {
        isActive = false;
    }

    bool IsActive() const {
        return isActive;
    }

    void NextCamera() {
        if (isActive) {
            currentCameraIndex = (currentCameraIndex + 1) % FIXED_CAMERA_COUNT;
        }
    }

    void PrevCamera() {
        if (isActive) {
            currentCameraIndex = (currentCameraIndex - 1 + FIXED_CAMERA_COUNT) % FIXED_CAMERA_COUNT;
        }
    }

    void ApplyToCamera(Camera3D& camera) {
        if (isActive) {
            const FixedCamera& fc = fixedCameras[currentCameraIndex];
            camera.position = fc.position;
            camera.target = fc.target;
        }
    }

    void RestoreSavedCamera(Camera3D& camera) {
        if (hasSavedCamera) {
            camera = savedCamera;
            hasSavedCamera = false;
        }
    }

    const char* GetCurrentCameraName() const {
        if (isActive) {
            return fixedCameras[currentCameraIndex].name;
        }
        return "";
    }

    int GetCurrentIndex() const {
        return currentCameraIndex;
    }
};

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    int screenWidth = 1280;
    int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "BioCynosis - Ecosystem Simulation");
    TraceLog(LOG_INFO, "CURRENT PATH: %s", GetWorkingDirectory());
    // ========== МЕНЮ ==========
    MenuSettings menuSettings;
    MainMenu mainMenu(menuSettings, screenWidth, screenHeight);

    while (!menuSettings.readyToStart && !WindowShouldClose()) {
        mainMenu.Update();
        BeginDrawing();
        ClearBackground(DARKBLUE);
        mainMenu.Draw();
        EndDrawing();
    }

    if (WindowShouldClose()) {
        CloseWindow();
        return 0;
    }

    SetTargetFPS(60);

    // ========== МУЗЫКА ==========
    InitAudioDevice();

    World* myWorld = new World();

    // Стаями в лесу (по умолчанию ровно PACK_COUNT стай × 3-4 волка)
    SpawnWolfPacksInForest(*myWorld, menuSettings);
    SpawnSheepSmartAndLogical(*myWorld, menuSettings.sheepCount);
    SpawnCrabsOnBeaches(*myWorld, Config::Crab::INITIAL_COUNT);

    // Охотник живёт в своей хижине. Спавним ровно ОДНОГО, в точке хижины.
    Vector3 hutPos = myWorld->GetHunterHutPosition();
    myWorld->AddEntity(std::make_unique<Hunter>(hutPos));

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

    FixedCameraSystem fixedCameraSystem;

    EcoStatsDisplay stats(1050, 10);

    double totalSimTime = 0.0;
    int totalSheepBorn = 0;
    int totalWolvesBorn = 0;
    int totalSheepEaten = 0;
    int totalWolvesStarved = 0;

    // ========== НАЧАЛЬНЫЙ ПОДСЧЁТ СТАТИСТИКИ ==========
    int initialSheep = 0, initialWolves = 0;
    for (const auto& entity : myWorld->GetEntities()) {
        if (!entity->IsAlive()) continue;
        if (dynamic_cast<Sheep*>(entity.get())) initialSheep++;
        else if (dynamic_cast<Wolf*>(entity.get())) initialWolves++;
    }

    int initialGrass = myWorld->GetGrassCount();

    totalSheepBorn = 0;
    totalWolvesBorn = 0;

    int lastSheepCount = initialSheep;
    int lastWolfCount = initialWolves;

    stats.setSheepCount(initialSheep, totalSheepBorn);
    stats.setWolfCount(initialWolves, totalWolvesBorn);
    stats.setGrassCount(initialGrass);
    stats.setSimulationTime(0);
    stats.setSheepEaten(0);
    stats.setStarvedWolves(0);

    float timeScale = 1.0f;
    Rectangle speedBtn = { 20, 20, 160, 40 };

    bool mouseCaptured = true;
    bool isPaused = false;
    DisableCursor();

    // ========== ПЕРЕМЕННЫЕ ДЛЯ РЕГЕНЕРАЦИИ ==========
    bool isRegenerating = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        screenWidth = GetScreenWidth();
        screenHeight = GetScreenHeight();
        if (dt > 0.033f) dt = 0.033f;


        if (IsKeyPressed(KEY_P)) {
            isPaused = !isPaused;
        }

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

        // ========== КЛАВИША R - РЕГЕНЕРАЦИЯ МИРА ==========
        if (IsKeyPressed(KEY_R)) {
            isRegenerating = true;
        }

        // Обработка диалога регенерации
        if (isRegenerating) {
            // Ждём нажатия Y или N
            if (IsKeyPressed(KEY_Y)) {
                // СОЗДАЁМ НОВЫЙ МИР
                World* newWorld = new World();

                SpawnWolfPacksInForest(*newWorld, menuSettings);
                SpawnSheepSmartAndLogical(*newWorld, menuSettings.sheepCount);
                SpawnCrabsOnBeaches(*newWorld, Config::Crab::INITIAL_COUNT);
                // ОДИН охотник, ровно в точке хижины нового мира
                {
                    Vector3 newHut = newWorld->GetHunterHutPosition();
                    newWorld->AddEntity(std::make_unique<Hunter>(newHut));
                }

                delete myWorld;
                myWorld = newWorld;

                // СБРАСЫВАЕМ ВСЮ СТАТИСТИКУ
                totalSheepBorn = 0;
                totalWolvesBorn = 0;
                totalSheepEaten = 0;
                totalWolvesStarved = 0;
                totalSimTime = 0.0;

                // СБРАСЫВАЕМ КАМЕРУ
                camera.position = startPos;
                camera.target = { 0.0f, 20.0f, 0.0f };
                cameraController.SetPosition(startPos);

                // ПЕРЕСЧИТЫВАЕМ НАЧАЛЬНУЮ СТАТИСТИКУ
                int newInitialSheep = 0, newInitialWolves = 0;
                for (const auto& entity : myWorld->GetEntities()) {
                    if (!entity->IsAlive()) continue;
                    if (dynamic_cast<Sheep*>(entity.get())) newInitialSheep++;
                    else if (dynamic_cast<Wolf*>(entity.get())) newInitialWolves++;
                }

                lastSheepCount = newInitialSheep;
                lastWolfCount = newInitialWolves;
                int newInitialGrass = myWorld->GetGrassCount();

                stats.setSheepCount(newInitialSheep, totalSheepBorn);
                stats.setWolfCount(newInitialWolves, totalWolvesBorn);
                stats.setGrassCount(newInitialGrass);
                stats.setSimulationTime(0);
                stats.setSheepEaten(0);
                stats.setStarvedWolves(0);

                isRegenerating = false;
            }
            else if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE)) {
                isRegenerating = false;
            }
        }

        // ========== УПРАВЛЕНИЕ ФИКСИРОВАННЫМИ КАМЕРАМИ ==========
        if (IsKeyPressed(KEY_TAB)) {
            if (fixedCameraSystem.IsActive()) {
                fixedCameraSystem.RestoreSavedCamera(camera);
                fixedCameraSystem.Deactivate();
                mouseCaptured = true;
                DisableCursor();
            }
            else {
                fixedCameraSystem.Activate(camera);
                mouseCaptured = false;
                EnableCursor();
            }
        }

        if (fixedCameraSystem.IsActive()) {
            if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
                fixedCameraSystem.NextCamera();
            }
            if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
                fixedCameraSystem.PrevCamera();
            }
        }

        if (fixedCameraSystem.IsActive()) {
            fixedCameraSystem.ApplyToCamera(camera);
        }

        // Клавиша O - спавн 10 овец
        if (IsKeyPressed(KEY_O)) {
            Vector3 forward = Vector3Subtract(camera.target, camera.position);
            forward = Vector3Normalize(forward);

            Vector3 basePos = camera.position;
            basePos.x += forward.x * 20.0f;
            basePos.z += forward.z * 20.0f;

            for (int i = 0; i < 10; ++i) {
                float offsetX = (float)GetRandomValue(-150, 150) / 10.0f;
                float offsetZ = (float)GetRandomValue(-150, 150) / 10.0f;

                float sx = basePos.x + offsetX;
                float sz = basePos.z + offsetZ;
                float sy = myWorld->GetHeight(sx, sz);

                if (sy > Config::World::WATER_LEVEL) {
                    auto sheep = std::unique_ptr<Sheep>(new Sheep((Vector3) { sx, sy, sz }));
                    sheep->SetFlockCenter({ sx, sy, sz });
                    myWorld->AddEntity(std::move(sheep));
                }
            }
            myWorld->SpawnParticles(basePos, GREEN, 30, false);
        }

        // Клавиша V - спавн 1 волка
        if (IsKeyPressed(KEY_V)) {
            Vector3 forward = Vector3Subtract(camera.target, camera.position);
            forward = Vector3Normalize(forward);

            Vector3 spawnPos = camera.position;
            spawnPos.x += forward.x * 20.0f;
            spawnPos.z += forward.z * 20.0f;
            spawnPos.y = myWorld->GetHeight(spawnPos.x, spawnPos.z);

            myWorld->AddEntity(std::unique_ptr<Wolf>(new Wolf(spawnPos)));
            myWorld->SpawnParticles(spawnPos, RED, 15, false);
        }

        if (mouseCaptured && !fixedCameraSystem.IsActive()) {
            cameraController.Update(dt, camera, true);
        }

        cameraSpawn.Update(dt, camera);

        if (cameraSpawn.IsAnimationFinished()) {
            cameraController.SetPosition(camera.position);
        }

        bool isBtnHovered = !mouseCaptured && CheckCollisionPointRec(mousePos, speedBtn);
        if (isBtnHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (timeScale == 1.0f) timeScale = 5.0f;
            else if (timeScale == 5.0f) timeScale = 10.0f;
            else if (timeScale == 10.0f) timeScale = 20.0f;
            else timeScale = 1.0f;
        }

        if (!isPaused) {
            myWorld->Update(dt * timeScale);
            totalSimTime += dt * timeScale;
        }

        // ========== ПОДСЧЁТ СТАТИСТИКИ ==========
        int aliveSheep = 0, aliveWolves = 0;
        for (const auto& entity : myWorld->GetEntities()) {
            if (!entity->IsAlive()) continue;
            if (dynamic_cast<Sheep*>(entity.get())) aliveSheep++;
            else if (dynamic_cast<Wolf*>(entity.get())) aliveWolves++;
        }

        int grassCount = myWorld->GetGrassCount();

        int sheepDelta = aliveSheep - lastSheepCount;
        if (sheepDelta < 0) {
            totalSheepEaten += (-sheepDelta);
        } else if (sheepDelta > 0) {
            totalSheepBorn += sheepDelta;
        }
        lastSheepCount = aliveSheep;

        int wolfDelta = aliveWolves - lastWolfCount;
        if (wolfDelta < 0) {
            totalWolvesStarved += (-wolfDelta);
        } else if (wolfDelta > 0) {
            totalWolvesBorn += wolfDelta;
        }
        lastWolfCount = aliveWolves;

        stats.setSheepCount(aliveSheep, totalSheepBorn);
        stats.setWolfCount(aliveWolves, totalWolvesBorn);
        stats.setGrassCount(grassCount);
        stats.setSimulationTime(totalSimTime);
        stats.setSheepEaten(totalSheepEaten);
        stats.setStarvedWolves(totalWolvesStarved);

        stats.Update(dt);

        // ========== ОТРИСОВКА ==========
        BeginDrawing();
        ClearBackground(SKYBLUE);

        BeginMode3D(camera);
        myWorld->Draw();
        EndMode3D();

        Color btnColor = isBtnHovered ? LIGHTGRAY : RAYWHITE;
        DrawRectangleRec(speedBtn, btnColor);
        DrawRectangleLinesEx(speedBtn, 2, BLACK);
        DrawText(TextFormat("TIME: %.0fx", timeScale), speedBtn.x + 20, speedBtn.y + 10, 20, BLACK);

        if (isPaused) {
            DrawText("PAUSED [P]", screenWidth / 2 - 80, 20, 30, RED);
        }

        if (mouseCaptured) {
            DrawText(TextFormat("MOVE SPEED: %s", cameraController.GetSpeedModeText()),
                screenWidth - 200, 20, 18, YELLOW);
        }

        stats.Draw();

        // ========== ДИАЛОГ ПОДТВЕРЖДЕНИЯ РЕГЕНЕРАЦИИ ==========
        if (isRegenerating) {
            DrawRectangle(0, 0, screenWidth, screenHeight, (Color) { 0, 0, 0, 180 });

            Rectangle dialogBox = { screenWidth / 2.0f - 200.0f, screenHeight / 2.0f - 75.0f, 400, 150 };
            DrawRectangleRec(dialogBox, (Color) { 50, 50, 70, 255 });
            DrawRectangleLinesEx(dialogBox, 3, YELLOW);

            DrawText("Create new world?", screenWidth / 2 - MeasureText("Create new world?", 24) / 2,
                screenHeight / 2 - 45, 24, WHITE);
            DrawText("Current progress will be lost!", screenWidth / 2 - MeasureText("Current progress will be lost!", 18) / 2,
                screenHeight / 2 - 15, 18, ORANGE);

            DrawText("Press Y - YES     N - NO", screenWidth / 2 - MeasureText("Press Y - YES     N - NO", 20) / 2,
                screenHeight / 2 + 30, 20, YELLOW);
        }

        // ========== ОТРИСОВКА ИНТЕРФЕЙСА КАМЕР ==========
        if (fixedCameraSystem.IsActive()) {
            DrawRectangle(0, screenHeight - 80, 400, 80, (Color) { 0, 0, 0, 200 });

            const char* camName = fixedCameraSystem.GetCurrentCameraName();
            int camNameWidth = MeasureText(camName, 25);
            DrawText(camName, screenWidth / 2 - camNameWidth / 2, screenHeight - 70, 25, YELLOW);

            DrawText("Press TAB to exit camera mode", screenWidth / 2 - 150, screenHeight - 40, 18, LIGHTGRAY);
            DrawText("LEFT/RIGHT arrows to change camera", screenWidth / 2 - 170, screenHeight - 20, 16, GRAY);

            char camIndexText[32];
            snprintf(camIndexText, sizeof(camIndexText), "CAMERA %d/%d",
                fixedCameraSystem.GetCurrentIndex() + 1, FIXED_CAMERA_COUNT);
            int idxWidth = MeasureText(camIndexText, 15);
            DrawText(camIndexText, screenWidth - idxWidth - 20, screenHeight - 25, 15, GRAY);
        }

        if (!mouseCaptured) {
            DrawText("Click in window to capture mouse", screenWidth / 2 - 200, screenHeight - 30, 18, DARKGRAY);
        }
        else {
            DrawText("WASD+Mouse | Q/E | Shift/Ctrl | P-pause | ALT | F11 | O-Sheep | V-Wolf | TAB-cams | R-Reset",
                screenWidth / 2 - 450, screenHeight - 30, 18, DARKGRAY);
        }

        EndDrawing();
    }

    delete myWorld;
    CloseAudioDevice();

    EnableCursor();
    CloseWindow();
    return 0;
}