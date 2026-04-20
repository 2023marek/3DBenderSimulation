#include <fstream>
#define STB_IMAGE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
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

static glm::vec2 toNDC(float x, float y, float width, float height)
{
    float nx = (x / width) * 2.0f - 1.0f;
    float ny = 1.0f - (y / height) * 2.0f;
    return { nx, ny };
}
HUDPanel::HUDPanel(unsigned int windowWidth, unsigned int windowHeight)
    : windowWidth(windowWidth), windowHeight(windowHeight)
{
    
    initQuadMesh();
    loadFont("Assets/Fonts/marek.fnt", "Assets/Fonts/marek_0.png");


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

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fontTexture);

    drawText(100, 100, "HUD OK 123", glm::vec4(1, 1, 1, 1));

    glDisable(GL_TEXTURE_2D);
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


void HUDPanel::drawText(float x, float y, const std::string& text, glm::vec4 color)
{
    float cursorX = x;

    for (char c : text)
    {
        auto it = glyphs.find(c);
        if (it == glyphs.end())
        {
            cursorX += 10.0f; // fallback space for unknown chars
            continue;
        }

        Glyph& g = it->second;

        drawGlyph(cursorX, y, c);

        // IMPORTANT: use xAdvance, not width
        cursorX += g.xAdvance;
    }
}

unsigned int loadFontTexture(const std::string& path)
{
    int w, h, channels;

    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);

    if (!data)
    {
        std::cout << "FONT LOAD FAILED\n";
        return 0;
    }

    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
	std::cout << "Texture size: " << w << "x" << h << std::endl;
    return texID;
}
void HUDPanel::drawGlyph(float x, float y, char c)
{
    if (glyphs.find(c) == glyphs.end()) return;

    Glyph& g = glyphs[c];

    float x0 = x + g.xOffset;
    float y0 = y + g.yOffset;

    float x1 = x0 + g.width;
    float y1 = y0 + g.height;

    glBegin(GL_QUADS);
    glTexCoord2f(g.u0, g.v0); glVertex2f(x0, y0);
    glTexCoord2f(g.u1, g.v0); glVertex2f(x1, y0);
    glTexCoord2f(g.u1, g.v1); glVertex2f(x1, y1);
    glTexCoord2f(g.u0, g.v1); glVertex2f(x0, y1);
    glEnd();
}
unsigned int loadTexture(const std::string& path)
{
    int w, h, channels;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);

    if (!data)
    {
        std::cout << "FONT LOAD FAILED\n";
        return 0;
    }

    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return tex;
}

void HUDPanel::loadFont(const std::string& fntPath, const std::string& texturePath)
{
    fontTexture = loadTexture(texturePath);

    std::ifstream file(fntPath);
    std::string line;

    int texW = 0, texH = 0;

    while (std::getline(file, line))
    {
        // Get texture size
        if (line.find("scaleW") != std::string::npos)
        {
            sscanf_s(line.c_str(), "common lineHeight=%*d base=%*d scaleW=%d scaleH=%d",
                &texW, &texH);
        }

        // Parse glyph
        if (line.find("char id=") != std::string::npos)
        {
            int id = 0, x = 0, y = 0, w = 0, h = 0, xo = 0, yo = 0, xa = 0;

            int parsed = sscanf_s(line.c_str(),
                "char id=%d x=%d y=%d width=%d height=%d xoffset=%d yoffset=%d xadvance=%d",
                &id, &x, &y, &w, &h, &xo, &yo, &xa); 
            if (parsed != 8)
            {
                std::cout << "WARNING: Failed to parse glyph line:\n" << line << "\n";
                continue;
            }

            Glyph g;

            g.u0 = (float)x / texW;
            g.v0 = (float)y / texH;
            g.u1 = (float)(x + w) / texW;
            g.v1 = (float)(y + h) / texH;

            g.width = (float)w;
            g.height = (float)h;
            g.xOffset = (float)xo;
            g.yOffset = (float)yo;
            g.xAdvance = (float)xa;

            glyphs[(char)id] = g;
        }
    }

    std::cout << "Loaded glyphs: " << glyphs.size() << "\n";
}
