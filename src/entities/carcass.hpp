#pragma once

#include "../core/entity.hpp"

// Труп животного. Появляется когда умирает овца.
// Живёт 45 секунд, проходит три стадии свежести:
//   0-20 сек: свежий   → +40 голода волку (4/10)
//   20-35 сек: средний → +30 (3/10)
//   35-45 сек: несвежий→ +20 (2/10), но 50% шанс отравить волка
class Carcass : public Entity {
public:
    enum class Freshness { FRESH, MEDIUM, ROTTEN };

private:
    float age;

    static constexpr float TOTAL_LIFETIME = 45.0f;
    static constexpr float FRESH_UNTIL    = 20.0f;
    static constexpr float MEDIUM_UNTIL   = 35.0f;

public:
    Carcass(Vector3 startPosition);
    ~Carcass() override = default;

    void Update(float deltaTime, World* world) override;
    void Draw() override;

    Freshness GetFreshness() const;
    float     GetNutrition() const;  // 40 / 30 / 20
    bool      IsRotten()    const;   // age >= MEDIUM_UNTIL (35)
    float     GetAge()      const { return age; }

    Color GetDeathColor() const override { return (Color){70, 60, 50, 255}; }
};