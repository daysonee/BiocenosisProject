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

        // ── ПРИЛИВЫ ──────────────────────────────────────────────
        constexpr float TIDE_RISE_AMOUNT   = 5.0f;   // на сколько повышается уровень
        constexpr float TIDE_RISE_DURATION = 30.0f;  // ramp up за 30 сек
        constexpr float TIDE_FALL_DURATION = 60.0f;  // ramp down за 60 сек
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
        // Финт: если волк ближе DODGE_RANGE, овца резко поворачивает на ±DODGE_ANGLE_DEG.
        // После финта направление УДЕРЖИВАЕТСЯ DODGE_LOCK_DURATION секунд — без этого
        // финт длится один тик и компенсируется упреждением волка.
        constexpr float DODGE_RANGE          = 6.0f;
        constexpr float DODGE_ANGLE_DEG      = 90.0f;
        constexpr float DODGE_COOLDOWN       = 1.2f;
        constexpr float DODGE_LOCK_DURATION  = 0.7f;
        // Скоростной рывок после финта (овца получает буст на короткое время)
        constexpr float DODGE_SPEED_BOOST    = 1.25f;
        // Бег к стаду
        constexpr float FLOCK_SUPPORT_RADIUS = 12.0f;
        constexpr float FLOCK_SUPPORT_WEIGHT = 0.30f;
    }

    namespace Wolf {
        constexpr float SPEED_WALK    = 2.2f;
        constexpr float SPEED_RUN     = 5.5f;
        constexpr float VISION_RADIUS = 8.0f;
        constexpr float ATTACK_RADIUS = 1.0f;   // 1.2 → 1.0

        // Прыжок-рывок (ослаблен — успех атаки должен быть ~30-35%)
        constexpr float POUNCE_SPEED    = 9.0f;   // 11 → 9
        constexpr float POUNCE_RANGE    = 4.0f;   // 4.5 → 4.0
        constexpr float POUNCE_DURATION = 0.55f;
        constexpr float POUNCE_COOLDOWN = 4.5f;
        constexpr float POUNCE_REACH    = 1.0f;   // 1.6 → 1.0 (для BABY)
        constexpr float MEDIUM_POUNCE_REACH = 1.3f; // у MEDIUM хватка пошире
        constexpr float REST_SPEED_FACTOR = 0.65f;
        // Упреждение сильно ослаблено — даёт шанс финту сработать
        constexpr float LEAD_MAX_TIME     = 0.4f; // 1.0 → 0.4

        // ── РАДИУС ОХОТЫ (на жертву) ─────────────────────────────
        // Волк видит овцу только в этом радиусе. Без этого жертва
        // ищется по всей карте и волки гонятся через полмира.
        constexpr float HUNTING_RADIUS_BABY   = 30.0f;
        constexpr float HUNTING_RADIUS_MEDIUM = 25.0f;
        constexpr float HUNTING_RADIUS_ADULT  = 15.0f;

        // ── ВОЗРАСТНЫЕ СТАДИИ ─────────────────────────────────────
        enum class AgeStage { BABY, MEDIUM, ADULT };

        constexpr float TIME_TO_GROW_BABY_MIN   = 180.0f;
        constexpr float TIME_TO_GROW_BABY_MAX   = 240.0f;
        constexpr float TIME_TO_GROW_MEDIUM_MIN = 360.0f;
        constexpr float TIME_TO_GROW_MEDIUM_MAX = 420.0f;
        constexpr float TIME_ADULT_LIFESPAN     = 540.0f;   // ~9 мин, итого ~18 мин (овцы ~22)

        // BABY-волчонок
        constexpr float BABY_SPEED_FACTOR    = 1.30f;
        constexpr float BABY_SIZE_FACTOR     = 0.65f;
        constexpr int   BABY_KILLS_TO_FULL   = 2;
        constexpr float BABY_COOLDOWN_MULT   = 2.5f;

        // ADULT-вожак
        constexpr float ADULT_POUNCE_REACH   = 3.0f;
        constexpr float ADULT_LEAD_MAX_TIME  = 2.0f;
        constexpr float ADULT_SIZE_FACTOR    = 1.10f;

        // ── ПЛАВАНИЕ И ВСТРЯХИВАНИЕ ──────────────────────────────
        constexpr float SWIM_SPEED_FACTOR = 0.55f;
        constexpr float SHAKE_DURATION    = 2.0f;   // 1 → 2 сек "тупит"
        constexpr float SHAKE_AMPLITUDE   = 0.25f;  // визуальная амплитуда тряски

        // ── ГОЛОД И СМЕРТЬ ────────────────────────────────────────
        constexpr float HUNGER_DECAY_RATE = 1.0f;   // ед/сек — было 2.0, теперь как у овец
        constexpr float STARVATION_LIMIT  = 30.0f;  // 30 сек на нуле = смерть (было 15)
        constexpr float HUNGER_HUNT_TRIGGER = 50.0f; // 5/10 — порог активной охоты

        // Трава — плохая еда для волка: после её поедания
        // голод падает быстрее на короткое время
        constexpr float GRASS_EFFECT_DURATION = 30.0f;  // длительность эффекта
        constexpr float GRASS_HUNGER_MULT     = 2.5f;   // умножение decay
        // Когда волк очень голоден — расширяем радиус поиска травы
        constexpr float DESPERATE_HUNGER      = 30.0f;  // 3/10
        constexpr float GRASS_DESPERATE_RADIUS = 60.0f;

        // ── ДОМ СТАИ (патрулирование своей зоны) ─────────────────
        constexpr float HOME_PATROL_RADIUS = 70.0f;

        // ── КОНФЛИКТ СТАЙ ────────────────────────────────────────
        // Когда ADULT/MEDIUM встречает волка другой стаи (другой packId),
        // начинается бой. BABY не участвуют. Один выживает → вожак.
        constexpr float FIGHT_TRIGGER_RADIUS = 6.0f;
        constexpr float FIGHT_CONTACT_DIST   = 1.5f;
        constexpr float FIGHT_DAMAGE_RATE    = 25.0f;  // hp/сек при контакте
        constexpr float FIGHT_MAX_HEALTH     = 100.0f;
        // Cooldown после боя, чтобы не атаковали тут же бабушку
        constexpr float FIGHT_COOLDOWN       = 25.0f;

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
        constexpr float SHOOT_COOLDOWN = 1.0f;  
        constexpr float REST_DURATION  = 20.0f; 
        constexpr float HUNT_TIMEOUT   = 40.0f; 
    }

        namespace Crab {
        constexpr int   INITIAL_COUNT       = 30;
 
        constexpr float SPEED_WALK          = 1.5f;
        constexpr float WANDER_RADIUS       = 25.0f;
 
        // Поедание трупа
        constexpr float EAT_RADIUS          = 2.0f;    // дистанция для поедания
        constexpr float EAT_RATE            = 8.0f;    // единиц питания/сек (каждый краб)
        constexpr float HUNGER_MAX          = 100.0f;
        constexpr float HUNGER_DECAY        = 0.5f;    // ед/сек
        constexpr float HUNGER_EAT_TRIGGER  = 60.0f;   // при каком уровне голода ищет еду
 
        // Возраст
        constexpr float LIFESPAN_MIN        = 120.0f;  // 2 минуты
        constexpr float LIFESPAN_MAX        = 200.0f;  // ~3.3 минуты
 
        // Размножение
        constexpr float MATING_HUNGER_THRESHOLD = 80.0f;  // нужно наесться
        constexpr float MATING_COOLDOWN         = 40.0f;
        constexpr float MATING_PROCESS_TIME     = 3.0f;
        constexpr float MATING_APPROACH_DIST    = 1.5f;
        constexpr float SPAWN_OFFSET_RADIUS     = 2.0f;
    }
}