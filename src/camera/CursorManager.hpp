#pragma once
#include "raylib.h"

class CursorManager {
private:
    bool isCaptured;
    bool justReleased;
    Vector2 accumulatedDelta;

public:
    CursorManager();
    void Update();
    void Capture();
    void Release();
    bool IsCaptured() const { return isCaptured; }
    Vector2 GetAccumulatedDeltaAndReset();
};