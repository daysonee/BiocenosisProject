#pragma once

#include "../core/entity.hpp"

// Труп животного. Появляется когда умирает овца.
// Живёт 45 секунд, проходит три стадии свежести:
//   0-20 сек: свежий   → +40 голода волку (4/10)
//   20-35 сек: средний → +30 (3/10)
//   35-45 сек: несвежий→ +20 (2/10), но 50% шанс отравить волка
// Несколько крабов могут есть одновременно: каждый вызывает Eat(rate*dt),
// пока remainingFood > 0. Когда еда заканчивается — труп гибнет.
class Carcass : public Entity {
public:
    enum class Freshness { FRESH, MEDIUM, ROTTEN };

private:
    float age;
    float remainingFood;   // запас «мяса», уменьшается по мере поедания крабами

    static constexpr float TOTAL_LIFETIME  = 110.0f;
    static constexpr float FRESH_UNTIL     = 35.0f;
    static constexpr float MEDIUM_UNTIL    = 70.0f;
    static constexpr float INITIAL_FOOD    = 80.0f;  // сколько единиц пищи в трупе суммарно

public:
    Carcass(Vector3 startPosition);
    ~Carcass() override = default;

    void Update(float deltaTime, World* world) override;
    void Draw() override;

    Freshness GetFreshness() const;
    float     GetNutrition() const;  // 40 / 30 / 20 — ценность за один укус (для волков)
    bool      IsRotten()    const;   // age >= MEDIUM_UNTIL (35)
    float     GetAge()      const { return age; }

    // Краб ест труп: забирает amount единиц, возвращает реально отданное.
    // Когда remainingFood достигает 0 — труп помечается Die().
    float     Eat(float amount);

    bool      HasFood() const { return remainingFood > 0.0f; }
    float     GetRemainingFood() const { return remainingFood; }

    Color GetDeathColor() const override { return (Color){70, 60, 50, 255}; }
};