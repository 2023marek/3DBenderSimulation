#include "../Render/HUDPanel.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <iomanip>
#include <cmath>

HUDPanel::HUDPanel(unsigned int windowWidth, unsigned int windowHeight)
    : windowWidth(windowWidth), windowHeight(windowHeight)
{
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
    if (!visible)
        return;

    GLint oldBlendSrc, oldBlendDst;
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_BLEND_SRC, &oldBlendSrc);
    glGetIntegerv(GL_BLEND_DST, &oldBlendDst);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, windowWidth, windowHeight, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // =====================================================================
    // TOP PANEL
    // =====================================================================
    float topY = 20.0f;
    float topHeight = 100.0f;

    drawRect(10.0f, topY, windowWidth - 20.0f, topHeight, bgColor);

    // Status line
    std::ostringstream statusLine;
    statusLine << "Status: " << statusStr
        << "  |  Speed: " << std::fixed << std::setprecision(1)
        << speed << " mm/s  |  Mode: " << modeStr;
    drawText(25.0f, topY + 15.0f, statusLine.str(), textColor);

    // Operation info
    std::ostringstream opLine;
    opLine << "Op " << (currentOpIndex + 1) << "/" << totalOps << ": " << currentOpName;
    drawText(25.0f, topY + 40.0f, opLine.str(), textColor);

    // Progress bar
    drawProgressBar(25.0f, topY + 65.0f, 250.0f, 20.0f,
        currentProgress, barColor, glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));

    std::ostringstream progLine;
    progLine << std::setw(3) << (int)(currentProgress * 100) << "%";
    drawText(285.0f, topY + 67.0f, progLine.str(), textColor);

    // =====================================================================
    // BOTTOM PANEL
    // =====================================================================
    float bottomY = windowHeight - 120.0f;
    float bottomHeight = 110.0f;

    drawRect(10.0f, bottomY, windowWidth - 20.0f, bottomHeight, bgColor);

    // Position
    std::ostringstream posLine;
    posLine << "Position: (" << std::fixed << std::setprecision(1)
        << position.x << ", " << position.y << ", " << position.z << ") mm";
    drawText(25.0f, bottomY + 10.0f, posLine.str(), textColor);

    // Rotation and nodes
    std::ostringstream rotLine;
    rotLine << "Rotation: " << std::fixed << std::setprecision(1) << rotation << "deg  |  Nodes: " << nodeCount;
    drawText(25.0f, bottomY + 35.0f, rotLine.str(), textColor);

    // Time
    std::ostringstream timeLine;
    timeLine << "Time: " << std::fixed << std::setprecision(2) << currentTime << " sec";
    drawText(25.0f, bottomY + 60.0f, timeLine.str(), textColor);

    // Overall progress
    drawProgressBar(25.0f, bottomY + 85.0f, 400.0f, 15.0f,
        overallProgress, barColor, glm::vec4(0.3f, 0.3f, 0.3f, 1.0f));

    // =====================================================================
    // HELP TEXT
    // =====================================================================
    float helpY = windowHeight - 50.0f;
    drawText(15.0f, helpY, "[P]lay [Space]Pause [R]eset [M]ode [H]UD [+/-]Speed [ESC]Exit",
        glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    if (!blendEnabled)
        glDisable(GL_BLEND);
    else
        glBlendFunc(oldBlendSrc, oldBlendDst);

    glEnable(GL_DEPTH_TEST);
}

void HUDPanel::drawRect(float x, float y, float width, float height, glm::vec4 color)
{
    glDisable(GL_TEXTURE_2D);
    glColor4f(color.r, color.g, color.b, color.a);

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

void HUDPanel::drawCharacter(float x, float y, char c, glm::vec4 color)
{
    // Simple character rendering using rectangles
    // Each character is approximately 8x12 pixels

    glColor4f(color.r, color.g, color.b, color.a);

    // For now, just draw a small rectangle to represent the character
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + 8.0f, y);
    glVertex2f(x + 8.0f, y + 12.0f);
    glVertex2f(x, y + 12.0f);
    glEnd();
}

void HUDPanel::drawText(float x, float y, const std::string& text, glm::vec4 color)
{
    // Draw background for readability
    float textWidth = (float)(text.length() * 8);
    drawRect(x - 2.0f, y - 2.0f, textWidth + 4.0f, 14.0f,
        glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));

    // Draw text - currently using simple rectangles as placeholders
    // For production, integrate FreeType or similar library
    glColor4f(color.r, color.g, color.b, color.a);

    for (size_t i = 0; i < text.length(); ++i)
    {
        drawCharacter(x + (float)i * 8.0f, y, text[i], color);
    }
}