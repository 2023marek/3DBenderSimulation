#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include "Common/UserAction.h"
#include "Render/ControlCamera.h"
#include "GLView.h"
#include "App/AppController.h"
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

   
    glGenVertexArrays(1, &pipeVAO);
    glGenBuffers(1, &pipeVBO);

    glBindVertexArray(pipeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pipeVBO);

    // no data yet!
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
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

    if (!shader) return;
    shader->use();

    uploadPipeGeometry();

    if (pipe && autoFrame)
    {
        float size = 100.0f;
        glm::vec3 center = computePipeCenterAndSize(size);

        camera.target = center;
        camera.distance = size * 2.5f + 20.0f;
   
        camera.pitch = 25.0f;
        camera.yaw = -45.0f;
        autoFrame = false; // ?? only once
    }

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjection(width(), height());
    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 mvp = proj * view * model;

    shader->setMat4("MVP", mvp);

    if (pipeVertexCount > 1)
    {
        glLineWidth(2.0f);
        glBindVertexArray(pipeVAO);
        glDrawArrays(GL_LINE_STRIP, 0, pipeVertexCount);
    }
}


void GLView::uploadPipeGeometry()
{
    // ============================================================
    // 1. VALIDATION: no pipe ? nothing to send to GPU
    // ============================================================
    if (!pipe)
        return;

    const auto& nodes = pipe->getNodes();
    pipeVertexCount = static_cast<int>(nodes.size());

    // No geometry ? skip rendering this frame
    if (pipeVertexCount == 0)
        return;

    // ============================================================
    // 2. CPU ? BUILD CONTIGUOUS VERTEX ARRAY
    //    Convert PipeAxis3D nodes into flat float buffer
    //
    //    Layout: [x0 y0 z0 | x1 y1 z1 | x2 y2 z2 ...]
    //
    //    IMPORTANT:
    //    - No scaling here ? keep consistent world units (mm)
    //    - Camera will handle framing instead
    // ============================================================
    std::vector<float> data;
    data.reserve(pipeVertexCount * 3); // 3 floats per vertex

    for (const auto& n : nodes)
    {
        data.push_back(static_cast<float>(n.pos.x));
        data.push_back(static_cast<float>(n.pos.y));
        data.push_back(static_cast<float>(n.pos.z));
    }

    // ============================================================
    // 3. GPU OBJECT CREATION (RUN ONLY ONCE)
    //
    //    VAO = describes how vertex data is interpreted
    //    VBO = actual GPU memory buffer
    //
    //    This block should NOT run every frame
    // ============================================================
    if (pipeVAO == 0)
    {
        glGenVertexArrays(1, &pipeVAO);
        glGenBuffers(1, &pipeVBO);

        glBindVertexArray(pipeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pipeVBO);

        // Define vertex layout:
        // attribute 0 ? vec3 position
        glVertexAttribPointer(
            0,                      // location in shader
            3,                      // vec3
            GL_FLOAT,
            GL_FALSE,
            3 * sizeof(float),      // stride
            (void*)0                // offset
        );

        glEnableVertexAttribArray(0);

        // Unbind VAO (good practice)
        glBindVertexArray(0);
    }

    // ============================================================
    // 4. UPLOAD DATA TO GPU (EVERY FRAME)
    //
    //    We use GL_DYNAMIC_DRAW because:
    //    - geometry changes every frame (simulation)
    // ============================================================
    glBindBuffer(GL_ARRAY_BUFFER, pipeVBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        data.size() * sizeof(float),
        data.data(),
        GL_DYNAMIC_DRAW
    );

    // NOTE:
    // No need to re-bind VAO or redefine attributes here
    // VAO already "remembers" layout from step 3
}

void GLView::mousePressEvent(QMouseEvent* event)
{
    lastMousePos = event->pos();

    if (event->button() == Qt::LeftButton)
        leftPressed = true;

    if (event->button() == Qt::RightButton)
        rightPressed = true;
}

void GLView::mouseMoveEvent(QMouseEvent* event)
{
    QPoint delta = event->pos() - lastMousePos;
    lastMousePos = event->pos();

    if (leftPressed)
    {
        // ORBIT
        camera.processMouseMovement(delta.x(), -delta.y());
    }
    else if (rightPressed)
    {
        // PAN
        camera.processPan(delta.x(), -delta.y());
    }

    update(); // trigger repaint
}
void GLView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        leftPressed = false;

    if (event->button() == Qt::RightButton)
        rightPressed = false;
}

void GLView::wheelEvent(QWheelEvent* event)
{
    float delta = event->angleDelta().y() / 120.0f;
    camera.processScroll(delta);

    update();
}


glm::vec3 GLView::computePipeCenterAndSize(float& outSize)
{
    const auto& nodes = pipe->getNodes();

    if (nodes.empty())
    {
        outSize = 100.0f;
        return glm::vec3(0.0f);
    }

    glm::vec3 minP(1e9f);
    glm::vec3 maxP(-1e9f);

    for (const auto& n : nodes)
    {
        glm::vec3 p(n.pos.x, n.pos.y, n.pos.z);

        minP = glm::min(minP, p);
        maxP = glm::max(maxP, p);
    }

    glm::vec3 center = (minP + maxP) * 0.5f;

    glm::vec3 sizeVec = maxP - minP;
    outSize = glm::length(sizeVec);

    return center;
}



void GLView::keyPressEvent(QKeyEvent* event)
{
    std::cout << "[KEY PRESSED] key=" << event->key() << std::endl;
    std::cout << "[PLAY CALLED] this=" << this << std::endl;
    if (!app) return;
    
    switch (event->key())
    {
    case Qt::Key_Space:
        std::cout << "[KEY] SPACE ? PLAY\n";
        app->handleAction(UserAction::Play);
        break;

    case Qt::Key_P:
        std::cout << "[KEY] P ? PAUSE\n";
        app->handleAction(UserAction::Pause);
        break;

    case Qt::Key_R:
        std::cout << "[KEY] R ? RESET\n";
        app->handleAction(UserAction::Reset);
        break;

    case Qt::Key_S:
        std::cout << "[KEY] S ? STEP\n";
        app->handleAction(UserAction::Step);
        break;
    }

    update(); // refresh screen
}