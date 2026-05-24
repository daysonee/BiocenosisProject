#include "world.hpp"
#include "../entities/animal.hpp"
#include "../entities/wolf.hpp"
#include "../entities/sheep.hpp"
#include "../entities/plant.hpp"
#include "../utils/noise.hpp"
#include <vector>
#include <rlgl.h>
#include "raymath.h"

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

    trunkMesh = GenMeshCylinder(0.15f, 0.8f, 5);
    leafBottomMesh = GenMeshCone(1.2f, 1.2f, 5);
    leafMidMesh = GenMeshCone(0.9f, 1.0f, 5);
    leafTopMesh = GenMeshCone(0.6f, 0.8f, 5);
    bushMesh = GenMeshSphere(0.8f, 6, 6); 

    trunkMat = LoadMaterialDefault(); trunkMat.maps[MATERIAL_MAP_DIFFUSE].color = BROWN;
    leafMat = LoadMaterialDefault();  leafMat.maps[MATERIAL_MAP_DIFFUSE].color = DARKGREEN;
    bushMat = LoadMaterialDefault(); bushMat.maps[MATERIAL_MAP_DIFFUSE].color = GREEN;


    int halfMap = Config::World::MAP_SIZE / 2;
    int maxTrees = Config::World::TREE_COUNT;
    int attempts = 0;
    float minTreeDist = 2.0f; 

    while (treePositions.size() < maxTrees && attempts < maxTrees * 3) {
        attempts++;
        float rx = (float)GetRandomValue(-halfMap, halfMap);
        float rz = (float)GetRandomValue(-halfMap, halfMap);
        float ry = GetHeight(rx, rz);
        
        if (ry > Config::World::SAND_LEVEL && ry <= Config::World::BIOME_THRESHOLD) {
            bool overlap = false;
            for (const auto& pos : treePositions) {
                float dx = rx - pos.x;
                float dz = rz - pos.z;
                if (dx * dx + dz * dz < minTreeDist * minTreeDist) {
                    overlap = true;
                    break;
                }
            }
            if (!overlap) {
                treePositions.push_back({rx, ry, rz}); 

                trunkTransforms.push_back(MatrixTranslate(rx, ry + 0.4f, rz));
                leafBottomTransforms.push_back(MatrixTranslate(rx, ry + 0.6f + 0.6f, rz));
                leafMidTransforms.push_back(MatrixTranslate(rx, ry + 1.4f + 0.5f, rz));
                leafTopTransforms.push_back(MatrixTranslate(rx, ry + 2.1f + 0.4f, rz));
            }
        }
    }


    for (int i = 0; i < Config::World::BUSH_COUNT; i++) {
        float rx = (float)GetRandomValue(-halfMap, halfMap);
        float rz = (float)GetRandomValue(-halfMap, halfMap);
        float ry = GetHeight(rx, rz);
        
        if (ry > Config::World::BIOME_THRESHOLD && ry <= 65.0f) {
            bushTransforms.push_back(MatrixTranslate(rx, ry + 0.2f, rz));
        }
    }

    int maxRainDrops = Config::World::RAINDROPS_COUNT; 
    for (int i = 0; i < maxRainDrops; i++) {
        rainDrops.push_back({
            (float)GetRandomValue(-halfMap, halfMap),
            (float)GetRandomValue(20, 100), 
            (float)GetRandomValue(-halfMap, halfMap)
        });
    }
    

}

World::~World(){
    UnloadModel(terrainModel);

    UnloadMesh(trunkMesh);
    UnloadMesh(leafBottomMesh);
    UnloadMesh(leafMidMesh);
    UnloadMesh(leafTopMesh);
    UnloadMesh(bushMesh);

    UnloadMaterial(trunkMat);
    UnloadMaterial(leafMat);
    UnloadMaterial(bushMat);
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
        float depthFactor = height / Config::World::WATER_LEVEL;
        if (depthFactor < 0.5f) return (Color){20, 50, 80, 255};
        return (Color){140, 120, 90, 255};                      
    }
    if (height <= Config::World::SAND_LEVEL) {
        return (Color){230, 215, 160, 255}; 
    }
    if (height <= Config::World::BIOME_THRESHOLD) {
        return (Color){45, 110, 60, 255}; 
    }
    if (height <= 65.0f) {
        return (Color){110, 160, 80, 255}; 
    }
    
    return (Color){130, 130, 130, 255}; 
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
            
            float h1 = GetHeight(x, z);
            float h2 = GetHeight(x + step, z);
            float h3 = GetHeight(x + step, z + step);
            float h4 = GetHeight(x, z + step);

            Color c1 = GetBiomeColor(h1);
            Color c2 = GetBiomeColor(h2);
            Color c3 = GetBiomeColor(h3);
            Color c4 = GetBiomeColor(h4);

            mesh.vertices[v++] = x;        mesh.vertices[v++] = h1; mesh.vertices[v++] = z;
            mesh.colors[c++] = c1.r; mesh.colors[c++] = c1.g; mesh.colors[c++] = c1.b; mesh.colors[c++] = c1.a;

            mesh.vertices[v++] = x + step; mesh.vertices[v++] = h3; mesh.vertices[v++] = z + step;
            mesh.colors[c++] = c3.r; mesh.colors[c++] = c3.g; mesh.colors[c++] = c3.b; mesh.colors[c++] = c3.a;

            mesh.vertices[v++] = x + step; mesh.vertices[v++] = h2; mesh.vertices[v++] = z;
            mesh.colors[c++] = c2.r; mesh.colors[c++] = c2.g; mesh.colors[c++] = c2.b; mesh.colors[c++] = c2.a;

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

Vector3 World::ResolveTreeCollisions(Vector3 animalPos, float animalRadius) const {
    Vector3 correctedPos = animalPos;
    float treeRadius = 0.25f; 
    float minDist = animalRadius + treeRadius;
    float minDistSq = minDist * minDist;

    for (const auto& treePos : treePositions) {
        float dx = correctedPos.x - treePos.x;
        float dz = correctedPos.z - treePos.z;
        float distSq = dx * dx + dz * dz;

        if (distSq < minDistSq) {
            float distance = sqrtf(distSq);
            if (distance == 0.0f) {
                correctedPos.x += 0.1f;
            } else {
                correctedPos.x = treePos.x + (dx / distance) * minDist;
                correctedPos.z = treePos.z + (dz / distance) * minDist;
            }
        }
    }
    return correctedPos;
}

void World::Update(float deltaTime){
    weatherTimer -= deltaTime;
    if (weatherTimer <= 0.0f){
        ChangeWeather();
    }

    int halfMap = Config::World::MAP_SIZE / 2;
    if (currentWeather == WeatherState::RAINING){
       float rainSpeed = 60.0f * deltaTime;
        for (auto& drop : rainDrops) {
            drop.y -= rainSpeed;
            drop.x -= rainSpeed * 0.1f;
            
            if (drop.y < -10.0f) {
                drop.y = (float)GetRandomValue(40, 100);
                drop.x = (float)GetRandomValue(-halfMap, halfMap);
                drop.z = (float)GetRandomValue(-halfMap, halfMap);
            }
        }

        plantSpawnTimer -= deltaTime;
        if (plantSpawnTimer <= 0){
            float rx = (float)GetRandomValue(-halfMap, halfMap);
            float rz = (float)GetRandomValue(-halfMap, halfMap);
            float ry = GetHeight(rx, rz);
            
            if (ry > Config::World::SAND_LEVEL) {
                AddEntity(std::make_unique<Plant>((Vector3){rx, ry, rz}));
                plantSpawnTimer = Config::World::PLANT_SPAWN_DELAY;
            }
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

    DrawCubeV(
        (Vector3){ 0.0f, Config::World::WATER_LEVEL / 2.0f, 0.0f }, 
        (Vector3){ (float)mapSize, Config::World::WATER_LEVEL, (float)mapSize }, 
        (Color){ 30, 120, 190, 255 } 
    );

    for (const auto& mat : bushTransforms) {
        DrawMesh(bushMesh, bushMat, mat);
    }

    for (size_t i = 0; i < trunkTransforms.size(); i++) {
        DrawMesh(trunkMesh, trunkMat, trunkTransforms[i]);
        DrawMesh(leafBottomMesh, leafMat, leafBottomTransforms[i]);
        DrawMesh(leafMidMesh, leafMat, leafMidTransforms[i]);
        DrawMesh(leafTopMesh, leafMat, leafTopTransforms[i]);
    }
  
    for (auto& entity : entities){
        if (entity->IsAlive()){
            entity->Draw(); 
        }
    }

    int halfMap = Config::World::MAP_SIZE / 2;
    if (currentWeather == WeatherState::RAINING){
       rlBegin(RL_LINES);
        rlColor4ub(140, 170, 250, 120); 
        for (const auto& drop : rainDrops) {
            rlVertex3f(drop.x, drop.y, drop.z);
            rlVertex3f(drop.x + 0.3f, drop.y + 3.0f, drop.z);
        }
        rlEnd();
    }


}

