#include "Render/ControlCamera.h"
#include <glm/gtc/matrix_transform.hpp>

ControlCamera::ControlCamera()
{
    target = glm::vec3(0.0f, 0.0f, 0.0f);
    distance = 300.0f;

    yaw = -90.0f;
    pitch = 30.0f;
}

//glm::mat4 ControlCamera::getViewMatrix()
//{
  //  glm::vec3 position;

//    position.x = target.x + distance * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
 //   position.y = target.y + distance * sin(glm::radians(pitch));
//    position.z = target.z + distance * cos(glm::radians(pitch)) * sin(glm::radians(yaw));

 //   return glm::lookAt(position, target, glm::vec3(0, 1, 0));
//}
glm::mat4 ControlCamera::getViewMatrix()
{
    // =====================================================
    // FRONT VIEW
    //
    // Camera sits on +Z and looks toward the target.
    // Y remains screen-up.
    // =====================================================

    if (view == CameraView::Front)
    {
        const glm::vec3 position =
            target
            + glm::vec3(
                0.0f,
                0.0f,
                distance
            );

        return glm::lookAt(
            position,
            target,
            glm::vec3(
                0.0f,
                1.0f,
                0.0f
            )
        );
    }


    // =====================================================
    // LEFT VIEW
    //
    // Camera sits on -X and looks toward +X.
    // =====================================================

    if (view == CameraView::Left)
    {
        const glm::vec3 position =
            target
            + glm::vec3(
                -distance,
                0.0f,
                0.0f
            );

        return glm::lookAt(
            position,
            target,
            glm::vec3(
                0.0f,
                1.0f,
                0.0f
            )
        );
    }


    // =====================================================
    // TOP VIEW
    //
    // Camera sits on +Y and looks downward.
    //
    // IMPORTANT:
    //
    // We cannot use Y as the up vector here because
    // viewing direction is already parallel to Y.
    //
    // Use -Z as screen-up instead.
    // =====================================================

    if (view == CameraView::Top)
    {
        const glm::vec3 position =
            target
            + glm::vec3(
                0.0f,
                distance,
                0.0f
            );

        return glm::lookAt(
            position,
            target,
            glm::vec3(
                0.0f,
                0.0f,
                -1.0f
            )
        );
    }


    // =====================================================
    // NORMAL PERSPECTIVE / ORBIT VIEW
    // =====================================================

    glm::vec3 position;

    position.x =
        target.x
        + distance
        * cos(glm::radians(pitch))
        * cos(glm::radians(yaw));

    position.y =
        target.y
        + distance
        * sin(glm::radians(pitch));

    position.z =
        target.z
        + distance
        * cos(glm::radians(pitch))
        * sin(glm::radians(yaw));

    return glm::lookAt(
        position,
        target,
        glm::vec3(
            0.0f,
            1.0f,
            0.0f
        )
    );
}
//glm::mat4 ControlCamera::getProjection(float width, float height)
//{
 //   return glm::perspective(glm::radians(45.0f), width / height, 0.1f, 2000.0f);
//}
glm::mat4 ControlCamera::getProjection(
    float width,
    float height)
{
    if (width <= 0.0f
        || height <= 0.0f)
    {
        return glm::mat4(1.0f);
    }

    const float aspect =
        width / height;

    // =====================================================
    // ORTHOGRAPHIC VIEW SIZE
    //
    // distance is now also used as orthographic zoom.
    //
    // Larger distance:
    //     more geometry visible
    //
    // Smaller distance:
    //     zoom in
    // =====================================================

    const float halfHeight =
        distance;

    const float halfWidth =
        halfHeight * aspect;

    // Large depth range because workshop geometry
    // can be several metres long.
    const float nearPlane =
        -10000.0f;

    const float farPlane =
        10000.0f;

    return glm::ortho(
        -halfWidth,
        halfWidth,
        -halfHeight,
        halfHeight,
        nearPlane,
        farPlane
    );
}



void ControlCamera::processMouseMovement(float dx, float dy)
{


    // Mouse orbit leaves an exact engineering view.
    view =
        CameraView::Perspective;
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
//void ControlCamera::processPan(float dx, float dy)
//{
//    float panSpeed = 0.5f;

    // compute camera position (same as in getViewMatrix)
//    glm::vec3 position;

//    position.x = target.x + distance * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
//    position.y = target.y + distance * sin(glm::radians(pitch));
//    position.z = target.z + distance * cos(glm::radians(pitch)) * sin(glm::radians(yaw));

//    glm::vec3 forward = glm::normalize(target - position);
//    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
//    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    // move target
//    target += (-right * dx + up * dy) * panSpeed;
//}
// 
// 
// 
void ControlCamera::processPan(
    float dx,
    float dy)
{
    const float panSpeed =
        0.5f;

    const glm::vec3 position =
        getPosition();

    const glm::vec3 forward =
        glm::normalize(
            target
            - position
        );

    glm::vec3 cameraUp;

    if (view == CameraView::Top)
    {
        cameraUp =
            glm::vec3(
                0.0f,
                0.0f,
                -1.0f
            );
    }
    else
    {
        cameraUp =
            glm::vec3(
                0.0f,
                1.0f,
                0.0f
            );
    }

    const glm::vec3 right =
        glm::normalize(
            glm::cross(
                forward,
                cameraUp
            )
        );

    const glm::vec3 up =
        glm::normalize(
            glm::cross(
                right,
                forward
            )
        );

    target +=
        (
            -right * dx
            + up * dy
            )
        * panSpeed;
}
//glm::vec3 ControlCamera::getPosition() const
//{
 //   glm::vec3 position;

//    position.x = target.x + distance * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
//    position.y = target.y + distance * sin(glm::radians(pitch));
//    position.z = target.z + distance * cos(glm::radians(pitch)) * sin(glm::radians(yaw));

  //  return position;
//}

glm::vec3 ControlCamera::getPosition() const
{
    if (view == CameraView::Front)
    {
        return target
            + glm::vec3(
                0.0f,
                0.0f,
                distance
            );
    }

    if (view == CameraView::Left)
    {
        return target
            + glm::vec3(
                -distance,
                0.0f,
                0.0f
            );
    }

    if (view == CameraView::Top)
    {
        return target
            + glm::vec3(
                0.0f,
                distance,
                0.0f
            );
    }

    glm::vec3 position;

    position.x =
        target.x
        + distance
        * cos(glm::radians(pitch))
        * cos(glm::radians(yaw));

    position.y =
        target.y
        + distance
        * sin(glm::radians(pitch));

    position.z =
        target.z
        + distance
        * cos(glm::radians(pitch))
        * sin(glm::radians(yaw));

    return position;
}

void ControlCamera::setPerspectiveView()
{
    view =
        CameraView::Perspective;
}


void ControlCamera::setFrontView()
{
    view =
        CameraView::Front;
}


void ControlCamera::setLeftView()
{
    view =
        CameraView::Left;
}


void ControlCamera::setTopView()
{
    view =
        CameraView::Top;
}


CameraView ControlCamera::getView() const
{
    return view;
}