#pragma once

#include <vector>
#include <memory>
#include "entity.hpp"
#include "constants.hpp"

enum class WeatherState {
    SUNNY,
    RAINING
};

class World{
private:
    std::vector<std::unique_ptr<Entity>> entities;
    Model terrainModel;

    float plantSpawnTimer;

    WeatherState currentWeather;
    float weatherTimer;

    int plantCount;
    int sheepCount;
    int wolfCount;

    void ChangeWeather();
    void GenerateTerrainMesh();

    const int mapSize = Config::World::MAP_SIZE;

    float offsetX;
    float offsetZ;

public:
    World();
    ~World();

    float GetHeight(float x, float z) const;
    Color GetBiomeColor(float height) const;
    
    void AddEntity(std::unique_ptr<Entity> entity);

    void Update(float deltaTime);
    void Draw();

    const std::vector<std::unique_ptr<Entity>>& GetEntities() const {return entities;}
};