#pragma once

#include <glad/glad.h>
#include <QOpenGLWidget>
class AppController;
#include "Core/PipeAxis3D.h"
#include "Render/ShaderGL.h"
#include "Render/ControlCamera.h"
class GLView : public QOpenGLWidget
{
    Q_OBJECT

public:
    GLView() {
        setFocusPolicy(Qt::StrongFocus);
        camera.pitch = 20.0f;
        camera.yaw = -45.0f;
    }
    void setPipe(const PipeAxis3D* pipe);

protected:
    void initializeGL() override;
    void paintGL() override;
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    ControlCamera camera;
    QPoint lastMousePos;
    bool leftPressed = false;
    bool rightPressed = false;
    bool autoFrame = true;   // run once
    glm::vec3 computePipeCenterAndSize(float& outSize);

    // =========================
    // DATA SOURCE
    // =========================
    const PipeAxis3D* pipe = nullptr;

    // =========================
    // SHADER
    // =========================
    ShaderGL* shader = nullptr;

  

    // =========================
    // PIPE RENDERING (NEW)
    // =========================
    GLuint pipeVAO = 0;
    GLuint pipeVBO = 0;
    int pipeVertexCount = 0;

    void uploadPipeGeometry();   // 


public:
    void setAppController(AppController* a)
    {
        app = a;
    }

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    AppController* app = nullptr;
   

    
};