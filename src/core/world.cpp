#include "world.hpp"
#include "../entities/animal.hpp"
#include "../entities/wolf.hpp"
#include "../entities/sheep.hpp"
#include "../entities/plant.hpp"
#include "../utils/noise.hpp"
#include <string>
#include <rlgl.h>

World::World(){

    plantSpawnTimer = Config::World::PLANT_SPAWN_DELAY;
    plantCount = 0;
    sheepCount = 0;
    wolfCount = 0;

    offsetX = (float)GetRandomValue(-5000, 5000); //рандомный сид
    offsetZ = (float)GetRandomValue(-5000, 5000);
    GenerateTerrainMesh();

    currentWeather = WeatherState::SUNNY;
    weatherTimer = (float)GetRandomValue(Config::World::WEATHER_SUNNY_MIN, Config::World::WEATHER_SUNNY_MAX);
}

World::~World(){
    UnloadModel(terrainModel);
}

void World::ChangeWeather(){
    if (currentWeather == WeatherState::SUNNY){
        currentWeather = WeatherState::RAINING;
        weatherTimer = (float)GetRandomValue(Config::World::WEATHER_RAIN_MIN, Config::World::WEATHER_RAIN_MAX);
    } else{
        currentWeather = WeatherState::SUNNY;
        weatherTimer = (float)GetRandomValue(Config::World::WEATHER_SUNNY_MIN, Config::World::WEATHER_SUNNY_MAX);
    }
}

float World::GetHeight(float x, float z) const {
    float amplitude = Config::World::PERLIN_AMPLITUDE;
    float frequency = Config::World::PERLIN_SCALE;
    float noiseVal = 0.0f;
    float maxVal = 0.0f; 

    for (int i = 0; i < Config::World::OCTAVES; i++) {
        noiseVal += PerlinNoise::Noise2D((x + offsetX) * frequency, (z + offsetZ) * frequency) * amplitude;
        maxVal += amplitude;
        amplitude *= Config::World::PERSISTENCE;
        frequency *= Config::World::LACUNARITY;
    }

    noiseVal /= maxVal;

    float normalizedNoise = (noiseVal + 1.0f) * 0.5f;
    return pow(normalizedNoise, Config::World::PERLIN_EXPONENT) * Config::World::PERLIN_AMPLITUDE;
}

Color World::GetBiomeColor(float height) const {
    // Вода и дно океана
    if (height <= Config::World::WATER_LEVEL) {
        // Чем глубже дно, тем темнее песок/грязь под водой
        float depthFactor = height / Config::World::WATER_LEVEL;
        if (depthFactor < 0.5f) return (Color){20, 50, 80, 255}; // Глубокое океаническое дно
        return (Color){140, 120, 90, 255};                      // Мелководное дно
    }
    
    // Пляж
    if (height <= Config::World::SAND_LEVEL) {
        return (Color){230, 215, 160, 255}; // Приятный желтый песок
    }
    
    // Равнинные леса и тайга
    if (height <= Config::World::BIOME_THRESHOLD) {
        return (Color){45, 110, 60, 255}; // Густой зеленый цвет леса
    }
    
    // Горные луга
    if (height <= 65.0f) {
        return (Color){110, 160, 80, 255}; // Светло-зеленый цвет высокогорных лугов
    }
    
    // Каменистые вершины горы
    return (Color){130, 130, 130, 255}; // Серые скалы для самых высоких пиков
}

void World::GenerateTerrainMesh() {
    float step = Config::World::MESH_DENSITY;
    float limit = mapSize / 2.0f;

    int cells = (int)(mapSize / step);
    int vertexCount = cells * cells * 6;

    Mesh mesh = { 0 };
    mesh.vertexCount = vertexCount;
    mesh.triangleCount = cells * cells * 2;

    mesh.vertices = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.colors = (unsigned char *)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char));

    int v = 0;
    int c = 0;

    for (float x = -limit; x < limit; x += step) {
        for (float z = -limit; z < limit; z += step) {
            
            // ВАЖНО: Получаем ЧЕСТНУЮ высоту шума без всяких обрезаний и зажатий!
            float h1 = GetHeight(x, z);
            float h2 = GetHeight(x + step, z);
            float h3 = GetHeight(x + step, z + step);
            float h4 = GetHeight(x, z + step);

            // Получаем цвета биомов на основе честной высоты
            Color c1 = GetBiomeColor(h1);
            Color c2 = GetBiomeColor(h2);
            Color c3 = GetBiomeColor(h3);
            Color c4 = GetBiomeColor(h4);

            // Треугольник 1
            mesh.vertices[v++] = x;        mesh.vertices[v++] = h1; mesh.vertices[v++] = z;
            mesh.colors[c++] = c1.r; mesh.colors[c++] = c1.g; mesh.colors[c++] = c1.b; mesh.colors[c++] = c1.a;

            mesh.vertices[v++] = x + step; mesh.vertices[v++] = h3; mesh.vertices[v++] = z + step;
            mesh.colors[c++] = c3.r; mesh.colors[c++] = c3.g; mesh.colors[c++] = c3.b; mesh.colors[c++] = c3.a;

            mesh.vertices[v++] = x + step; mesh.vertices[v++] = h2; mesh.vertices[v++] = z;
            mesh.colors[c++] = c2.r; mesh.colors[c++] = c2.g; mesh.colors[c++] = c2.b; mesh.colors[c++] = c2.a;

            // Треугольник 2
            mesh.vertices[v++] = x;        mesh.vertices[v++] = h1; mesh.vertices[v++] = z;
            mesh.colors[c++] = c1.r; mesh.colors[c++] = c1.g; mesh.colors[c++] = c1.b; mesh.colors[c++] = c1.a;

            mesh.vertices[v++] = x;        mesh.vertices[v++] = h4; mesh.vertices[v++] = z + step;
            mesh.colors[c++] = c4.r; mesh.colors[c++] = c4.g; mesh.colors[c++] = c4.b; mesh.colors[c++] = c4.a;

            mesh.vertices[v++] = x + step; mesh.vertices[v++] = h3; mesh.vertices[v++] = z + step;
            mesh.colors[c++] = c3.r; mesh.colors[c++] = c3.g; mesh.colors[c++] = c3.b; mesh.colors[c++] = c3.a;
        }
    }

    UploadMesh(&mesh, false);
    terrainModel = LoadModelFromMesh(mesh);
}

void World::AddEntity(std::unique_ptr<Entity> entity){
    entities.push_back(std::move(entity));
}

void World::Update(float deltaTime){
    weatherTimer -= deltaTime;
    if (weatherTimer <= 0.0f){
        ChangeWeather();
    }

    int halfMap = Config::World::MAP_SIZE / 2;
    if (currentWeather == WeatherState::RAINING){
        plantSpawnTimer -= deltaTime;
        if (plantSpawnTimer <= 0){
            float rx = (float)GetRandomValue(-halfMap, halfMap);
            float rz = (float)GetRandomValue(-halfMap, halfMap);
            float ry = GetHeight(rx, rz);
            AddEntity(std::make_unique<Plant>((Vector3){rx, ry, rz}));

            plantSpawnTimer = Config::World::PLANT_SPAWN_DELAY;
        }
    }



    plantCount = 0;
    wolfCount = 0;
    sheepCount = 0;

    for (auto& entity : entities){
        if (entity -> IsAlive()){
            entity->Update(deltaTime, this);

            if(dynamic_cast<Plant*>(entity.get())) plantCount++;
            if(dynamic_cast<Sheep*>(entity.get())) sheepCount++;
            if(dynamic_cast<Wolf*>(entity.get())) wolfCount++;
        }
    }
}

void World::Draw(){
DrawModel(terrainModel, Vector3(), 1.0f, WHITE);

    // ОТРИСОВКА ВОДНОЙ ГЛАДИ поверх низин
    // Используем DrawCube, чтобы создать красивый плоский океан по всей карте
    DrawCubeV(
        (Vector3){ 0.0f, Config::World::WATER_LEVEL / 2.0f, 0.0f }, 
        (Vector3){ (float)mapSize, Config::World::WATER_LEVEL, (float)mapSize }, 
        (Color){ 30, 120, 190, 255 } // Синий океан. Пока без прозрачности, чтобы не нагружать код
    );

    // Отрисовка сущностей (остается без изменений)
    for (auto& entity : entities){
        if (entity->IsAlive()){
            entity->Draw(); 
        }
    }
    int halfMap = Config::World::MAP_SIZE / 2;
    if (currentWeather == WeatherState::RAINING){
        for(int i = 0; i < 30; ++i){
            float rx = (float)GetRandomValue(-halfMap, halfMap);
            float rz = (float)GetRandomValue(-halfMap, halfMap);
            float surfaceY = GetHeight(rx, rz);
            float ry = (float)GetRandomValue(surfaceY, surfaceY+10.0f);
            Vector3 startPos = (Vector3){rx, ry, rz};
            Vector3 endPos = (Vector3){rx, ry-0.8f, rz};

            DrawLine3D(startPos, endPos, BLUE);
        }
    }

}
