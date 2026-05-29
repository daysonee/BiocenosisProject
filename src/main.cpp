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

// ========== СИСТЕМА КАМЕР (Tab = цикл, WASD = вернуться к FREE) ==========
#include <vector>
#include <string>

struct CamSlot {
    Vector3 position;
    Vector3 target;
    std::string name;
};

class CameraSystem {
    std::vector<CamSlot> slots; // 0 = FREE (placeholder), 1..N = fixed
    int current = 0;            // 0 = свободная камера
    Camera3D savedFreeCamera = {};
    bool hasSaved = false;
    float showPanelTimer = 0.0f; // показ панели после Tab

public:
    void Deactivate() {
        current = 0;           // Принудительно возвращаем свободную камеру
        showPanelTimer = 0.0f; // Скрываем UI-панель со списком камер
    }

    void Init(Vector3 hutPos) {
        slots.clear();
        // 0 — свободная камера (placeholder, позиция не важна)
        slots.push_back({{0, 0, 0}, {0, 0, 0}, "FREE CAMERA"});
        // 1 — обзорная
        slots.push_back({{0, 80, -150}, {0, 30, 0}, "OVERVIEW"});
        // 2 — хижина охотника
        slots.push_back({
            {hutPos.x + 18.0f, hutPos.y + 12.0f, hutPos.z + 18.0f},
            {hutPos.x, hutPos.y + 2.0f, hutPos.z},
            "HUNTER'S HUT"
        });
        // 3 — западный лес
        slots.push_back({{-200, 50, -100}, {-150, 30, -80}, "WEST FOREST"});
        // 4 — восточный луг
        slots.push_back({{200, 50, 100}, {150, 30, 80}, "EAST MEADOW"});
        // 5 — южный пляж
        slots.push_back({{-100, 40, 150}, {-80, 25, 100}, "SOUTH BEACH"});
    }

    void OnTab(Camera3D& camera) {
        // Сохраняем свободную камеру перед уходом
        if (current == 0) {
            savedFreeCamera = camera;
            hasSaved = true;
        }
        current = (current + 1) % (int)slots.size();
        if (current == 0) {
            // Вернулись к свободной — восстановить
            if (hasSaved) camera = savedFreeCamera;
        } else {
            camera.position = slots[current].position;
            camera.target   = slots[current].target;
        }
        showPanelTimer = 3.0f; // показать панель на 3 сек
    }

    // WASD при фиксированной камере — вернуться к свободной
    void CheckReturnToFree(Camera3D& camera) {
        if (current == 0) return;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D)) {
            if (hasSaved) camera = savedFreeCamera;
            current = 0;
            showPanelTimer = 2.0f;
        }
    }

    bool IsFree() const { return current == 0; }
    int GetCurrent() const { return current; }
    const char* GetCurrentName() const { return slots[current].name.c_str(); }

    void Update(float dt) {
        if (showPanelTimer > 0.0f) showPanelTimer -= dt;
    }

    void DrawPanel(int screenWidth) const {
        if (showPanelTimer <= 0.0f && current == 0) return; // скрыта

        int panelW = 200;
        int lineH  = 24;
        int panelH = (int)slots.size() * lineH + 16;
        
        // ИЗМЕНЕНО: вместо правого края привязываемся к левому
        int px = 15; 
        int py = 15;

        // Фон панели
        float alpha = (showPanelTimer > 0.0f) ? fminf(showPanelTimer, 1.0f) : 0.8f;
        DrawRectangle(px, py, panelW, panelH,
                      (Color){0, 0, 0, (unsigned char)(180 * alpha)});
        DrawRectangleLinesEx(
            (Rectangle){(float)px, (float)py, (float)panelW, (float)panelH},
            1.5f, (Color){200, 200, 200, (unsigned char)(200 * alpha)});

        for (int i = 0; i < (int)slots.size(); ++i) {
            int y = py + 8 + i * lineH;
            bool isCurrent = (i == current);
            Color textColor = isCurrent
                ? (Color){255, 220, 80, (unsigned char)(255 * alpha)}  // золотой
                : (Color){180, 180, 180, (unsigned char)(200 * alpha)};
            const char* prefix = isCurrent ? "> " : "  ";
            char line[64];
            snprintf(line, sizeof(line), "%s%d. %s", prefix, i, slots[i].name.c_str());
            DrawText(line, px + 8, y, 16, textColor);
        }
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
    Hunter* playerHunter = new Hunter(hutPos);
    myWorld->AddEntity(std::unique_ptr<Hunter>(playerHunter));
    bool hunterControlMode = false;
    bool wolfControlMode = false;
    bool sheepControlMode = false;
    Wolf* playerWolf = nullptr;
    Sheep* playerSheep = nullptr;
    float hunterYaw = 0.0f;
    float hunterPitch = 0.0f;
    float animalYaw = 0.0f;
    float animalPitch = 0.0f;

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

    CameraSystem cameraSystem;
    // Инициализируем камеры после создания мира
    cameraSystem.Init(myWorld->GetHunterHutPosition());

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

    auto FindFirstAliveWolf = [&]() -> Wolf* {
        for (const auto& entity : myWorld->GetEntities()) {
            if (!entity->IsAlive()) continue;
            Wolf* wolf = dynamic_cast<Wolf*>(entity.get());
            if (wolf) return wolf;
        }
        return nullptr;
    };

    auto FindFirstAliveSheep = [&]() -> Sheep* {
        for (const auto& entity : myWorld->GetEntities()) {
            if (!entity->IsAlive()) continue;
            Sheep* sheep = dynamic_cast<Sheep*>(entity.get());
            if (sheep) return sheep;
        }
        return nullptr;
    };

    auto ExitAllPlayerModes = [&]() {
        if (playerHunter) playerHunter->SetPlayerControlled(false);
        if (playerWolf) playerWolf->SetPlayerControlled(false);
        if (playerSheep) playerSheep->SetPlayerControlled(false);
        hunterControlMode = false;
        wolfControlMode = false;
        sheepControlMode = false;
    };

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
                    playerHunter = new Hunter(newHut);
                    newWorld->AddEntity(std::unique_ptr<Hunter>(playerHunter));
                    hunterControlMode = false;
                    wolfControlMode = false;
                    sheepControlMode = false;
                    playerWolf = nullptr;
                    playerSheep = nullptr;
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
                // Переинициализируем камеры (новая хижина)
                cameraSystem.Init(myWorld->GetHunterHutPosition());

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


        // ========== РЕЖИМЫ ОТ ПЕРВОГО ЛИЦА ==========
        if (!isRegenerating && IsKeyPressed(KEY_N) && playerHunter != nullptr) {
            bool enable = !hunterControlMode;
            ExitAllPlayerModes();
            if (enable) {
                hunterControlMode = true;
                playerHunter->SetPlayerControlled(true);
                Vector3 viewDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
                hunterYaw = atan2f(viewDir.x, viewDir.z);
                hunterPitch = asinf(Clamp(viewDir.y, -0.95f, 0.95f));
                mouseCaptured = true;
                DisableCursor();
                cameraSystem.Deactivate();
            }
        }

        if (!isRegenerating && IsKeyPressed(KEY_M)) {
            bool enable = !wolfControlMode;
            ExitAllPlayerModes();
            if (enable) {
                playerWolf = FindFirstAliveWolf();
                if (playerWolf) {
                    wolfControlMode = true;
                    playerWolf->SetPlayerControlled(true);
                    Vector3 viewDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
                    animalYaw = atan2f(viewDir.x, viewDir.z);
                    animalPitch = asinf(Clamp(viewDir.y, -0.95f, 0.95f));
                    mouseCaptured = true;
                    DisableCursor();
                    cameraSystem.Deactivate();
                }
            }
        }

        if (!isRegenerating && IsKeyPressed(KEY_G)) {
            bool enable = !sheepControlMode;
            ExitAllPlayerModes();
            if (enable) {
                playerSheep = FindFirstAliveSheep();
                if (playerSheep) {
                    sheepControlMode = true;
                    playerSheep->SetPlayerControlled(true);
                    Vector3 viewDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
                    animalYaw = atan2f(viewDir.x, viewDir.z);
                    animalPitch = asinf(Clamp(viewDir.y, -0.95f, 0.95f));
                    mouseCaptured = true;
                    DisableCursor();
                    cameraSystem.Deactivate();
                }
            }
        }

        // ========== СИСТЕМА КАМЕР (Tab = цикл) ==========
        cameraSystem.Update(dt);

        if (IsKeyPressed(KEY_TAB)) {
            // 1. Выходим из режимов управления (чтобы избежать конфликтов координат)
            ExitAllPlayerModes();

            // 2. ВЫЗЫВАЕМ само переключение камер (меняет индекс и координаты)
            cameraSystem.OnTab(camera);

            // 3. Управляем захватом мыши в зависимости от того, куда переключились
            if (cameraSystem.IsFree()) {
                mouseCaptured = true;   // Вернулись к свободной - прячем мышь
                DisableCursor();
            } else {
                mouseCaptured = false;  // Фиксированная камера - показываем мышь
                EnableCursor();
            }
        }

        // WASD при фиксированной камере → вернуться к FREE
        if (!cameraSystem.IsFree()) {
            cameraSystem.CheckReturnToFree(camera);
            if (cameraSystem.IsFree()) {
                mouseCaptured = true;
                DisableCursor();
            }
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

        if (hunterControlMode && playerHunter != nullptr && playerHunter->IsAlive()) {
            Vector2 mouseDelta = GetMouseDelta();
            hunterYaw -= mouseDelta.x * 0.0035f;
            hunterPitch -= mouseDelta.y * 0.0035f;
            hunterPitch = Clamp(hunterPitch, -1.2f, 1.2f);

            Vector3 forward = {
                sinf(hunterYaw) * cosf(hunterPitch),
                sinf(hunterPitch),
                cosf(hunterYaw) * cosf(hunterPitch)
            };
            forward = Vector3Normalize(forward);
            Vector3 right = { cosf(hunterYaw), 0.0f, -sinf(hunterYaw) };

            Vector3 hp = playerHunter->GetPosition();
            camera.position = { hp.x + forward.x * 0.85f, hp.y + 1.82f + forward.y * 0.10f, hp.z + forward.z * 0.85f };
            camera.target = Vector3Add(camera.position, forward);
            playerHunter->SetPlayerAim(forward, right, camera.position);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE)) {
                playerHunter->RequestShoot();
            }
        } else if (wolfControlMode && playerWolf != nullptr && playerWolf->IsAlive()) {
            Vector2 mouseDelta = GetMouseDelta();
            animalYaw -= mouseDelta.x * 0.0035f;
            animalPitch -= mouseDelta.y * 0.0035f;
            animalPitch = Clamp(animalPitch, -1.0f, 1.0f);

            Vector3 forward = {
                sinf(animalYaw) * cosf(animalPitch),
                sinf(animalPitch),
                cosf(animalYaw) * cosf(animalPitch)
            };
            forward = Vector3Normalize(forward);
            Vector3 right = { cosf(animalYaw), 0.0f, -sinf(animalYaw) };

            Vector3 wp = playerWolf->GetPosition();
            camera.position = { wp.x + forward.x * 0.80f, wp.y + 0.95f, wp.z + forward.z * 0.80f };
            camera.target = Vector3Add(camera.position, forward);
            playerWolf->SetPlayerAim(forward, right);
        } else if (sheepControlMode && playerSheep != nullptr && playerSheep->IsAlive()) {
            Vector2 mouseDelta = GetMouseDelta();
            animalYaw += mouseDelta.x * 0.0035f;
            animalPitch -= mouseDelta.y * 0.0035f;
            animalPitch = Clamp(animalPitch, -1.0f, 1.0f);

            Vector3 forward = {
                sinf(animalYaw) * cosf(animalPitch),
                sinf(animalPitch),
                cosf(animalYaw) * cosf(animalPitch)
            };
            forward = Vector3Normalize(forward);
            Vector3 right = { cosf(animalYaw), 0.0f, -sinf(animalYaw) };

            Vector3 sp = playerSheep->GetPosition();
            camera.position = { sp.x + forward.x * 0.70f, sp.y + 0.95f, sp.z + forward.z * 0.70f };
            camera.target = Vector3Add(camera.position, forward);
            playerSheep->SetPlayerAim(forward, right);
        } else if (mouseCaptured && cameraSystem.IsFree()) {
            cameraController.Update(dt, camera, true);
        }

        if (!hunterControlMode && !wolfControlMode && !sheepControlMode && cameraSystem.IsFree()) {
            cameraSpawn.Update(dt, camera);
        }

        if (cameraSpawn.IsAnimationFinished() && !hunterControlMode && !wolfControlMode && !sheepControlMode) {
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

        if (hunterControlMode) {
            DrawRectangle(20, 70, 460, 82, (Color){0, 0, 0, 170});
            DrawText("HUNTER FPS MODE", 35, 82, 22, GOLD);
            DrawText("Mouse - aim | WASD - move | Shift - run", 35, 108, 18, WHITE);
            DrawText("LMB/SPACE - shoot wolves and sheep | N - exit", 35, 130, 18, WHITE);
            DrawCircleLines(screenWidth / 2, screenHeight / 2, 8.0f, WHITE);
        }

        if (wolfControlMode) {
            DrawRectangle(20, 70, 460, 82, (Color){0, 0, 0, 170});
            DrawText("WOLF FPS MODE", 35, 82, 22, RED);
            DrawText("Mouse - look | WASD - move | Shift - run", 35, 108, 18, WHITE);
            DrawText("LMB/SPACE - bite nearest sheep | M - exit", 35, 130, 18, WHITE);
            DrawCircleLines(screenWidth / 2, screenHeight / 2, 8.0f, WHITE);
        }

        if (sheepControlMode) {
            DrawRectangle(20, 70, 460, 82, (Color){0, 0, 0, 170});
            DrawText("SHEEP FPS MODE", 35, 82, 22, LIGHTGRAY);
            DrawText("Mouse - look | WASD - move | Shift - run", 35, 108, 18, WHITE);
            DrawText("G - exit", 35, 130, 18, WHITE);
            DrawCircleLines(screenWidth / 2, screenHeight / 2, 8.0f, WHITE);
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

        // ========== МИНИ-ПАНЕЛЬ КАМЕР ==========
        cameraSystem.DrawPanel(screenWidth);
        // Подсказка текущей камеры внизу (если не свободная)
        if (!cameraSystem.IsFree()) {
            const char* camName = cameraSystem.GetCurrentName();
            int camNameWidth = MeasureText(camName, 22);
            DrawRectangle(screenWidth / 2 - camNameWidth / 2 - 12,
                          screenHeight - 45, camNameWidth + 24, 32,
                          (Color){0, 0, 0, 180});
            DrawText(camName, screenWidth / 2 - camNameWidth / 2,
                     screenHeight - 40, 22, YELLOW);
        }

        if (!mouseCaptured) {
            DrawText("Click in window to capture mouse", screenWidth / 2 - 200, screenHeight - 30, 18, DARKGRAY);
        }
        else {
            DrawText("WASD+Mouse | Q/E | Shift/Ctrl | P-pause | ALT | F11 | O-Sheep | V-Wolf | TAB-cams | R-Reset | N-Hunter | M-Wolf | G-Sheep",
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