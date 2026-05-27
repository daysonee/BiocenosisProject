#include "carcass.hpp"
#include "../core/world.hpp"

Carcass::Carcass(Vector3 startPosition) : Entity(startPosition) {
    age = 0.0f;
}

void Carcass::Update(float deltaTime, World* world) {
    (void)world;
    age += deltaTime;
    if (age >= TOTAL_LIFETIME) Die();
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

void Carcass::Draw() {
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
    DrawCube(position, 1.1f, 0.45f, 0.7f, body);
    DrawCubeWires(position, 1.1f, 0.45f, 0.7f, (Color){40, 35, 30, 200});

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
        DrawCube(spot, 0.6f, 0.1f, 0.4f, (Color){80, 110, 50, 200});
    }
}