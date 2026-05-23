#pragma once

namespace Config {

    namespace World{
        //ОТРИСОВКА
        constexpr int MAP_SIZE = 200;
        constexpr float MESH_DENSITY = 0.5f;

        constexpr float PLANT_SPAWN_DELAY = 1.5f;
        constexpr float WEATHER_SUNNY_MIN = 10.0f;
        constexpr float WEATHER_SUNNY_MAX = 15.0f;
        constexpr float WEATHER_RAIN_MIN = 10.0f;
        constexpr float WEATHER_RAIN_MAX = 15.0f;

        //ФРАКТАЛЬНЫЙ ШУМ 
        constexpr float PERLIN_SCALE = 0.04f;    // ширина биомов
        constexpr float PERLIN_AMPLITUDE = 6.0f; // макс высота гор
        constexpr float PERLIN_EXPONENT = 1.5f;  // выравнивание долин
        
        constexpr int OCTAVES = 4;               // колво слоев
        constexpr float PERSISTENCE = 0.5f;      // 0.0 - 1.0
        constexpr float LACUNARITY = 2.0f;       // уменьшение слоев

        //БИОМЫ
        constexpr float WATER_LEVEL = 0.6f;         
        constexpr float SAND_LEVEL = 0.8f;       
        constexpr float BIOME_THRESHOLD = 2.5f;  // высота лесов до 2.5 потом луга

        
    }

    namespace Sheep {
        constexpr int INITIAL_COUNT = 15;
        
        constexpr float SPEED_WALK = 1.5f;
        constexpr float SPEED_RUN = 4.0f;

        constexpr float VISION_RADIUS = 5.0f;

        constexpr float EAT_RADUIS = 1.0f;

        constexpr float REACH_TARGET_DIST = 0.2f;

        constexpr float MAX_HUNGER = 100.0f;
        constexpr float HUNGER_THRESHOLD = 50.0f;
        constexpr float HUNGER_DECAY_RATE = 1.0f;

        constexpr float IDLE_TIME = 2.0f;

        constexpr int WALK_INCREASE = 115;
        constexpr int WALK_DECREASE = 85;
        constexpr int RUN_INCREASE = 115;
        constexpr int RUN_DECREASE = 85; 
        constexpr int MAX_HUNGER_INCREASE = 115;
        constexpr int MAX_HUNGER_DECREASE = 85;
        constexpr int HUNGER_THRESHOLD_INCREASE = 115;
        constexpr int HUNGER_THRESHOLD_DECREASE = 85;
        constexpr int VISION_INCREASE = 115;
        constexpr int VISION_DECREASE = 85;
    }

    namespace Wolf {
        constexpr float SPEED_WALK = 1.8f;
        constexpr float SPEED_RUN = 4.5f;

        constexpr float VISION_RADIUS = 8.0f;
        constexpr float ATTACK_RADIUS = 1.2f;
    }
}