#pragma once

#include <vector>
#include <memory>
#include "entity.hpp"
#include "constants.hpp"

enum class WeatherState {
    SUNNY,
    RAINING
};

struct Particle {
    Vector3 position;
    Vector3 velocity;
    Color   color;
    float   lifetime;
    float   maxLifetime; 
    bool    isHeart;
};

class World {
private:
    std::vector<Particle>               particles;
    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<Entity>> pendingEntities;

    Model terrainModel;

    std::vector<Vector3> treePositions;
    std::vector<Vector3> grassPositions;

    Mesh trunkMesh, leafBottomMesh, leafMidMesh, leafTopMesh;
    Material trunkMat, leafMat;

    std::vector<Matrix> trunkTransforms;
    std::vector<Matrix> leafBottomTransforms;
    std::vector<Matrix> leafMidTransforms;
    std::vector<Matrix> leafTopTransforms;

    // ── Три вида травы ────────────────────────────────────────────────
    Mesh     meadowMesh,  fernMesh,  coastalMesh;
    Material meadowMat,   fernMat,   coastalMat;

    struct GrassPatch {
        Vector3            position;
        Config::Grass::Type type;
        bool               alive;
    };
    std::vector<GrassPatch> grassPatches;

    float plantSpawnTimer;

    WeatherState currentWeather;
    float weatherTimer;
    std::vector<Vector3> rainDrops;

    int plantCount, sheepCount, wolfCount;

    void ChangeWeather();
    void GenerateTerrainMesh();

    const int mapSize = Config::World::MAP_SIZE;
    float offsetX, offsetZ;

    Vector3 hunterHutPosition;
public:
    World();
    ~World();

    Vector3 ResolveTreeCollisions(Vector3 animalPos, float animalRadius) const;


    int  FindNearestGrass(Vector3 from, float maxRadius,
                          Vector3& outPos, Config::Grass::Type& outType) const;
    float EatGrass(int index);

    float GetHeight(float x, float z) const;
    Color GetBiomeColor(float height) const;

    void SpawnParticles(Vector3 pos, Color color, int count, bool isHeart);
    void UpdateParticles(float deltaTime);

    void AddEntity(std::unique_ptr<Entity> entity);
    void QueueEntity(std::unique_ptr<Entity> entity);

    void Update(float deltaTime);
    void Draw();

    const std::vector<Vector3>& GetTreePositions() const { return treePositions; }
    const std::vector<std::unique_ptr<Entity>>& GetEntities() const { return entities; }

    void Draw2D(Camera camera);
};