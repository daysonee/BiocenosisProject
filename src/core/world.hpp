#pragma once

#include <vector>
#include <memory>
#include "entity.hpp"

enum class WeatherState {
    SUNNY,
    RAINING
};

class World{
private:
    std::vector<std::unique_ptr<Entity>> entities;

    float plantSpawnTimer;
    const float PLANT_SPAWN_DELAY = 1.5f;

    WeatherState currentWeather;
    float weatherTimer;

    int plantCount;
    int sheepCount;
    int wolfCount;

    void ChangeWeather();

    const int mapSize = 40;

public:
    World();
    ~World() = default;

    float GetHeight(float x, float z) const;

    void AddEntity(std::unique_ptr<Entity> entity);

    void Update(float deltaTime);
    void Draw();

    const std::vector<std::unique_ptr<Entity>>& GetEntities() const {return entities;}
};