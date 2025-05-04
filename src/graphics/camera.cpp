#include "camera.h"
#include "maths.h"

Camera::Camera()
    : position(0.0f, 0.0f, 5.0f), target(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f),
      fov(3.14159f / 4.0f), aspect(4.0f / 3.0f), nearPlane(0.1f), farPlane(100.0f) {}

void Camera::setPosition(const Vec3f &position) { this->position = position; }
void Camera::setTarget(const Vec3f &target) { this->target = target; }
void Camera::setUp(const Vec3f &up) { this->up = up; }
void Camera::setFOV(float fov) { this->fov = fov; }
void Camera::setAspect(float aspect) { this->aspect = aspect; }
void Camera::setNearPlane(float nearPlane) { this->nearPlane = nearPlane; }
void Camera::setFarPlane(float farPlane) { this->farPlane = farPlane; }

Vec3f Camera::getPosition() const { return position; }
Vec3f Camera::getTarget() const { return target; }
Vec3f Camera::getUp() const { return up; }
float Camera::getFOV() const { return fov; }
float Camera::getAspect() const { return aspect; }
float Camera::getNearPlane() const { return nearPlane; }
float Camera::getFarPlane() const { return farPlane; }

Matrix4x4f Camera::getViewMatrix() const { return Matrix4x4f::lookAt(position, target, up); }
Matrix4x4f Camera::getProjectionMatrix() const { return Matrix4x4f::perspective(fov, aspect, nearPlane, farPlane); }