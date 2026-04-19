#define STB_IMAGE_IMPLEMENTATION
#include "../Source/ThirdParty/stb_image.h"
#include "../Render/HUDPanel.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <iomanip>
#include <cmath>
unsigned int loadFontTexture(const std::string& path);
// ===== ADD THIS NEAR TOP OF HUDPanel.cpp =====

glm::vec2 toNDC(float x, float y, float width, float height)
{
    float nx = (x / width) * 2.0f - 1.0f;
    float ny = 1.0f - (y / height) * 2.0f;
    return { nx, ny };
}
HUDPanel::HUDPanel(unsigned int windowWidth, unsigned int windowHeight)
    : windowWidth(windowWidth), windowHeight(windowHeight)
{
    initFont();
    initQuadMesh();
}

HUDPanel::~HUDPanel()
{
    if (quadVAO != 0)
        glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO != 0)
        glDeleteBuffers(1, &quadVBO);
}

void HUDPanel::initQuadMesh()
{
    float quadVertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void HUDPanel::update(const SimulationController& simulator, const RenderMode& mode)
{
    if (!visible)
        return;

    const MachineState& state = simulator.getState();

    // Status
    if (simulator.isPlaying())
        statusStr = "PLAYING";
    else if (simulator.isPaused())
        statusStr = "PAUSED";
    else
        statusStr = "IDLE";

    speed = simulator.getSpeed();
    currentTime = state.currentTime;

    // Progress
    currentProgress = simulator.getCurrentOperationProgress();
    overallProgress = simulator.getOverallProgress();
    currentOpIndex = simulator.getCurrentOperationIndex();
    totalOps = simulator.getTotalOperations();

    // Operation names
    std::ostringstream oss;
    if (currentOpIndex < totalOps)
    {
        const OperationQueue& queue = simulator.getQueue();
        const Operation* currentOp = queue.getCurrent();
        if (currentOp)
        {
            if (currentOp->type == Operation::FEED)
            {
                oss << "FEED " << std::fixed << std::setprecision(0)
                    << currentOp->length << "mm";
            }
            else if (currentOp->type == Operation::BEND)
            {
                double angleDeg = currentOp->angle * 180.0 / 3.14159265358979323846;
                oss << "BEND R=" << std::fixed << std::setprecision(1)
                    << currentOp->R << "mm, angle=" << angleDeg << "deg";
            }
            else if (currentOp->type == Operation::ROTATE)
            {
                double angleDeg = currentOp->angle * 180.0 / 3.14159265358979323846;
                oss << "ROTATE " << std::fixed << std::setprecision(1) << angleDeg << "deg";
            }
        }
    }
    currentOpName = oss.str();

    // Machine state
    position = glm::vec3(state.position.x, state.position.y, state.position.z);
    rotation = (float)(state.rotation * 180.0 / 3.14159265358979323846);

    const PipeAxis3D& geom = simulator.getPipeGeometry();
    nodeCount = geom.getNodes().size();

    // Render mode
    modeStr = (mode == RenderMode::LINE) ? "LINE" : "MESH";
}

void HUDPanel::render()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(0);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, windowWidth, windowHeight, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(1.0f, 0.0f, 0.0f);

    //drawRect(50, 50, 300, 100, glm::vec4(0, 1, 0, 0.5f));
    drawRect(100, 200, 200, 80, glm::vec4(1, 0, 0, 1));
    drawText(100, 100, "HUD OK", glm::vec4(1, 1, 1, 1));
}

void HUDPanel::drawRect(float x, float y, float width, float height, glm::vec4 color)
{
    // ?? Force fixed pipeline
    glUseProgram(0);

    // ?? Make sure blending works
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ?? Set color
    glColor4f(color.r, color.g, color.b, color.a);

    // ?? Draw quad
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void HUDPanel::drawProgressBar(float x, float y, float width, float height,
    double progress, glm::vec4 fillColor, glm::vec4 bgColor)
{
    drawRect(x, y, width, height, bgColor);
    float filledWidth = (float)(width * progress);
    drawRect(x, y, filledWidth, height, fillColor);
}

void HUDPanel::drawCharBitmap(float x, float y, float w, float h, char c)
{
    int index = (unsigned char)c;

    int cols = 16;
    float cell = 1.0f / cols;

    int row = index / cols;
    int col = index % cols;

    float u0 = col * cell;
    float v0 = row * cell;
    float u1 = u0 + cell;
    float v1 = v0 + cell;

    glm::vec2 p0 = toNDC(x, y,windowWidth,windowHeight);
    glm::vec2 p1 = toNDC(x + w, y + h, windowWidth,windowHeight);

    float vertices[6][4] = {
        { p0.x, p1.y, u0, v1 },
        { p0.x, p0.y, u0, v0 },
        { p1.x, p0.y, u1, v0 },

        { p0.x, p1.y, u0, v1 },
        { p1.x, p0.y, u1, v0 },
        { p1.x, p1.y, u1, v1 }
    };

    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}
void HUDPanel::drawText(float x, float y, const std::string& text, glm::vec4 color)
{
    for (size_t i = 0; i < text.size(); ++i)
    {
        drawRect(
            x + i * 12.0f,   // spacing
            y,
            8.0f,
            12.0f,
            color
        );
    }
}void HUDPanel::initFont()
{
    // load texture (use your stb loader here)
    fontTexture = loadFontTexture("Assets/Fonts/8x8text_whiteNoShadow.png");

    if (fontTexture == 0)
    {
        std::cerr << "ERROR: Font texture failed to load!\n";
    }

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);

    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glBindVertexArray(0);
}
unsigned int loadFontTexture(const std::string& path)
{
    int width, height, channels;

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data)
    {
        std::cerr << "Failed to load font texture: " << path << std::endl;
        return 0;
    }

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
        0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glGenerateMipmap(GL_TEXTURE_2D);

    // Texture settings (important for fonts)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    return texture;
}

