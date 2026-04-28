#pragma once

#include <glad/glad.h>
#include <QOpenGLWidget>
#include "Core/PipeAxis3D.h"
#include "Render/ShaderGL.h"
#include "Render/ControlCamera.h"
class GLView : public QOpenGLWidget
{
    Q_OBJECT

public:
    void setPipe(const PipeAxis3D* pipe);

protected:
    void initializeGL() override;
    void paintGL() override;

private:
    ControlCamera camera;
    // =========================
    // DATA SOURCE
    // =========================
    const PipeAxis3D* pipe = nullptr;

    // =========================
    // SHADER
    // =========================
    ShaderGL* shader = nullptr;

    // =========================
    // OLD TRIANGLE (can remove later)
    // =========================
    unsigned int VAO = 0;
    unsigned int VBO = 0;

    // =========================
    // PIPE RENDERING (NEW)
    // =========================
    GLuint pipeVAO = 0;
    GLuint pipeVBO = 0;
    int pipeVertexCount = 0;

    void uploadPipeGeometry();   // ?? REQUIRED
};