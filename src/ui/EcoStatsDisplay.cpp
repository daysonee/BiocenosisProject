#pragma warning(disable: 4576)
#include "EcoStatsDisplay.hpp"
#include <cstdio>

EcoStatsDisplay::EcoStatsDisplay(int screenPosX, int screenPosY)
    : sheepAlive(0)
    , sheepTotalBorn(0)
    , wolvesAlive(0)
    , wolvesTotalBorn(0)
    , grassCount(0)
    , simulationTimeSeconds(0.0)
    , sheepEatenCount(0)
    , starvedWolvesCount(0)
    , posX(screenPosX)
    , posY(screenPosY)
    , fontSize(20)
    , panelColor({ 0, 0, 0, 200 })      // ЧЁРНЫЙ ФОН - хорошо видно
    , textColor(WHITE)                  // БЕЛЫЙ ТЕКСТ
{
}

void EcoStatsDisplay::setSheepCount(int alive, int totalBorn) {
    sheepAlive = alive;
    sheepTotalBorn = totalBorn;
}

void EcoStatsDisplay::setWolfCount(int alive, int totalBorn) {
    wolvesAlive = alive;
    wolvesTotalBorn = totalBorn;
}

void EcoStatsDisplay::setGrassCount(int amount) {
    grassCount = amount;
}

void EcoStatsDisplay::setSimulationTime(double seconds) {
    simulationTimeSeconds = seconds;
}

void EcoStatsDisplay::setSheepEaten(int count) {
    sheepEatenCount = count;
}

void EcoStatsDisplay::setStarvedWolves(int count) {
    starvedWolvesCount = count;
}

std::string EcoStatsDisplay::GetBalanceState() const {
    if (sheepAlive < 3 && wolvesAlive > 0) return "COLLAPSE";
    if (wolvesAlive > sheepAlive * 1.5f) return "WOLVES WIN";
    if (sheepAlive > wolvesAlive * 2.0f) return "SHEEP WIN";
    return "STABLE";
}

std::string EcoStatsDisplay::FormatTime(int totalSeconds) const {
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
    return std::string(buffer);
}

void EcoStatsDisplay::Update(float deltaTime) {
    // Ничего не делаем
}

void EcoStatsDisplay::Draw() const {
    int lineHeight = fontSize + 4;
    int yOffset = posY;

    // Яркая рамка для проверки
    DrawRectangle(posX - 5, posY - 5, 320, 280, panelColor);
    DrawRectangleLines(posX - 5, posY - 5, 320, 280, YELLOW);  // Жёлтая рамка

    DrawText("=== STATISTICS ===", posX, yOffset, fontSize + 2, SKYBLUE);
    yOffset += lineHeight + 4;

    DrawText(TextFormat("Sheep: %d (born: %d)", sheepAlive, sheepTotalBorn),
        posX, yOffset, fontSize, GREEN);
    yOffset += lineHeight;

    DrawText(TextFormat("Wolves: %d (born: %d)", wolvesAlive, wolvesTotalBorn),
        posX, yOffset, fontSize, RED);
    yOffset += lineHeight;

    DrawText(TextFormat("Grass: %d", grassCount),
        posX, yOffset, fontSize, YELLOW);
    yOffset += lineHeight + 6;

    int totalSec = (int)simulationTimeSeconds;
    DrawText(TextFormat("Time: %s", FormatTime(totalSec).c_str()),
        posX, yOffset, fontSize, WHITE);
    yOffset += lineHeight;

    std::string balance = GetBalanceState();
    Color balanceColor;
    if (balance == "STABLE") balanceColor = GREEN;
    else if (balance == "WOLVES WIN") balanceColor = RED;
    else if (balance == "SHEEP WIN") balanceColor = ORANGE;
    else balanceColor = PURPLE;

    DrawText(TextFormat("Balance: %s", balance.c_str()),
        posX, yOffset, fontSize, balanceColor);
    yOffset += lineHeight + 6;

    DrawText(TextFormat("Sheep died: %d", sheepEatenCount),
        posX, yOffset, fontSize, ORANGE);
    yOffset += lineHeight;

    DrawText(TextFormat("Wolves starved: %d", starvedWolvesCount),
        posX, yOffset, fontSize, GRAY);
}