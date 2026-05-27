#pragma warning(disable: 4576)
#include "CameraSpawn.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>

float CameraSpawn::EaseOutCubic(float t) {
    float t1 = t - 1.0f;
    return t1 * t1 * t1 + 1.0f;
}

CameraSpawn::CameraSpawn(const Vector3& startPos, float startPitch, float startYaw)
    : startPosition(startPos)
    , targetPitch(startPitch)
    , targetYaw(startYaw)
    , animationDuration(0.5f)
    , isAnimating(true)
{
    // Инициализируем random
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(nullptr));
        seeded = true;
    }

    float angle = (float)(rand() % 360) * DEG2RAD;
    float radius = (float)(rand() % 100) / 10.0f; // 0-10 units

    currentOffset = { cosf(angle) * radius, (float)(rand() % 20 - 10), sinf(angle) * radius };
    targetOffset = { 0, 0, 0 };

    animationTimer = 0.0f;
}

void CameraSpawn::StartSpawnAnimation() {
    isAnimating = true;
    animationTimer = 0.0f;
}

void CameraSpawn::Update(float deltaTime, Camera3D& camera) {
    if (!isAnimating) return;

    animationTimer += deltaTime;
    float t = (animationTimer / animationDuration);
    if (t > 1.0f) t = 1.0f;
    float eased = EaseOutCubic(t);

    // Интерполируем позицию
    camera.position.x = startPosition.x + currentOffset.x * (1.0f - eased);
    camera.position.y = startPosition.y + currentOffset.y * (1.0f - eased);
    camera.position.z = startPosition.z + currentOffset.z * (1.0f - eased);

    // Интерполируем углы
    camera.target.x = startPosition.x + sinf(targetYaw * DEG2RAD) * cosf(targetPitch * DEG2RAD);
    camera.target.y = startPosition.y + sinf(targetPitch * DEG2RAD);
    camera.target.z = startPosition.z + cosf(targetYaw * DEG2RAD) * cosf(targetPitch * DEG2RAD);

    if (t >= 1.0f) {
        isAnimating = false;
        camera.position = startPosition;
        ForceFinish(camera);
    }
}

bool CameraSpawn::IsAnimationFinished() const {
    return !isAnimating;
}

void CameraSpawn::ForceFinish(Camera3D& camera) {
    camera.position = startPosition;
    float pitchRad = targetPitch * DEG2RAD;
    float yawRad = targetYaw * DEG2RAD;
    camera.target.x = startPosition.x + sinf(yawRad) * cosf(pitchRad);
    camera.target.y = startPosition.y + sinf(pitchRad);
    camera.target.z = startPosition.z + cosf(yawRad) * cosf(pitchRad);
    isAnimating = false;
}