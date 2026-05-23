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
    if (height <= Config::World::WATER_LEVEL) {
        return (Color){30, 144, 255, 255}; // вода
    } else if (height <= Config::World::SAND_LEVEL) {
        return (Color){238, 214, 175, 255}; // песок 
    } else if (height < Config::World::BIOME_THRESHOLD) {
        return (Color){34, 139, 34, 255};  // лес
    } else {
        return (Color){124, 252, 0, 255};  // луга
    }
}

void World::GenerateTerrainMesh() {
    float step = Config::World::MESH_DENSITY;
    int cells = (int)(mapSize / step);
    int vertexCount = cells * cells * 6; 

    Mesh mesh = { 0 };
    mesh.vertexCount = vertexCount;
    mesh.triangleCount = cells * cells * 2;
    
    mesh.vertices = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.colors = (unsigned char *)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char));

    int v = 0; 
    int c = 0; 
    float limit = mapSize / 2.0f;

    for (float x = -limit; x < limit; x += step) {
        for (float z = -limit; z < limit; z += step) {
            float h1 = GetHeight(x, z);
            float h2 = GetHeight(x + step, z);
            float h3 = GetHeight(x + step, z + step);
            float h4 = GetHeight(x, z + step);

            Color c1 = GetBiomeColor(h1);
            Color c2 = GetBiomeColor(h2);
            Color c3 = GetBiomeColor(h3);
            Color c4 = GetBiomeColor(h4);

            float drawH1 = (h1 < Config::World::WATER_LEVEL) ? Config::World::WATER_LEVEL : h1;
            float drawH2 = (h2 < Config::World::WATER_LEVEL) ? Config::World::WATER_LEVEL : h2;
            float drawH3 = (h3 < Config::World::WATER_LEVEL) ? Config::World::WATER_LEVEL : h3;
            float drawH4 = (h4 < Config::World::WATER_LEVEL) ? Config::World::WATER_LEVEL : h4;

            mesh.vertices[v++] = x; mesh.vertices[v++] = drawH1; mesh.vertices[v++] = z;
            mesh.colors[c++] = c1.r; mesh.colors[c++] = c1.g; mesh.colors[c++] = c1.b; mesh.colors[c++] = c1.a;
            
            mesh.vertices[v++] = x + step; mesh.vertices[v++] = drawH3; mesh.vertices[v++] = z + step;
            mesh.colors[c++] = c3.r; mesh.colors[c++] = c3.g; mesh.colors[c++] = c3.b; mesh.colors[c++] = c3.a;
            
            mesh.vertices[v++] = x + step; mesh.vertices[v++] = drawH2; mesh.vertices[v++] = z;
            mesh.colors[c++] = c2.r; mesh.colors[c++] = c2.g; mesh.colors[c++] = c2.b; mesh.colors[c++] = c2.a;


            mesh.vertices[v++] = x; mesh.vertices[v++] = drawH1; mesh.vertices[v++] = z;
            mesh.colors[c++] = c1.r; mesh.colors[c++] = c1.g; mesh.colors[c++] = c1.b; mesh.colors[c++] = c1.a;
            
            mesh.vertices[v++] = x; mesh.vertices[v++] = drawH4; mesh.vertices[v++] = z + step;
            mesh.colors[c++] = c4.r; mesh.colors[c++] = c4.g; mesh.colors[c++] = c4.b; mesh.colors[c++] = c4.a;
            
            mesh.vertices[v++] = x + step; mesh.vertices[v++] = drawH3; mesh.vertices[v++] = z + step;
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
