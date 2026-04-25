#include "Render/ControlCamera.h"
#include <glm/gtc/matrix_transform.hpp>

ControlCamera::ControlCamera()
{
    target = glm::vec3(0.0f, 0.0f, 0.0f);
    distance = 300.0f;

    yaw = -90.0f;
    pitch = 30.0f;
}

glm::mat4 ControlCamera::getViewMatrix()
{
    glm::vec3 position;

    position.x = target.x + distance * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    position.y = target.y + distance * sin(glm::radians(pitch));
    position.z = target.z + distance * cos(glm::radians(pitch)) * sin(glm::radians(yaw));

    return glm::lookAt(position, target, glm::vec3(0, 1, 0));
}

glm::mat4 ControlCamera::getProjection(float width, float height)
{
    return glm::perspective(glm::radians(45.0f), width / height, 0.1f, 2000.0f);
}

void ControlCamera::processMouseMovement(float dx, float dy)
{
    float sensitivity = 0.3f;

    yaw += dx * sensitivity;
    pitch += dy * sensitivity;

    // clamp pitch (avoid flipping)
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

void ControlCamera::processScroll(float offset)
{
    distance -= offset * 10.0f;

    if (distance < 10.0f) distance = 10.0f;
    if (distance > 2000.0f) distance = 2000.0f;
}
void ControlCamera::processPan(float dx, float dy)
{
    float panSpeed = 0.5f;

    // compute camera position (same as in getViewMatrix)
    glm::vec3 position;

    position.x = target.x + distance * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    position.y = target.y + distance * sin(glm::radians(pitch));
    position.z = target.z + distance * cos(glm::radians(pitch)) * sin(glm::radians(yaw));

    glm::vec3 forward = glm::normalize(target - position);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    // move target
    target += (-right * dx + up * dy) * panSpeed;
}
glm::vec3 ControlCamera::getPosition() const
{
    glm::vec3 position;

    position.x = target.x + distance * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    position.y = target.y + distance * sin(glm::radians(pitch));
    position.z = target.z + distance * cos(glm::radians(pitch)) * sin(glm::radians(yaw));

    return position;
}