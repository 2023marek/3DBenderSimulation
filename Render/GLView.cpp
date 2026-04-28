

#include "Render/ControlCamera.h"
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
uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(aPos, 1.0);
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
        -0.5f, -0.2f, 0.0f,
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (shader)
        shader->use();

    // upload latest pipe data every frame
    uploadPipeGeometry();
    std::cout << "Vertices: " << pipeVertexCount << std::endl;
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjection(width(), height());
    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 mvp = proj * view * model;

    shader->setMat4("MVP", mvp);
    // draw PIPE
    if (pipeVertexCount > 1)
    {
        glBindVertexArray(pipeVAO);
        glDrawArrays(GL_LINE_STRIP, 0, pipeVertexCount);
    }
}


void GLView::uploadPipeGeometry()
{
    if (!pipe) return;

    const auto& nodes = pipe->getNodes();
    pipeVertexCount = (int)nodes.size();

    if (pipeVertexCount == 0) return;

    std::vector<float> data;
    data.reserve(pipeVertexCount * 3);

    for (const auto& n : nodes)
    {
        float scale = 0.01f;   // ?? key

        data.push_back(n.pos.x * scale);
        data.push_back(n.pos.y * scale);
        data.push_back(n.pos.z * scale);
    }

    if (pipeVAO == 0)
    {
        glGenVertexArrays(1, &pipeVAO);
        glGenBuffers(1, &pipeVBO);
    }

    glBindVertexArray(pipeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pipeVBO);

    glBufferData(GL_ARRAY_BUFFER,
        data.size() * sizeof(float),
        data.data(),
        GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}