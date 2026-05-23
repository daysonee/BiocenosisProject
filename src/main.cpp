#include "raylib.h"
#include "core/world.hpp"
#include "entities/plant.hpp"
#include "entities/animal.hpp"
#include "entities/sheep.hpp"
#include "entities/wolf.hpp"
#include <memory>

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

    // === НАЧАЛЬНЫЙ СПАВН ОВЕЧЕК У ВОДОЁМОВ ===
    int halfMap = Config::World::MAP_SIZE / 2;
    int spawnedSheep = 0;

    while (spawnedSheep < Config::Sheep::INITIAL_COUNT) {
        // Выбираем случайную точку на карте
        float rx = (float)GetRandomValue(-halfMap, halfMap);
        float rz = (float)GetRandomValue(-halfMap, halfMap);
        float ry = myWorld.GetHeight(rx, rz);

        // Проверяем, находится ли точка у воды:
        // Она должна быть выше уровня воды (чтобы не спавниться под водой)
        // и не выше песчаной/прибрежной зоны (например, до высоты 1.2f)
        if (ry > Config::World::WATER_LEVEL && ry <= Config::World::SAND_LEVEL + 0.4f) {
            myWorld.AddEntity(std::make_unique<Sheep>((Vector3){ rx, ry, rz }));
            spawnedSheep++;
        }
    }

    myWorld.AddEntity(std::make_unique<Wolf>((Vector3){ 10.0f, 0.0f, 10.0f }));

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