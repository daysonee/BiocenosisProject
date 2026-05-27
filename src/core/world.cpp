#pragma warning(disable: 4576)
#include "world.hpp"
#include "../entities/animal.hpp"
#include "../entities/wolf.hpp"
#include "../entities/sheep.hpp"
#include "../entities/plant.hpp"
#include "../entities/carcass.hpp"
#include "../utils/noise.hpp"
#include <vector>
#include <rlgl.h>
#include "raymath.h"
#include <algorithm>

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

    // Три вида травы — разные размеры и формы
    meadowMesh  = GenMeshSphere(0.35f, 6, 6);   // невысокие кочки
    fernMesh    = GenMeshCone(0.45f, 1.1f, 5);  // высокие папоротники
    coastalMesh = GenMeshSphere(0.5f,  6, 6);   // прибрежные пучки

    trunkMat = LoadMaterialDefault(); trunkMat.maps[MATERIAL_MAP_DIFFUSE].color = BROWN;
    leafMat  = LoadMaterialDefault(); leafMat.maps[MATERIAL_MAP_DIFFUSE].color  = DARKGREEN;

    meadowMat  = LoadMaterialDefault(); meadowMat.maps[MATERIAL_MAP_DIFFUSE].color  = (Color){120, 200, 80,  255}; // ярко-зелёный
    fernMat    = LoadMaterialDefault(); fernMat.maps[MATERIAL_MAP_DIFFUSE].color    = (Color){40,  90,  45,  255}; // тёмно-зелёный
    coastalMat = LoadMaterialDefault(); coastalMat.maps[MATERIAL_MAP_DIFFUSE].color = (Color){170, 200, 110, 255}; // желтовато-зелёный


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

                // ФИКС ЛЕВИТАЦИИ: основание ствола опускаем на 0.2 ниже
                // средней высоты, чтобы при наклоне поверхности корни
                // не висели в воздухе. GenMeshCylinder строится вверх от точки.
                const float TRUNK_SINK   = 0.2f;
                const float TRUNK_HEIGHT = 0.8f; // совпадает с GenMeshCylinder
                float trunkBaseY = ry - TRUNK_SINK;
                float trunkTopY  = trunkBaseY + TRUNK_HEIGHT;

                trunkTransforms.push_back(MatrixTranslate(rx, trunkBaseY, rz));
                leafBottomTransforms.push_back(MatrixTranslate(rx, trunkTopY + 0.0f, rz));
                leafMidTransforms.push_back   (MatrixTranslate(rx, trunkTopY + 0.7f, rz));
                leafTopTransforms.push_back   (MatrixTranslate(rx, trunkTopY + 1.4f, rz));
            }
        }
    }


    // ── СПАВН ТРЁХ ВИДОВ ТРАВЫ ────────────────────────────────────────
    auto spawnGrass = [&](int count, float minH, float maxH,
                          Config::Grass::Type type) {
        int attempts = 0;
        int spawned  = 0;
        while (spawned < count && attempts < count * 10) {
            ++attempts;
            float rx = (float)GetRandomValue(-halfMap, halfMap);
            float rz = (float)GetRandomValue(-halfMap, halfMap);
            float ry = GetHeight(rx, rz);
            if (ry > minH && ry <= maxH) {
                GrassPatch p;
                p.position = { rx, ry, rz };
                p.type     = type;
                p.alive    = true;
                grassPatches.push_back(p);
                ++spawned;
            }
        }
    };

    spawnGrass(Config::Grass::MEADOW_COUNT,
               Config::Grass::MEADOW_MIN,  Config::Grass::MEADOW_MAX,
               Config::Grass::Type::MEADOW);
    spawnGrass(Config::Grass::FERN_COUNT,
               Config::Grass::FERN_MIN,    Config::Grass::FERN_MAX,
               Config::Grass::Type::FERN);
    spawnGrass(Config::Grass::COASTAL_COUNT,
               Config::Grass::COASTAL_MIN, Config::Grass::COASTAL_MAX,
               Config::Grass::Type::COASTAL);


    // ─── ГАРАНТИРОВАННЫЙ СПАВН ДОМИКА (ГОРЫ -> ЛЕС) ───
    bool hutSpawned = false;
    // Убрали "const int", так как halfMap уже объявлен в начале конструктора!

    // Попытка 1: Ищем высокое место в горах (выше BIOME_THRESHOLD)
    for (int attempts = 0; attempts < 1500; ++attempts) {
        float rx = (float)GetRandomValue(-halfMap + 60, halfMap - 60); 
        float rz = (float)GetRandomValue(-halfMap + 60, halfMap - 60);
        float h = GetHeight(rx, rz);

        if (h > Config::World::BIOME_THRESHOLD) {
            hunterHutPosition = { rx, h, rz };
            hutSpawned = true;
            break;
        }
    }

    // Попытка 2 (Фоллбэк): Если гор на карте вообще нет, спавним в лесу
    if (!hutSpawned) {
        for (int attempts = 0; attempts < 1500; ++attempts) {
            float rx = (float)GetRandomValue(-halfMap + 60, halfMap - 60);
            float rz = (float)GetRandomValue(-halfMap + 60, halfMap - 60);
            float h = GetHeight(rx, rz);

            if (h >= Config::World::SAND_LEVEL + 1.0f && h <= Config::World::BIOME_THRESHOLD) {
                hunterHutPosition = { rx, h, rz };
                hutSpawned = true;
                break;
            }
        }
    }

    // Попытка 3 (Аварийная)
    if (!hutSpawned) {
        hunterHutPosition = { 0.0f, GetHeight(0.0f, 0.0f), 0.0f };
    }
    // ──────────────────────────────────────────────────
    

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
    UnloadMesh(meadowMesh);
    UnloadMesh(fernMesh);
    UnloadMesh(coastalMesh);

    UnloadMaterial(trunkMat);
    UnloadMaterial(leafMat);
    UnloadMaterial(meadowMat);
    UnloadMaterial(fernMat);
    UnloadMaterial(coastalMat);
}

void World::ChangeWeather(){
    if (currentWeather == WeatherState::SUNNY){
        currentWeather = WeatherState::RAINING;
        weatherTimer = (float)GetRandomValue(Config::World::WEATHER_RAIN_MIN, Config::World::WEATHER_RAIN_MAX);
        // ПРИЛИВ: уровень воды повышается за 30 сек
        tideSourceOffset = waterLevelOffset;
        tideTargetOffset = Config::World::TIDE_RISE_AMOUNT;
        tideElapsed      = 0.0f;
        tideDuration     = Config::World::TIDE_RISE_DURATION;
    } else{
        currentWeather = WeatherState::SUNNY;
        weatherTimer = (float)GetRandomValue(Config::World::WEATHER_SUNNY_MIN, Config::World::WEATHER_SUNNY_MAX);
        // ОТЛИВ: уровень воды возвращается за 60 сек
        tideSourceOffset = waterLevelOffset;
        tideTargetOffset = 0.0f;
        tideElapsed      = 0.0f;
        tideDuration     = Config::World::TIDE_FALL_DURATION;
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
    if (height <= Config::World::PLANT_LEVEL) {
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

void World::QueueEntity(std::unique_ptr<Entity> entity){
    pendingEntities.push_back(std::move(entity));
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

    // ── ПЛАВНОЕ ОБНОВЛЕНИЕ ПРИЛИВА ───────────────────────────────
    if (tideElapsed < tideDuration) {
        tideElapsed += deltaTime;
        float t = tideElapsed / tideDuration;
        if (t > 1.0f) t = 1.0f;
        // Easing — плавный заход и выход (smooth-step)
        float s = t * t * (3.0f - 2.0f * t);
        waterLevelOffset = tideSourceOffset + (tideTargetOffset - tideSourceOffset) * s;
    } else {
        waterLevelOffset = tideTargetOffset;
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
            // Дождевой спавн Plant-сущностей отключён — теперь трава
            // живёт в grassPatches (фиксированный набор, поедается насмерть).
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

    for (const auto& entity : entities) {
        if (!entity->IsAlive()) {
            SpawnParticles(entity->GetPosition(), entity->GetDeathColor(), 15, false);
        }
    }

    // ── СПАВН ТРУПОВ ОВЕЦ ──────────────────────────────────────────
    // Любая смерть овцы оставляет труп (Carcass) который может съесть волк
    for (const auto& deadEntity : entities) {
        if (deadEntity->IsAlive()) continue;
        Sheep* deadSheep = dynamic_cast<Sheep*>(deadEntity.get());
        if (!deadSheep) continue;
        QueueEntity(std::make_unique<Carcass>(deadEntity->GetPosition()));
    }

    // ── НАСЛЕДОВАНИЕ ВОЖАКА (смерть ADULT-вожака от старости) ───────
    for (const auto& deadEntity : entities) {
        if (deadEntity->IsAlive()) continue;
        Wolf* deadWolf = dynamic_cast<Wolf*>(deadEntity.get());
        if (!deadWolf || !deadWolf->DiedOfOldAge() || !deadWolf->IsLeader()) continue;

        int pid = deadWolf->GetPackId();
        Vector3 deathPos = deadWolf->GetPosition();

        SpawnParticles(deathPos, (Color){180, 180, 180, 255}, 22, false);
        SpawnParticles(deathPos, (Color){120, 100, 90, 255}, 15, false);

        std::vector<Wolf*> candidates;
        for (const auto& e : entities) {
            if (!e->IsAlive()) continue;
            Wolf* w = dynamic_cast<Wolf*>(e.get());
            if (w && w->GetPackId() == pid
                && w->GetAgeStage() == Config::Wolf::AgeStage::MEDIUM) {
                candidates.push_back(w);
            }
        }
        if (!candidates.empty()) {
            int idx = GetRandomValue(0, (int)candidates.size() - 1);
            candidates[idx]->PromoteToLeader();
            SpawnParticles(candidates[idx]->GetPosition(),
                           (Color){200, 60, 60, 255}, 12, false);
        }
    }

    // ── МЕРЖ СТАИ ПОСЛЕ КОНФЛИКТА ──────────────────────────────────
    // Только для волков, погибших в БОЮ (diedInFight=true).
    // BABY и MEDIUM проигравшей стаи переходят к стае победителя.
    for (const auto& deadEntity : entities) {
        if (deadEntity->IsAlive()) continue;
        Wolf* deadWolf = dynamic_cast<Wolf*>(deadEntity.get());
        if (!deadWolf) continue;
        if (!deadWolf->DiedInFight()) continue;

        int losingPack = deadWolf->GetPackId();
        Vector3 deathPos = deadWolf->GetPosition();

        // Ищем ближайшего ЖИВОГО волка другой стаи (= победитель)
        Wolf* winner = nullptr;
        float bestD = 25.0f; // в зоне мест боя
        for (const auto& e : entities) {
            if (!e->IsAlive()) continue;
            Wolf* w = dynamic_cast<Wolf*>(e.get());
            if (!w || w->GetPackId() == losingPack) continue;
            float d = Vector3Distance(deathPos, w->GetPosition());
            if (d < bestD) { bestD = d; winner = w; }
        }
        if (!winner) continue;

        // Победитель должен быть ADULT — если MEDIUM, промокируем
        if (winner->GetAgeStage() == Config::Wolf::AgeStage::MEDIUM) {
            // Промокируем только если у стаи нет вожака сейчас
            bool hasLeader = false;
            for (const auto& e : entities) {
                if (!e->IsAlive()) continue;
                Wolf* w = dynamic_cast<Wolf*>(e.get());
                if (w && w->GetPackId() == winner->GetPackId()
                    && w->GetAgeStage() == Config::Wolf::AgeStage::ADULT) {
                    hasLeader = true; break;
                }
            }
            if (!hasLeader) winner->PromoteToLeader();
        }

        int winnerPack = winner->GetPackId();

        // Переводим всех BABY (и оставшихся выживших) проигравшей стаи
        // в стаю победителя. Меняем им packId и homePos.
        Vector3 winnerHome = winner->GetHomePos();
        for (const auto& e : entities) {
            if (!e->IsAlive()) continue;
            Wolf* w = dynamic_cast<Wolf*>(e.get());
            if (w && w->GetPackId() == losingPack) {
                w->SetPackId(winnerPack);
                w->SetHomePos(winnerHome);
            }
        }

        SpawnParticles(deathPos, (Color){200, 200, 200, 255}, 10, false);
    }

    // ── ОЧИСТКА DANGLING POINTERS ──────────────────────────────────
    // Перед удалением мёртвых: каждая ЖИВАЯ сущность должна обнулить
    // свои указатели на сущности, которые сейчас будут удалены.
    for (const auto& dying : entities) {
        if (dying->IsAlive()) continue;
        Entity* dyingPtr = dying.get();
        for (const auto& alive : entities) {
            if (!alive->IsAlive()) continue;
            alive->OnEntityDying(dyingPtr);
        }
    }

    entities.erase(
        std::remove_if(entities.begin(), entities.end(), [](const std::unique_ptr<Entity>& e) {
            return !e->IsAlive();
        }),
        entities.end()
    );

    float currentWaterLvl = GetCurrentWaterLevel();

    for (size_t i = 0; i < grassPatches.size(); ++i) {
        // Если трава ещё жива, но её высота оказалась ниже или равна уровню воды
        if (grassPatches[i].alive && grassPatches[i].position.y <= currentWaterLvl) {
            grassPatches[i].alive = false;

            // Добавим красивый визуальный эффект: всплеск воды (синие партиклы) вместо травы!
            Vector3 splashPos = grassPatches[i].position;
            splashPos.y = currentWaterLvl + 0.1f; // Чуть выше уровня воды для видимости
            
            // Спавним 4 сине-голубых партикла всплеска
            SpawnParticles(splashPos, Color{ 60, 130, 220, 255 }, 4, false);
        }
    }

    for (auto& e : pendingEntities) entities.push_back(std::move(e));
    pendingEntities.clear();

    UpdateParticles(deltaTime);
}

void World::Draw(){
    DrawModel(terrainModel, Vector3(), 1.0f, WHITE);

    // Уровень воды с учётом прилива
    float waterLvl = GetCurrentWaterLevel();
    DrawCubeV(
        (Vector3){ 0.0f, waterLvl / 2.0f, 0.0f }, 
        (Vector3){ (float)mapSize, waterLvl, (float)mapSize }, 
        (Color){ 30, 120, 190, 255 } 
    );

    // Отрисовка живой травы по типам
    for (const auto& g : grassPatches) {
        if (!g.alive) continue;
        Matrix m = MatrixTranslate(g.position.x, g.position.y + 0.2f, g.position.z);
        switch (g.type) {
            case Config::Grass::Type::MEADOW:  DrawMesh(meadowMesh,  meadowMat,  m); break;
            case Config::Grass::Type::FERN:    DrawMesh(fernMesh,    fernMat,    m); break;
            case Config::Grass::Type::COASTAL: DrawMesh(coastalMesh, coastalMat, m); break;
        }
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

    for (const auto& p : particles) {
        // Размер частицы убывает по мере угасания
        float t = Clamp(p.lifetime, 0.0f, 1.0f);
        if (p.isHeart) {
            float s = 1.5f * (0.5f + 0.5f * t); // 0.75..1.5 — уменьшается
            DrawCube(p.position, s, s, s * 0.6f, RED);
        } else {
            float s = 1.0f * t;
            DrawCube(p.position, s, s, s, p.color);
        }
    }
    for (const auto& p : particles) {
        float lifeRatio = Clamp(p.lifetime / p.maxLifetime, 0.0f, 1.0f);
        
        if (p.isHeart) {
            float s = 0.35f;
            if (lifeRatio < 0.25f) {
                s *= (lifeRatio / 0.25f); 
            }
            
            DrawCube(p.position, s, s, s, RED);
            DrawCubeWires(p.position, s, s, s, MAROON); 
        } else {
            float s = 0.22f * lifeRatio;
            if (s < 0.04f) s = 0.04f; 
            
            DrawCube(p.position, s, s, s, p.color);
            
            Color strokeColor = {
                (unsigned char)(p.color.r * 0.6f),
                (unsigned char)(p.color.g * 0.6f),
                (unsigned char)(p.color.b * 0.6f),
                255
            };
            DrawCubeWires(p.position, s, s, s, strokeColor);
        }
    }
    // ─── ОТРИСОВКА ДОМИКА ОХОТНИКА ───
    // База домика (коричневый куб 4x4x4)
    Vector3 houseCenter = hunterHutPosition;
    houseCenter.y += 2.0f; // Приподнимаем, чтобы куб стоял НА земле, а не наполовину под ней
    DrawCube(houseCenter, 4.0f, 4.0f, 4.0f, BROWN);
    DrawCubeWires(houseCenter, 4.0f, 4.0f, 4.0f, DARKBROWN);

    // Крыша домика (бордовый куб поменьше чуть выше основания)
    Vector3 roofPos = hunterHutPosition;
    roofPos.y += 4.0f;
    DrawCube((Vector3){roofPos.x, roofPos.y + 1.0f, roofPos.z}, 4.5f, 2.0f, 4.5f, MAROON);
    DrawCubeWires((Vector3){roofPos.x, roofPos.y + 1.0f, roofPos.z}, 4.5f, 2.0f, 4.5f, BLACK);
}


void World::SpawnParticles(Vector3 pos, Color color, int count, bool isHeart) {
    for (int i = 0; i < count; i++) {
        Particle p;
        
        float rx = (float)GetRandomValue(-4, 4) / 10.0f;  // -0.4 .. 0.4
        float ry = (float)GetRandomValue(3, 12) / 10.0f;  // 0.3 .. 1.2 (уровень тела животного)
        float rz = (float)GetRandomValue(-4, 4) / 10.0f;  // -0.4 .. 0.4
        
        p.position = { pos.x + rx, pos.y + ry, pos.z + rz };
        p.color = color;
        p.isHeart = isHeart;

        if (isHeart) {
            p.velocity = {
                (float)GetRandomValue(-8, 8) / 10.0f,
                (float)GetRandomValue(12, 22) / 10.0f, // Вертикальный импульс
                (float)GetRandomValue(-8, 8) / 10.0f
            };
            p.lifetime = 1.2f + (float)GetRandomValue(0, 6) / 10.0f; // живут 1.2 .. 1.8 сек
        } else {
            // Обычные партиклы (разрушение блоков / еда)
            p.velocity = {
                (float)GetRandomValue(-30, 30) / 10.0f,
                (float)GetRandomValue(15, 45) / 10.0f,
                (float)GetRandomValue(-30, 30) / 10.0f
            };
            p.lifetime = 0.5f + (float)GetRandomValue(0, 5) / 10.0f; // живут 0.5 .. 1.0 сек
        }
        
        p.maxLifetime = p.lifetime; // Фиксируем стартовое время жизни
        particles.push_back(p);
    }
}

void World::UpdateParticles(float deltaTime) {
    for (auto it = particles.begin(); it != particles.end();) {
        if (it->isHeart) {
            // плавно плывут вверх + покачиваются из стороны в сторону
            it->velocity.y = 1.3f; // Константный подъём
            
            // Swaying-эффект (покачивание) на основе оставшегося времени жизни
            it->velocity.x = sinf(it->lifetime * 5.0f) * 0.4f;
            it->velocity.z = cosf(it->lifetime * 5.0f) * 0.4f;
        } else {
            // Логика обычных блоков: гравитация + трение о воздух
            it->velocity.y -= 9.81f * deltaTime; // Падение вниз
            
            // Drag (плавное замедление горизонтального разлёта)
            float drag = 1.0f - (2.0f * deltaTime);
            if (drag < 0.0f) drag = 0.0f;
            it->velocity.x *= drag;
            it->velocity.z *= drag;
        }

        it->position.x += it->velocity.x * deltaTime;
        it->position.y += it->velocity.y * deltaTime;
        it->position.z += it->velocity.z * deltaTime;

        // Фиксация над землёй (чтобы частицы не тонули в геометрии ландшафта)
        float terrainHeight = GetHeight(it->position.x, it->position.z);
        if (it->position.y < terrainHeight) {
            it->position.y = terrainHeight;
            if (!it->isHeart) {
                it->velocity.y *= -0.2f; // Лёгкий майнкрафтовский отскок при ударе о землю
                it->velocity.x *= 0.5f;  
                it->velocity.z *= 0.5f;
            }
        }

        it->lifetime -= deltaTime;
        
        if (it->lifetime <= 0.0f) {
            it = particles.erase(it);
        } else {
            ++it;
        }
    }
}
// ── Поиск ближайшей живой травы ────────────────────────────────────────
int World::FindNearestGrass(Vector3 from, float maxRadius,
                            Vector3& outPos, Config::Grass::Type& outType) const {
    int   bestIdx  = -1;
    float bestDist = maxRadius;
    for (size_t i = 0; i < grassPatches.size(); ++i) {
        if (!grassPatches[i].alive) continue;
        float d = Vector3Distance(from, grassPatches[i].position);
        if (d < bestDist) {
            bestDist = d;
            bestIdx  = (int)i;
        }
    }
    if (bestIdx >= 0) {
        outPos  = grassPatches[bestIdx].position;
        outType = grassPatches[bestIdx].type;
    }
    return bestIdx;
}

// ── Поедание травы по индексу ──────────────────────────────────────────
float World::EatGrass(int index) {
    if (index < 0 || index >= (int)grassPatches.size()) return 0.0f;
    if (!grassPatches[index].alive) return 0.0f;
    grassPatches[index].alive = false;

    // Партиклы исчезающей травы: ярко-зелёный взрыв из листочков
    Vector3 burstPos = grassPatches[index].position;
    burstPos.y += 0.3f;
    SpawnParticles(burstPos, (Color) { 90, 220, 60, 255 }, 8, false);

    switch (grassPatches[index].type) {
    case Config::Grass::Type::MEADOW:  return Config::Grass::MEADOW_NUTRITION;
    case Config::Grass::Type::FERN:    return Config::Grass::FERN_NUTRITION;
    case Config::Grass::Type::COASTAL: return Config::Grass::COASTAL_NUTRITION;
    }
    return 0.0f;
}
// ── Draw2D: охотник рисует свой тег через этот метод 
void World::Draw2D(Camera camera) {
    // Пока пуст — hunter.Draw2D вызывается из main или draw
}

// ↓↓↓ ДОБАВЬТЕ ЭТОТ МЕТОД СЮДА ↓↓↓
int World::GetGrassCount() const {
    int count = 0;
    for (const auto& g : grassPatches) {
        if (g.alive) count++;
    }
    return count;
}
// ↑↑↑ ДОБАВЬТЕ ЭТОТ МЕТОД СЮДА ↑↑↑
