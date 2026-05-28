#pragma once

#include <vector>
#include <memory>
#include "entity.hpp"
#include "constants.hpp"
#include "raylib.h"

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
    Sound zhelezinHalyava;
    Sound zhelezinLaviKontest;
    Sound zhelezinNePonayal;
    Sound zhelezinOnulenie;
    Sound zhelezinTiOpozdal;
    Sound zhelezinTiUmer;

    Sound sheep1;
    Sound sheep2;
    Sound sheep3;

    Sound sunny;
    Sound rainning;

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
        float              regrowTimer = 0.0f; 
        bool               reserved = false; // секунд до возрождения после поедания
    };
    std::vector<GrassPatch> grassPatches;

    float plantSpawnTimer;

    WeatherState currentWeather;
    float weatherTimer;
    std::vector<Vector3> rainDrops;

    int plantCount, sheepCount, wolfCount;

    // ── ПРИЛИВЫ ──────────────────────────────────────────────────
    // Во время дождя вода поднимается за 30 сек,
    // после окончания дождя возвращается за 60 сек.
    float waterLevelOffset = 0.0f;   // текущее смещение от базового WATER_LEVEL
    float tideSourceOffset = 0.0f;
    float tideTargetOffset = 0.0f;
    float tideElapsed      = 0.0f;
    float tideDuration     = 1.0f;   // 30 для прилива, 60 для отлива

    void ChangeWeather();
    void GenerateTerrainMesh();

    const int mapSize = Config::World::MAP_SIZE;
    float offsetX, offsetZ;

    Vector3 hunterHutPosition;
public:

    void PlayZhelezinHalyava() const;
    void PlayZhelezinLaviKontest() const;
    void PlayZhelezinNePonayal() const;
    void PlayZhelezinOnulenie() const;
    void PlayZhelezinTiOpozdal() const;
    void PlayZhelezinTiUmer() const;

    void PlaySheep1() const;
    void PlaySheep2() const;
    void PlaySheep3() const;

    void PlaySunny() const;
    void PlayRainning() const;

    Vector3 GetHunterHutPosition() const { return hunterHutPosition; }
    
    World();
    ~World();

    Vector3 ResolveTreeCollisions(Vector3 animalPos, float animalRadius) const;


    int  FindNearestGrass(Vector3 from, float maxRadius,
                          Vector3& outPos, Config::Grass::Type& outType) const;
    float EatGrass(int index);

    void SetGrassReserved(int index, bool reserved);

    float GetHeight(float x, float z) const;
    Color GetBiomeColor(float height) const;

    // Текущий уровень воды с учётом приливов
    float GetCurrentWaterLevel() const { return Config::World::WATER_LEVEL + waterLevelOffset; }

    void SpawnParticles(Vector3 pos, Color color, int count, bool isHeart);
    void UpdateParticles(float deltaTime);

    void AddEntity(std::unique_ptr<Entity> entity);
    void QueueEntity(std::unique_ptr<Entity> entity);

    void Update(float deltaTime);
    void Draw();
    int GetGrassCount() const;
    const std::vector<Vector3>& GetTreePositions() const { return treePositions; }
    const std::vector<std::unique_ptr<Entity>>& GetEntities() const { return entities; }

    void Draw2D(Camera camera);
};