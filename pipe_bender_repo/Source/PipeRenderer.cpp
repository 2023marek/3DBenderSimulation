#include "../Render/PipeRenderer.h"
#include <glad/glad.h>

// =========================
// CONSTRUCTOR / DESTRUCTOR
// =========================
PipeRenderer::PipeRenderer()
{
    setupBuffers();
}

PipeRenderer::~PipeRenderer()
{
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &NBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}

// =========================
// SETUP
// =========================
void PipeRenderer::setupBuffers()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &NBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // POSITION (location = 0)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // NORMAL (location = 1)
    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// =========================
// MODE
// =========================
void PipeRenderer::setMode(RenderMode m)
{
    mode = m;
}

// =========================
// UPLOAD LINE
// =========================
void PipeRenderer::uploadLine(const std::vector<float>& vertices)
{
    mode = RenderMode::LINE;

    vertexCount = vertices.size() / 3;

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_DYNAMIC_DRAW);
}

// =========================
// UPLOAD MESH
// =========================
void PipeRenderer::uploadMesh(const std::vector<float>& vertices,
    const std::vector<float>& normals,
    const std::vector<unsigned int>& indices)
{
    mode = RenderMode::MESH;

    vertexCount = vertices.size() / 3;
    indexCount = indices.size();

    glBindVertexArray(VAO);

    // positions
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_DYNAMIC_DRAW);

    // normals
    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    glBufferData(GL_ARRAY_BUFFER,
        normals.size() * sizeof(float),
        normals.data(),
        GL_DYNAMIC_DRAW);

    // indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_DYNAMIC_DRAW);
}

// =========================
// DRAW
// =========================
void PipeRenderer::draw()
{
    glBindVertexArray(VAO);

    if (mode == RenderMode::LINE)
    {
        glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)vertexCount);
    }
    else // MESH
    {
        glDrawElements(GL_TRIANGLES, (GLsizei)indexCount, GL_UNSIGNED_INT, 0);
    }
}