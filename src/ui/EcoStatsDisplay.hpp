#pragma once
#include "raylib.h"
#include <string>

class EcoStatsDisplay {
private:
    int sheepAlive, sheepTotalBorn;
    int wolvesAlive, wolvesTotalBorn;
    int grassCount;
    double simulationTimeSeconds;
    int sheepEatenCount;
    int starvedWolvesCount;

    int posX, posY;
    int fontSize;
    Color panelColor;
    Color textColor;

    std::string GetBalanceState() const;
    std::string FormatTime(int totalSeconds) const;

public:
    EcoStatsDisplay(int screenPosX = 10, int screenPosY = 10);

    void setSheepCount(int alive, int totalBorn);
    void setWolfCount(int alive, int totalBorn);
    void setGrassCount(int amount);
    void setSimulationTime(double seconds);
    void setSheepEaten(int count);
    void setStarvedWolves(int count);

    void Update(float deltaTime);
    void Draw() const;
};  