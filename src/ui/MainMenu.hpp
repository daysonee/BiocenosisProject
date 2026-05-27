#pragma once
#include "raylib.h"
#include <string>
#include <cstdio>

struct MenuSettings {
    int sheepCount = 200;
    int wolfCount = 1;
    int grassCount = 1200;
    bool readyToStart = false;
};

class MainMenu {
private:
    MenuSettings& settings;
    int screenWidth, screenHeight;

    Rectangle sheepBtnDec, sheepBtnInc, sheepBtnValue;
    Rectangle wolfBtnDec, wolfBtnInc, wolfBtnValue;
    Rectangle grassBtnDec, grassBtnInc, grassBtnValue;
    Rectangle startBtn;

    const int SHEEP_MIN = 0;
    const int SHEEP_MAX = 500;
    const int WOLF_MIN = 0;
    const int WOLF_MAX = 20;
    const int GRASS_MIN = 100;
    const int GRASS_MAX = 5000;

    float titlePulse = 0.0f;
    bool pulseDirection = true;

    void DrawButton(Rectangle rect, const char* text, Color color, bool isHovered);
    void DrawNumberButton(Rectangle rect, const char* text, bool isHovered);
    void DrawValueBox(Rectangle rect, int value, const char* label, bool isHovered);

public:
    MainMenu(MenuSettings& settingsRef, int width, int height);
    void Update();
    void Draw();
};