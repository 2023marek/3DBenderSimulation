#pragma once

#include <glad/glad.h>
#include <QOpenGLWidget>
#include "Core/PipeAxis3D.h"
#include "Render/ShaderGL.h"   // ? FIX: required for ShaderGL

// Widget renderuj¹cy (nic nie wie o symulacji!)
class GLView : public QOpenGLWidget
{
    Q_OBJECT

public:
    void setPipe(const PipeAxis3D* pipe);

protected:
    void initializeGL() override;
    void paintGL() override;

private:
    // =========================
    // DATA SOURCE (from simulation)
    // =========================
    const PipeAxis3D* pipe = nullptr;

    // =========================
    // GPU RESOURCES
    // =========================
    ShaderGL* shader = nullptr;   // shader program (your class)

    unsigned int VAO = 0;         // vertex array object (layout)
    unsigned int VBO = 0;         // vertex buffer (data)

   
};