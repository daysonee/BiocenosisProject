#pragma warning(disable: 4576)
#include "CursorManager.hpp"

CursorManager::CursorManager()
    : isCaptured(true)
    , justReleased(false)
    , accumulatedDelta{ 0, 0 }
{
    DisableCursor();
}

void CursorManager::Capture() {
    if (!isCaptured) {
        DisableCursor();
        isCaptured = true;
        justReleased = false;
    }
}

void CursorManager::Release() {
    if (isCaptured) {
        EnableCursor();
        isCaptured = false;
        justReleased = true;
    }
}

void CursorManager::Update() {
    // Обработка Escape
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (isCaptured) Release();
    }

    // Если курсор только что освобождён — не захватываем обратно автоматически
    if (justReleased) {
        justReleased = false;
    }

    // Аккумулируем дельту мыши
    Vector2 delta = GetMouseDelta();
    if (isCaptured) {
        accumulatedDelta.x += delta.x;
        accumulatedDelta.y += delta.y;
    }
}

Vector2 CursorManager::GetAccumulatedDeltaAndReset() {
    Vector2 ret = accumulatedDelta;
    accumulatedDelta = { 0, 0 };
    return ret;
}