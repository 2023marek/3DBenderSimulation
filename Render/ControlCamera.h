#pragma once
#include <glm/glm.hpp>

enum class CameraView
{
    Perspective,
    Front,
    Left,
    Top
};

class ControlCamera
{
public:
    glm::vec3 target;

    float distance;
    float yaw;
    float pitch;

    ControlCamera();

    glm::mat4 getViewMatrix();
    glm::mat4 getProjection(float width, float height);
    glm::vec3 getPosition() const;
    
    void processMouseMovement(float dx, float dy);
    void processScroll(float offset);
    void processPan(float dx, float dy);

    // =====================================================
   // STANDARD ENGINEERING VIEWS
   // =====================================================

    void setPerspectiveView();
    void setFrontView();
    void setLeftView();
    void setTopView();

    CameraView getView() const;

private:
    CameraView view =
        CameraView::Perspective;
    bool rebuildDebugStretchHelixContactGeometry();
};
