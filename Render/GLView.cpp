#include "GLView.h"
#include "Render/ShaderGL.h"
#include <QOpenGLContext>
#include <iostream>

// =========================
// SIMPLE SHADERS (GPU PROGRAM)
// =========================
static const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0); // pass position directly
}
)";

static const char* fragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(0.2, 0.9, 0.3, 1.0); // green color
}
)";


// =========================
// CONNECT PIPE DATA
// =========================
void GLView::setPipe(const PipeAxis3D* p)
{
    pipe = p;
}


// =========================
// INITIALIZE OPENGL (RUNS ONCE)
// =========================
void GLView::initializeGL()
{
    // Get current OpenGL context from Qt
    QOpenGLContext* ctx = QOpenGLContext::currentContext();

    if (!ctx)
    {
        std::cout << "No OpenGL context!\n";
        return;
    }

    // =========================
    // LOAD GLAD (function pointers)
    // =========================
    auto loader = [](const char* name) -> void*
        {
            return reinterpret_cast<void*>(
                QOpenGLContext::currentContext()->getProcAddress(name)
                );
        };

    if (!gladLoadGLLoader((GLADloadproc)loader))
    {
        std::cout << "Failed to initialize GLAD\n";
        return;
    }

    // =========================
    // CREATE SHADER (YOUR CLASS)
    // =========================
    shader = new ShaderGL(vertexShaderSrc, fragmentShaderSrc);

    // =========================
    // CREATE TRIANGLE DATA (GPU BUFFER)
    // =========================
    float vertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };

    // VAO = "how to interpret vertex data"
    glGenVertexArrays(1, &VAO);

    // VBO = actual vertex data stored on GPU
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Tell GPU: attribute 0 = vec3 position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // =========================
    // BASIC SETTINGS
    // =========================
    glEnable(GL_DEPTH_TEST);       // enable depth (3D)
    glClearColor(0, 0, 0, 1);      // black background
}


// =========================
// RENDER FRAME (RUNS EVERY FRAME)
// =========================
void GLView::paintGL()
{
    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Activate shader program
    if (shader)
        shader->use();

    // Bind geometry
    glBindVertexArray(VAO);

    // Draw triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);
}