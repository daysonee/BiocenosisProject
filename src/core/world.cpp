#include "world.hpp"
#include "../entities/animal.hpp"
#include "../entities/wolf.hpp"
#include "../entities/sheep.hpp"
#include "../entities/plant.hpp"
#include "../utils/noise.hpp"
#include <string>

World::World(){
    plantSpawnTimer = PLANT_SPAWN_DELAY;
    plantCount = 0;
    sheepCount = 0;
    wolfCount = 0;

    currentWeather = WeatherState::SUNNY;
    weatherTimer = (float)GetRandomValue(10, 15);
}

void World::ChangeWeather(){
    if (currentWeather == WeatherState::SUNNY){
        currentWeather = WeatherState::RAINING;
        weatherTimer = (float)GetRandomValue(10, 15);
    } else{
        currentWeather = WeatherState::SUNNY;
        weatherTimer = (float)GetRandomValue(10, 15);
    }
}

float World::GetHeight(float x, float z) const{
    float scale = 0.1f;
    float amplitude = 3.5f;

    float noiseVal = PerlinNoise::Noise2D(x * scale, z * scale);
    return noiseVal * amplitude;

}

void World::AddEntity(std::unique_ptr<Entity> entity){
    entities.push_back(std::move(entity));
}

void World::Update(float deltaTime){
    weatherTimer -= deltaTime;
    if (weatherTimer <= 0.0f){
        ChangeWeather();
    }

    if (currentWeather == WeatherState::RAINING){
        plantSpawnTimer -= deltaTime;
        if (plantSpawnTimer <= 0){
            float rx = (float)GetRandomValue(-15, 15);
            float rz = (float)GetRandomValue(-15, 15);
            float ry = GetHeight(rx, rz);
            AddEntity(std::make_unique<Plant>((Vector3){rx, ry, rz}));

            plantSpawnTimer = PLANT_SPAWN_DELAY;
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
    for (int x = -mapSize/2; x < mapSize/2; ++x){
        for (int z = -mapSize/2; z < mapSize/2; ++z){
            Vector3 p1 = (Vector3){(float)x, GetHeight((float)x, (float)z), (float)z};
            Vector3 p2 = (Vector3){(float)x+1.0f, GetHeight((float)x+1.0f, (float)z), (float)z};
            Vector3 p3 = (Vector3){(float)x+1.0f, GetHeight((float)x+1.0f, (float)z+1.0f), (float)z+1.0f};
            Vector3 p4 = (Vector3){(float)x, GetHeight((float)x, float(z)+1.0f), (float)z+1.0f};

            Color terrainColor = ((x + z) % 2 == 0) ? (Color){34, 139, 34, 255} : (Color){46, 139, 87, 255};
            DrawTriangle3D(p1, p3, p2, terrainColor);
            DrawTriangle3D(p1, p4, p3, terrainColor);

            DrawLine3D(p1, p2, (Color){0, 100, 0, 50});
            DrawLine3D(p1, p4, (Color){0, 100, 0, 50});
        }
    }


    for (auto& entity : entities){
        if (entity->IsAlive()){
            entity->Draw(); 
        }
    }

    if (currentWeather == WeatherState::RAINING){
        for(int i = 0; i < 30; ++i){
            float rx = (float)GetRandomValue(-15, 15);
            float rz = (float)GetRandomValue(-15, 15);
            float surfaceY = GetHeight(rx, rz);
            float ry = (float)GetRandomValue(surfaceY, surfaceY+10.0f);
            Vector3 startPos = (Vector3){rx, ry, rz};
            Vector3 endPos = (Vector3){rx, ry-0.8f, rz};

            DrawLine3D(startPos, endPos, BLUE);
        }
    }

    std::string plantsStr = "Plants: " + std::to_string(plantCount);
    std::string sheepStr = "Sheep: " + std::to_string(sheepCount);
    std::string wolvesStr = "Wolves: " + std::to_string(wolfCount);
    
    std::string weatherStr = (currentWeather == WeatherState::SUNNY) ? "WEATHER: SUNNY" : "WEATHER: RAINING";
    Color weatherColor = (currentWeather == WeatherState::SUNNY) ? ORANGE : BLUE;

    DrawRectangle(10, 10, 220, 120, Fade(BLACK, 0.6f));
    DrawText("ECOSYSTEM STATS:", 20, 20, 16, GREEN);
    DrawText(weatherStr.c_str(), 20, 45, 14, weatherColor);
    DrawText(plantsStr.c_str(), 20, 70, 14, WHITE);
    DrawText(sheepStr.c_str(), 20, 85, 14, WHITE);
    DrawText(wolvesStr.c_str(), 20, 100, 14, WHITE);
}