#include "MainMenu.hpp"
#include "raymath.h"
void MainMenu::DrawButton(Rectangle rect, const char* text, Color color, bool isHovered) {
    Color btnColor = isHovered ? Color{ color.r, color.g, color.b, 200 } : color;
    DrawRectangleRec(rect, btnColor);
    DrawRectangleLinesEx(rect, 2, DARKGRAY);

    // Масштабируем размер шрифта динамически под размер кнопки
    int dynamicFontSize = (int)(28 * scale); 
    int textWidth = MeasureText(text, dynamicFontSize);
    
    DrawText(text, rect.x + rect.width / 2 - textWidth / 2,
        rect.y + rect.height / 2 - dynamicFontSize / 2, dynamicFontSize, BLACK);
}

void MainMenu::DrawNumberButton(Rectangle rect, const char* text, bool isHovered) {
    Color color = isHovered ? LIGHTGRAY : RAYWHITE;
    DrawRectangleRec(rect, color);
    DrawRectangleLinesEx(rect, 2, DARKGRAY);

    int dynamicFontSize = (int)(30 * scale);
    int textWidth = MeasureText(text, dynamicFontSize);
    DrawText(text, rect.x + rect.width / 2 - textWidth / 2,
        rect.y + rect.height / 2 - dynamicFontSize / 2, dynamicFontSize, BLACK);
}

void MainMenu::DrawValueBox(Rectangle rect, int value, const char* label, bool isHovered) {
    Color color = isHovered ? SKYBLUE : RAYWHITE;
    DrawRectangleRec(rect, color);
    DrawRectangleLinesEx(rect, 2, DARKGRAY);

    char valText[32];
    snprintf(valText, sizeof(valText), "%s: %d", label, value);

    int dynamicFontSize = (int)(24 * scale);
    int textWidth = MeasureText(valText, dynamicFontSize);
    DrawText(valText, rect.x + rect.width / 2 - textWidth / 2,
        rect.y + rect.height / 2 - dynamicFontSize / 2, dynamicFontSize, BLACK);
}

MainMenu::MainMenu(MenuSettings& settingsRef, int width, int height)
    : settings(settingsRef), screenWidth(width), screenHeight(height) {

    float centerX = (float)screenWidth / 2.0f;
    float startY = (float)screenHeight / 3.0f;

    sheepBtnValue = { centerX - 100.0f, startY, 200.0f, 50.0f };
    sheepBtnDec = { centerX - 140.0f, startY, 35.0f, 50.0f };
    sheepBtnInc = { centerX + 105.0f, startY, 35.0f, 50.0f };

    wolfBtnValue = { centerX - 100.0f, startY + 80.0f, 200.0f, 50.0f };
    wolfBtnDec = { centerX - 140.0f, startY + 80.0f, 35.0f, 50.0f };
    wolfBtnInc = { centerX + 105.0f, startY + 80.0f, 35.0f, 50.0f };

    grassBtnValue = { centerX - 100.0f, startY + 160.0f, 200.0f, 50.0f };
    grassBtnDec = { centerX - 140.0f, startY + 160.0f, 35.0f, 50.0f };
    grassBtnInc = { centerX + 105.0f, startY + 160.0f, 35.0f, 50.0f };

    startBtn = { centerX - 120.0f, startY + 260.0f, 240.0f, 60.0f };
}

void MainMenu::Update() {
    // 1. ДИНАМИЧЕСКИЙ ПЕРЕРАСЧЕТ ГЕОМЕТРИИ И МАСШТАБА
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();
    float deltaTime = GetFrameTime();    
    // За базовую высоту возьмем 720 пикселей. На 1440p масштаб станет 2.0x, на FullHD — ~1.5x
    scale = (float)screenHeight / 720.0f;
    if (scale < 0.5f) scale = 0.5f; // Защита от слишком сильного сжатия окна

    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;

    // Адаптивные метрики блоков интерфейса
    float boxWidth  = 220.0f * scale;
    float boxHeight = 55.0f  * scale;
    float btnWidth  = 45.0f  * scale;
    float spacing   = 15.0f  * scale;

    // Расставляем элементы вертикально от центра экрана
    // Строка настроек Овец (Выше центра)
    float ySheep = centerY - boxHeight - spacing;
    sheepBtnValue = { centerX - boxWidth / 2.0f, ySheep, boxWidth, boxHeight };
    sheepBtnDec   = { sheepBtnValue.x - btnWidth - 5 * scale, ySheep, btnWidth, boxHeight };
    sheepBtnInc   = { sheepBtnValue.x + sheepBtnValue.width + 5 * scale, ySheep, btnWidth, boxHeight };

    // Строка настроек Волков (Строго по центру)
    float yWolf = centerY;
    wolfBtnValue  = { centerX - boxWidth / 2.0f, yWolf, boxWidth, boxHeight };
    wolfBtnDec    = { wolfBtnValue.x - btnWidth - 5 * scale, yWolf, btnWidth, boxHeight };
    wolfBtnInc    = { wolfBtnValue.x + wolfBtnValue.width + 5 * scale, yWolf, btnWidth, boxHeight };

    // Строка настроек Травы (Ниже центра)
    float yGrass = centerY + boxHeight + spacing;
    grassBtnValue = { centerX - boxWidth / 2.0f, yGrass, boxWidth, boxHeight };
    grassBtnDec   = { grassBtnValue.x - btnWidth - 5 * scale, yGrass, btnWidth, boxHeight };
    grassBtnInc   = { grassBtnValue.x + grassBtnValue.width + 5 * scale, yGrass, btnWidth, boxHeight };

    // Большая Адаптивная Кнопка СТАРТ
    float startWidth  = 340.0f * scale;
    float startHeight = 65.0f * scale;
    startBtn = { centerX - startWidth / 2.0f, yGrass + boxHeight + spacing * 2, startWidth, startHeight };

    // (Ваш оригинальный код обработки пульсации titlePulse...)

    // 2. ОБРАБОТКА ИНПУТА (Теперь работает по динамическим Rectangle!)
    Vector2 mousePos = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mousePos, sheepBtnDec) && settings.sheepCount > SHEEP_MIN) settings.sheepCount -= 10;
        if (CheckCollisionPointRec(mousePos, sheepBtnInc) && settings.sheepCount < SHEEP_MAX) settings.sheepCount += 10;
        
        if (CheckCollisionPointRec(mousePos, wolfBtnDec) && settings.wolfCount > WOLF_MIN) settings.wolfCount -= 5;
        if (CheckCollisionPointRec(mousePos, wolfBtnInc) && settings.wolfCount < WOLF_MAX) settings.wolfCount += 5;
        
        if (CheckCollisionPointRec(mousePos, grassBtnDec) && settings.grassCount > GRASS_MIN) settings.grassCount -= 100;
        if (CheckCollisionPointRec(mousePos, grassBtnInc) && settings.grassCount < GRASS_MAX) settings.grassCount += 100;

        if (CheckCollisionPointRec(mousePos, startBtn)) {
            settings.readyToStart = true;
        }
    }
}

void MainMenu::Draw() {
    // Градиентный фон
    for (int i = 0; i < screenHeight; i++) {
        float t = (float)i / screenHeight;
        Color color = {
            (unsigned char)(20 + t * 40),
            (unsigned char)(30 + t * 50),
            (unsigned char)(40 + t * 60),
            255
        };
        DrawLine(0, i, screenWidth, i, color);
    }

    // Заголовок
    float titleScale = 0.9f + titlePulse * 0.2f;
    int titleSize = (int)(60 * titleScale);
    if (titleSize < 45) titleSize = 45;
    if (titleSize > 75) titleSize = 75;

    const char* title = "BIOCYNOSIS";
    int titleWidth = MeasureText(title, titleSize);
    DrawText(title, screenWidth / 2 - titleWidth / 2, 60, titleSize,
        Color{ 100, 200, 100, 255 });

    DrawText("Ecosystem Simulation", screenWidth / 2 - MeasureText("Ecosystem Simulation", 20) / 2,
        130, 20, LIGHTGRAY);

    Vector2 mousePos = GetMousePosition();

    DrawValueBox(sheepBtnValue, settings.sheepCount, "SHEEP",
        CheckCollisionPointRec(mousePos, sheepBtnValue));
    DrawNumberButton(sheepBtnDec, "-", CheckCollisionPointRec(mousePos, sheepBtnDec));
    DrawNumberButton(sheepBtnInc, "+", CheckCollisionPointRec(mousePos, sheepBtnInc));

    DrawValueBox(wolfBtnValue, settings.wolfCount, "WOLVES",
        CheckCollisionPointRec(mousePos, wolfBtnValue));
    DrawNumberButton(wolfBtnDec, "-", CheckCollisionPointRec(mousePos, wolfBtnDec));
    DrawNumberButton(wolfBtnInc, "+", CheckCollisionPointRec(mousePos, wolfBtnInc));

    DrawValueBox(grassBtnValue, settings.grassCount, "GRASS",
        CheckCollisionPointRec(mousePos, grassBtnValue));
    DrawNumberButton(grassBtnDec, "-", CheckCollisionPointRec(mousePos, grassBtnDec));
    DrawNumberButton(grassBtnInc, "+", CheckCollisionPointRec(mousePos, grassBtnInc));

    bool startHover = CheckCollisionPointRec(mousePos, startBtn);
    Color startColor = startHover ? GREEN : DARKGREEN;
    DrawRectangleRec(startBtn, startColor);
    DrawRectangleLinesEx(startBtn, 3, YELLOW);

    int startTextWidth = MeasureText("START SIMULATION", 32);
    DrawText("START SIMULATION", startBtn.x + startBtn.width / 2 - startTextWidth / 2,
        startBtn.y + startBtn.height / 2 - 16, 32, WHITE);

    DrawText("Use +/- to adjust values", screenWidth / 2 - MeasureText("Use +/- to adjust values", 16) / 2,
        screenHeight - 60, 16, GRAY);
    DrawText("Press ESC in-game to free cursor", screenWidth / 2 - MeasureText("Press ESC in-game to free cursor", 14) / 2,
        screenHeight - 35, 14, GRAY);
}