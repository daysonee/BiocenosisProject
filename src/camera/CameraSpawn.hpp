#pragma once
#include "raylib.h"

class CameraSpawn {
private:
    Vector3 startPosition;
    Vector3 currentOffset;
    Vector3 targetOffset;
    float   animationTimer;
    float   animationDuration;
    bool    isAnimating;
    float   targetPitch;
    float   targetYaw;
    float   startPitch;
    float   startYaw;

    static float EaseOutCubic(float t);

public:
    CameraSpawn(const Vector3& startPos, float startPitch, float startYaw);

    void StartSpawnAnimation();
    void Update(float deltaTime, Camera3D& camera);
    bool IsAnimationFinished() const;
    void ForceFinish(Camera3D& camera);
};