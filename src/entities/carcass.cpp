#include "carcass.hpp"
#include "../core/world.hpp"
#include <cmath>
#include <raymath.h>

Carcass::Carcass(Vector3 startPosition) : Entity(startPosition) {
    age           = 0.0f;
    remainingFood = INITIAL_FOOD;
}
 
void Carcass::Update(float deltaTime, World* world) {
    (void)world;
    age += deltaTime;
    if (age >= TOTAL_LIFETIME) Die();
    // Примечание: смерть от поедания крабами обрабатывается в Eat()
}
 
Carcass::Freshness Carcass::GetFreshness() const {
    if (age < FRESH_UNTIL)  return Freshness::FRESH;
    if (age < MEDIUM_UNTIL) return Freshness::MEDIUM;
    return Freshness::ROTTEN;
}
 
float Carcass::GetNutrition() const {
    switch (GetFreshness()) {
        case Freshness::FRESH:  return 40.0f;
        case Freshness::MEDIUM: return 30.0f;
        case Freshness::ROTTEN: return 20.0f;
    }
    return 0.0f;
}
 
bool Carcass::IsRotten() const {
    return age >= MEDIUM_UNTIL;
}
 
// Краб забирает amount единиц еды из трупа.
// Возвращает сколько реально было отдано (может быть меньше, если еда кончилась).
float Carcass::Eat(float amount) {
    if (remainingFood <= 0.0f || !IsAlive()) return 0.0f;
 
    float given = fminf(amount, remainingFood);
    remainingFood -= given;
 
    if (remainingFood <= 0.0f) {
        remainingFood = 0.0f;
        Die();   // труп полностью съеден
    }
    return given;
}
 
void Carcass::Draw() {
    if (!IsAlive()) return;
 
    // Масштаб тела уменьшается по мере поедания (0.3..1.0)
    float eatRatio = Clamp(remainingFood / 80.0f, 0.0f, 1.0f);
    float sx = 1.1f * (0.3f + 0.7f * eatRatio);
    float sz = 0.7f * (0.3f + 0.7f * eatRatio);
 
    // Цвет меняется от почти-белого (свежий) к серо-коричневому (гнилой)
    Color body;
    switch (GetFreshness()) {
        case Freshness::FRESH:
            body = (Color){200, 195, 185, 255};
            break;
        case Freshness::MEDIUM:
            body = (Color){130, 120, 100, 255};
            break;
        case Freshness::ROTTEN:
            body = (Color){80, 75, 50, 255};
            break;
    }
 
    // Лежащая фигура — низкая и широкая
    DrawCube(position, sx, 0.45f, sz, body);
    DrawCubeWires(position, sx, 0.45f, sz, (Color){40, 35, 30, 200});
 
    // У свежего трупа — маленькие тёмные пятна (кровь)
    if (GetFreshness() == Freshness::FRESH) {
        Vector3 spot1 = { position.x - 0.3f, position.y - 0.15f, position.z + 0.2f };
        Vector3 spot2 = { position.x + 0.2f, position.y - 0.15f, position.z - 0.25f };
        DrawCube(spot1, 0.25f, 0.05f, 0.2f, (Color){120, 30, 30, 255});
        DrawCube(spot2, 0.2f,  0.05f, 0.25f,(Color){100, 25, 25, 255});
    }
    // У несвежего — зеленоватые пятна гниения
    else if (GetFreshness() == Freshness::ROTTEN) {
        Vector3 spot = { position.x, position.y + 0.05f, position.z };
        DrawCube(spot, 0.6f * eatRatio, 0.1f, 0.4f * eatRatio, (Color){80, 110, 50, 200});
    }
}