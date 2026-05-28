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
    // Вычисляем масштаб на лету на основе текущей высоты окна
    float displayScale = (float)GetScreenHeight() / 720.0f;
    if (displayScale < 0.6f) displayScale = 0.6f;

    // Адаптивные габариты инфо-панели
    int panelWidth = (int)(330 * displayScale);
    int panelHeight = (int)(280 * displayScale);
    int dynamicFontSize = (int)(20 * displayScale);
    int lineHeight = (int)(26 * displayScale);

    // Если вы хотите, чтобы она всегда оставалась в Левом Верхнем Углу (как задумано изначально):
    int actualX = (int)(posX * displayScale);
    int actualY = (int)(posY * displayScale);

    /* ПОДСКАЗКА: Если вы захотите прижать плашку к ПРАВОМУ верхнему углу, 
    достаточно раскомментировать строку ниже:
    actualX = GetScreenWidth() - panelWidth - 15;
    */

    int yOffset = actualY + (int)(10 * displayScale);

    // Отрисовка заднего фона и рамки
    DrawRectangle(actualX - 5, actualY - 5, panelWidth, panelHeight, panelColor);
    
    // Используем DrawRectangleLinesEx для адаптивной толщины рамки
    DrawRectangleLinesEx((Rectangle){ (float)actualX - 5, (float)actualY - 5, (float)panelWidth, (float)panelHeight }, 2 * displayScale, YELLOW);  

    DrawText("=== STATISTICS ===", actualX, yOffset, dynamicFontSize + 2, SKYBLUE);
    yOffset += lineHeight + (int)(4 * displayScale);

    DrawText(TextFormat("Sheep: %d (born: %d)", sheepAlive, sheepTotalBorn),
        actualX, yOffset, dynamicFontSize, GREEN);
    yOffset += lineHeight;

    DrawText(TextFormat("Wolves: %d (born: %d)", wolvesAlive, wolvesTotalBorn),
        actualX, yOffset, dynamicFontSize, RED);
    yOffset += lineHeight;

    DrawText(TextFormat("Grass: %d", grassCount),
        actualX, yOffset, dynamicFontSize, YELLOW);
    yOffset += lineHeight + (int)(6 * displayScale);

    int totalSec = (int)simulationTimeSeconds;
    DrawText(TextFormat("Time: %s", FormatTime(totalSec).c_str()),
        actualX, yOffset, dynamicFontSize, WHITE);
    yOffset += lineHeight;

    std::string balance = GetBalanceState();
    Color balanceColor;
    if (balance == "STABLE") balanceColor = GREEN;
    else if (balance == "WOLVES WIN") balanceColor = RED;
    else if (balance == "SHEEP WIN") balanceColor = ORANGE;
    else balanceColor = PURPLE;

    DrawText(TextFormat("Status: %s", balance.c_str()),
        actualX, yOffset, dynamicFontSize, balanceColor);
    yOffset += lineHeight;

    DrawText(TextFormat("Eaten: %d | Starved: %d", sheepEatenCount, starvedWolvesCount),
        actualX, yOffset, (int)(16 * displayScale), LIGHTGRAY);
}