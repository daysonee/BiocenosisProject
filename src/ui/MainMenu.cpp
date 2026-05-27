#include "MainMenu.hpp"

void MainMenu::DrawButton(Rectangle rect, const char* text, Color color, bool isHovered) {
    Color btnColor = isHovered ? Color{ color.r, color.g, color.b, 200 } : color;
    DrawRectangleRec(rect, btnColor);
    DrawRectangleLinesEx(rect, 2, DARKGRAY);

    int textWidth = MeasureText(text, 28);
    DrawText(text, rect.x + rect.width / 2 - textWidth / 2,
        rect.y + rect.height / 2 - 14, 28, BLACK);
}

void MainMenu::DrawNumberButton(Rectangle rect, const char* text, bool isHovered) {
    Color color = isHovered ? LIGHTGRAY : RAYWHITE;
    DrawRectangleRec(rect, color);
    DrawRectangleLinesEx(rect, 2, DARKGRAY);

    int textWidth = MeasureText(text, 30);
    DrawText(text, rect.x + rect.width / 2 - textWidth / 2,
        rect.y + rect.height / 2 - 15, 30, BLACK);
}

void MainMenu::DrawValueBox(Rectangle rect, int value, const char* label, bool isHovered) {
    Color color = isHovered ? SKYBLUE : RAYWHITE;
    DrawRectangleRec(rect, color);
    DrawRectangleLinesEx(rect, 2, DARKGRAY);

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s: %d", label, value);
    int textWidth = MeasureText(buffer, 24);
    DrawText(buffer, rect.x + rect.width / 2 - textWidth / 2,
        rect.y + rect.height / 2 - 12, 24, DARKBLUE);
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
    if (pulseDirection) {
        titlePulse += 0.02f;
        if (titlePulse >= 1.0f) pulseDirection = false;
    }
    else {
        titlePulse -= 0.02f;
        if (titlePulse <= 0.0f) pulseDirection = true;
    }

    Vector2 mousePos = GetMousePosition();
    bool leftClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Овцы
    bool decHover = CheckCollisionPointRec(mousePos, sheepBtnDec);
    bool incHover = CheckCollisionPointRec(mousePos, sheepBtnInc);

    if (leftClicked && decHover && settings.sheepCount > SHEEP_MIN) {
        settings.sheepCount -= 10;
        if (settings.sheepCount < SHEEP_MIN) settings.sheepCount = SHEEP_MIN;
    }
    if (leftClicked && incHover && settings.sheepCount < SHEEP_MAX) {
        settings.sheepCount += 10;
        if (settings.sheepCount > SHEEP_MAX) settings.sheepCount = SHEEP_MAX;
    }

    // Волки
    decHover = CheckCollisionPointRec(mousePos, wolfBtnDec);
    incHover = CheckCollisionPointRec(mousePos, wolfBtnInc);

    if (leftClicked && decHover && settings.wolfCount > WOLF_MIN) {
        settings.wolfCount--;
    }
    if (leftClicked && incHover && settings.wolfCount < WOLF_MAX) {
        settings.wolfCount++;
    }

    // Трава
    decHover = CheckCollisionPointRec(mousePos, grassBtnDec);
    incHover = CheckCollisionPointRec(mousePos, grassBtnInc);

    if (leftClicked && decHover && settings.grassCount > GRASS_MIN) {
        settings.grassCount -= 50;
        if (settings.grassCount < GRASS_MIN) settings.grassCount = GRASS_MIN;
    }
    if (leftClicked && incHover && settings.grassCount < GRASS_MAX) {
        settings.grassCount += 50;
        if (settings.grassCount > GRASS_MAX) settings.grassCount = GRASS_MAX;
    }

    bool startHover = CheckCollisionPointRec(mousePos, startBtn);
    if (leftClicked && startHover) {
        settings.readyToStart = true;
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