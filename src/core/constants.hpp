#pragma once

namespace Config {
    namespace World {
        constexpr int   RAINDROPS_COUNT = 10000;

        // ОТРИСОВКА
        constexpr int   MAP_SIZE        = 1000;
        constexpr float MESH_DENSITY    = 1;     
        constexpr float PLANT_SPAWN_DELAY   = 1.5f;
        constexpr float WEATHER_SUNNY_MIN   = 10.0f;
        constexpr float WEATHER_SUNNY_MAX   = 15.0f;
        constexpr float WEATHER_RAIN_MIN    = 10.0f;
        constexpr float WEATHER_RAIN_MAX    = 15.0f;

        // ФРАКТАЛЬНЫЙ ШУМ
        constexpr float PERLIN_SCALE     = 0.0025f; 
        constexpr float PERLIN_AMPLITUDE = 80.0f;   
        constexpr float PERLIN_EXPONENT  = 1.3f;    

        constexpr int   OCTAVES          = 4;        
        constexpr float PERSISTENCE      = 0.45f;     
        constexpr float LACUNARITY       = 2.0f;     

        // БИОМЫ 
        constexpr float WATER_LEVEL      = 20.0f;   
        constexpr float SAND_LEVEL       = 23.0f;   
        constexpr float BIOME_THRESHOLD  = 35.0f;  
        constexpr float PLANT_LEVEL      = 65.0f;
        constexpr int   TREE_COUNT       = 5000;
        constexpr int   BUSH_COUNT      = 1000; // (legacy, не используется)
    }

    namespace Grass {
        // Кол-во на старте
        constexpr int MEADOW_COUNT  = 600; // обычная трава на лугах
        constexpr int FERN_COUNT    = 400; // папоротники в лесу
        constexpr int COASTAL_COUNT = 200; // прибрежная трава у воды

        // Насыщение от поедания (восстановление голода)
        constexpr float MEADOW_NUTRITION  = 60.0f; // сытная луговая трава
        constexpr float FERN_NUTRITION    = 35.0f; // папоротник — так себе
        constexpr float COASTAL_NUTRITION = 45.0f; // прибрежная — средне

        // Высотные границы спавна
        // Прибрежная: от уреза воды до полосы песка чуть выше
        constexpr float COASTAL_MIN = Config::World::WATER_LEVEL;       // 20
        constexpr float COASTAL_MAX = Config::World::SAND_LEVEL + 2.0f; // 25
        // Папоротники: лесная зона (там же где деревья)
        constexpr float FERN_MIN    = Config::World::SAND_LEVEL;        // 23
        constexpr float FERN_MAX    = Config::World::BIOME_THRESHOLD;   // 35
        // Луговая: луга выше леса
        constexpr float MEADOW_MIN  = Config::World::BIOME_THRESHOLD;   // 35
        constexpr float MEADOW_MAX  = Config::World::PLANT_LEVEL;       // 65

        enum class Type { MEADOW, FERN, COASTAL };
    }

    namespace Sheep {
        // Вдвое больше овец, с запасом для прибрежного спавна
        constexpr int   INITIAL_COUNT    = 200;

        // Добавить внутрь namespace Config::Sheep
        enum class AgeStage {
            BABY,
            MEDIUM,
            ADULT
        };

        // Настройки возраста (в секундах)
        constexpr float TIME_TO_GROW_BABY_MIN   = 180.0f; // 3 минуты
        constexpr float TIME_TO_GROW_BABY_MAX   = 240.0f; // 4 минуты
        constexpr float TIME_TO_GROW_MEDIUM_MIN = 360.0f; // 6 минут
        constexpr float TIME_TO_GROW_MEDIUM_MAX = 420.0f; // 7 минут
        constexpr float TIME_ADULT_LIFESPAN     = 720.0f; // 12 минут

        // Настройки спаривания
        constexpr float MATING_HUNGER_THRESHOLD = 60.0f;  // Овца хочет спариваться при сытости > 60
        constexpr float MATING_COOLDOWN         = 30.0f;  // Откат после родов (30 сек)
        constexpr float MATING_APPROACH_DIST    = 1.0f;   // Дистанция для начала процесса
        constexpr float MATING_PROCESS_TIME     = 3.0f;   // Сколько секунд нужно стоять рядом


        constexpr float SPEED_WALK       = 1.5f;
        constexpr float SPEED_RUN        = 4.0f;

        constexpr float BODY_RADIUS      = 0.4f;         // физический радиус одной овечки
        constexpr float SEVERE_STUCK_RADIUS = 0.6f;

        // Безопасная зона: на каком расстоянии овца чует волка при спавне
        constexpr float SAFE_ZONE_FROM_WOLVES = 12.0f;

        // Радиус разброса при первоначальном спавне стада
        constexpr float SPAWN_FLOCK_RADIUS    = 5.0f;

        constexpr float VISION_RADIUS    = 8.0f;
        constexpr float EAT_RADUIS       = 1.0f;
        constexpr float REACH_TARGET_DIST = 0.2f;

        constexpr float MAX_HUNGER           = 100.0f;
        constexpr float HUNGER_THRESHOLD     = 50.0f;
        constexpr float HUNGER_DECAY_RATE    = 1.0f;

        constexpr float IDLE_TIME            = 2.0f;

        constexpr int   WALK_INCREASE        = 115;
        constexpr int   WALK_DECREASE        = 85;
        constexpr int   RUN_INCREASE         = 115;
        constexpr int   RUN_DECREASE         = 85;
        constexpr int   MAX_HUNGER_INCREASE  = 115;
        constexpr int   MAX_HUNGER_DECREASE  = 85;
        constexpr int   HUNGER_THRESHOLD_INCREASE = 115;
        constexpr int   HUNGER_THRESHOLD_DECREASE = 85;
        constexpr int   VISION_INCREASE      = 115;
        constexpr int   VISION_DECREASE      = 85;

        // ── ТАКТИКА ПОБЕГА ────────────────────────────────────────
        // Зигзаг: периодически меняем угол бега на ±ZIGZAG_ANGLE_DEG
        constexpr float ZIGZAG_INTERVAL_MIN  = 0.4f;
        constexpr float ZIGZAG_INTERVAL_MAX  = 0.8f;
        constexpr float ZIGZAG_ANGLE_DEG     = 50.0f;
        // Финт в последний момент: если волк ближе DODGE_RANGE,
        // резко поворачиваем на ±DODGE_ANGLE_DEG (волк промахивается прыжком)
        constexpr float DODGE_RANGE          = 5.0f;
        constexpr float DODGE_ANGLE_DEG      = 90.0f;
        constexpr float DODGE_COOLDOWN       = 2.0f;
        // Бег к стаду: ищем соседей в радиусе и смещаем направление
        // к их центру массы (только если стадо НЕ в стороне волка)
        constexpr float FLOCK_SUPPORT_RADIUS = 12.0f;
        constexpr float FLOCK_SUPPORT_WEIGHT = 0.30f;
    }

    namespace Wolf {
        constexpr float SPEED_WALK    = 2.2f;
        constexpr float SPEED_RUN     = 5.5f;
        constexpr float VISION_RADIUS = 8.0f;
        constexpr float ATTACK_RADIUS = 1.2f;

        // Прыжок-рывок
        constexpr float POUNCE_SPEED    = 11.0f;
        constexpr float POUNCE_RANGE    = 4.5f;
        constexpr float POUNCE_DURATION = 0.55f;
        constexpr float POUNCE_COOLDOWN = 4.5f;
        constexpr float POUNCE_REACH    = 1.6f;
        constexpr float REST_SPEED_FACTOR = 0.65f;
        constexpr float LEAD_MAX_TIME     = 1.0f;

        // ── ВОЗРАСТНЫЕ СТАДИИ ─────────────────────────────────────
        enum class AgeStage { BABY, MEDIUM, ADULT };

        constexpr float TIME_TO_GROW_BABY_MIN   = 180.0f;
        constexpr float TIME_TO_GROW_BABY_MAX   = 240.0f;
        constexpr float TIME_TO_GROW_MEDIUM_MIN = 360.0f;
        constexpr float TIME_TO_GROW_MEDIUM_MAX = 420.0f;
        constexpr float TIME_ADULT_LIFESPAN     = 720.0f;

        // BABY-волчонок
        constexpr float BABY_SPEED_FACTOR    = 1.30f;
        constexpr float BABY_SIZE_FACTOR     = 0.65f;
        constexpr int   BABY_KILLS_TO_FULL   = 2;
        constexpr float BABY_COOLDOWN_MULT   = 2.5f;

        // ADULT-вожак
        constexpr float ADULT_POUNCE_REACH   = 3.0f;
        constexpr float ADULT_LEAD_MAX_TIME  = 2.0f;
        constexpr float ADULT_SIZE_FACTOR    = 1.10f;

        // ── ПЛАВАНИЕ ──────────────────────────────────────────────
        constexpr float SWIM_SPEED_FACTOR = 0.55f;
        constexpr float SHAKE_DURATION    = 1.0f;

        // ── РАЗМНОЖЕНИЕ ───────────────────────────────────────────
        constexpr float MATING_HUNGER_THRESHOLD = 70.0f;
        constexpr float MATING_COOLDOWN         = 60.0f;
        constexpr float MATING_APPROACH_DIST    = 1.2f;
        constexpr float MATING_PROCESS_TIME     = 3.5f;
        constexpr float MATING_SEARCH_RADIUS    = 40.0f;

        // ── СПАВН СТАЯМИ В ЛЕСУ ──────────────────────────────────
        constexpr int   PACK_COUNT    = 10;
        constexpr int   PACK_SIZE_MIN = 3;
        constexpr int   PACK_SIZE_MAX = 4;
        constexpr float PACK_RADIUS   = 6.0f;
    }

    namespace Hunter {
        constexpr float SPEED_WALK     = 2.5f;
        constexpr float SPEED_RUN      = 5.0f;
        constexpr float VISION_RADIUS  = 25.0f; 
        constexpr float SHOOT_RANGE    = 12.0f; 
        constexpr float SHOOT_COOLDOWN = 4.0f;  
        constexpr float REST_DURATION  = 20.0f; 
        constexpr float HUNT_TIMEOUT   = 40.0f; 
    }
}