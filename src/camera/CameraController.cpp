#pragma warning(disable: 4576)
#include "CameraController.hpp"
#include <cmath>

CameraController::CameraController(const Vector3& startPos, float startYaw, float startPitch)
    : position(startPos)
    , yaw(startYaw)
    , pitch(startPitch)
    , baseSpeed(25.0f)
    , currentSpeed(25.0f)
    , speedMode(SpeedMode::NORMAL)
    , mouseSensitivityX(0.2f)
    , mouseSensitivityY(0.2f)
    , smoothFactor(0.15f)
    , targetPosition(startPos)
    , targetYaw(startYaw)
    , targetPitch(startPitch)
{
}

void CameraController::UpdateTargetFromInput(float deltaTime) {
    Vector2 mouseDelta = GetMouseDelta();

    targetYaw -= mouseDelta.x * mouseSensitivityX;
    targetPitch -= mouseDelta.y * mouseSensitivityY;

    if (targetPitch > 85.0f) targetPitch = 85.0f;
    if (targetPitch < -85.0f) targetPitch = -85.0f;

    // Скорость с модификаторами
    float speed = baseSpeed;
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        speed = baseSpeed * 2.0f;
        speedMode = SpeedMode::FAST;
    }
    else if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        speed = baseSpeed * 0.5f;
        speedMode = SpeedMode::SLOW;
    }
    else {
        speedMode = SpeedMode::NORMAL;
    }
    currentSpeed = speed;

    // Вычисление векторов
    float yawRad = targetYaw * DEG2RAD;
    float forwardX = sinf(yawRad);
    float forwardZ = cosf(yawRad);

    float rightX = forwardZ;
    float rightZ = -forwardX;

    float moveX = 0.0f, moveZ = 0.0f;
    float moveY = 0.0f;  // ДОБАВЛЕНО для вертикального движения

    if (IsKeyDown(KEY_W)) { moveX += forwardX; moveZ += forwardZ; }
    if (IsKeyDown(KEY_S)) { moveX -= forwardX; moveZ -= forwardZ; }
    if (IsKeyDown(KEY_D)) { moveX -= rightX; moveZ -= rightZ; }
        if (IsKeyDown(KEY_A)) { moveX += rightX; moveZ += rightZ; }

    // ========== ВЕРТИКАЛЬНОЕ ДВИЖЕНИЕ ==========
    if (IsKeyDown(KEY_Q) || IsKeyDown(KEY_SPACE)) { moveY += 1.0f; }     // Q или Пробел - вверх
    if (IsKeyDown(KEY_E) || IsKeyDown(KEY_LEFT_CONTROL)) { moveY -= 1.0f; } // E или Ctrl - вниз
    // ===========================================

    // Горизонтальное движение
    if (fabs(moveX) > 0.01f || fabs(moveZ) > 0.01f) {
        float len = sqrtf(moveX * moveX + moveZ * moveZ);
        moveX /= len;
        moveZ /= len;
        targetPosition.x += moveX * currentSpeed * deltaTime;
        targetPosition.z += moveZ * currentSpeed * deltaTime;
    }

    // Вертикальное движение
    if (fabs(moveY) > 0.01f) {
        targetPosition.y += moveY * currentSpeed * deltaTime;
    }
}

void CameraController::ApplySmoothing(float deltaTime) {
    float t = smoothFactor * deltaTime * 60.0f;
    if (t > 1.0f) t = 1.0f;

    position.x = position.x * (1.0f - t) + targetPosition.x * t;
    position.y = position.y * (1.0f - t) + targetPosition.y * t;
    position.z = position.z * (1.0f - t) + targetPosition.z * t;

    yaw = yaw * (1.0f - t) + targetYaw * t;
    pitch = pitch * (1.0f - t) + targetPitch * t;
}

void CameraController::Update(float deltaTime, Camera3D& camera, bool mouseCaptured) {
    if (mouseCaptured) {
        UpdateTargetFromInput(deltaTime);
    }
    ApplySmoothing(deltaTime);

    camera.position = position;

    float pitchRad = pitch * DEG2RAD;
    float yawRad = yaw * DEG2RAD;
    camera.target.x = position.x + sinf(yawRad) * cosf(pitchRad);
    camera.target.y = position.y + sinf(pitchRad);
    camera.target.z = position.z + cosf(yawRad) * cosf(pitchRad);
}

void CameraController::SetPosition(const Vector3& newPos) {
    position = newPos;
    targetPosition = newPos;
}

const char* CameraController::GetSpeedModeText() const {
    switch (speedMode) {
    case SpeedMode::FAST:  return "FAST (x2)";
    case SpeedMode::SLOW:  return "SLOW (x0.5)";
    default:               return "NORMAL";
    }
}