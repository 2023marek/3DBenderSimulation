#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <map>
#include "../Core/SimulationController.h"
#include "../Render/RenderMode.h"
#include "../Render/ShaderManager.h"


// =========================================================================
// PHASE 4B+: HUD PANEL - WITH REAL TEXT RENDERING
// =========================================================================

class HUDPanel
{
public:
    HUDPanel(unsigned int windowWidth, unsigned int windowHeight);
    ~HUDPanel();

    // Update HUD data from simulator
    void update(const SimulationController& simulator, const RenderMode& mode);

    // Render HUD to screen
    void render();

    // Configuration
    void setVisible(bool visible) { this->visible = visible; }
    bool isVisible() const { return visible; }

    void setTextColor(glm::vec4 color) { textColor = color; }
    void setBarColor(glm::vec4 color) { barColor = color; }
    void setBackgroundAlpha(float alpha) { bgAlpha = alpha; }
    void setTextShader(ShaderGL* shader) { textShader = shader; }
private:
    unsigned int windowWidth;
    unsigned int windowHeight;
    
    bool visible = true;

    glm::vec4 textColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    glm::vec4 barColor = glm::vec4(0.2f, 0.8f, 0.3f, 1.0f);
    glm::vec4 bgColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f);
    float bgAlpha = 0.5f;
	// Glyph mapping 
    struct Glyph
	{
        float u0, v0, u1, v1;
        float width;
        float height;
		float xOffset;
		float yOffset;
		float xAdvance;
	};

	std::unordered_map<char, Glyph> glyphs;
    unsigned int fontTexture = 0;

	void loadFont(const std::string& fntPath, const std::string& texturePath);
    void drawGlyph(float x, float y, char c);
    // HUD Data
    std::string statusStr;
    double speed = 0.0;
    double currentTime = 0.0;
    double currentProgress = 0.0;
    double overallProgress = 0.0;
    int currentOpIndex = 0;
    int totalOps = 0;
    std::string currentOpName;
    glm::vec3 position;
    float rotation = 0.0f;
    int nodeCount = 0;
    std::string modeStr;

    

    ShaderGL* textShader = nullptr;
    // Rendering
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;

    void initQuadMesh();
    
    ShaderGL* hudShader = nullptr;

public:
    void setHUDShader(ShaderGL* shader) { hudShader = shader; }

    // Simple character renderer
    void drawCharacter(float x, float y, char c, glm::vec4 color);
    void drawText(float x, float y, const std::string& text, glm::vec4 color);

   
    unsigned int textVAO = 0;
    unsigned int textVBO = 0;

  
    
};