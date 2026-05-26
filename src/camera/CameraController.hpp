#pragma once
#include "raylib.h"

enum class SpeedMode { NORMAL, FAST, SLOW };

class CameraController {
private:
    float yaw;
    float pitch;
    Vector3 position;

    float baseSpeed;
    float currentSpeed;
    SpeedMode speedMode;

    float mouseSensitivityX;
    float mouseSensitivityY;

    float smoothFactor;

    Vector3 targetPosition;
    float targetYaw;
    float targetPitch;

    void UpdateTargetFromInput(float deltaTime);
    void ApplySmoothing(float deltaTime);

public:
    CameraController(const Vector3& startPos, float startYaw, float startPitch);

    void Update(float deltaTime, Camera3D& camera, bool mouseCaptured);
    void SetPosition(const Vector3& newPos);
    SpeedMode GetSpeedMode() const { return speedMode; }
    const char* GetSpeedModeText() const;
};