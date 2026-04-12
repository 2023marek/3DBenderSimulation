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
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // =========================
    // INTERLEAVED VERTEX BUFFER
    // layout:
    // [pos.xyz | normal.xyz]
    // =========================

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // position (location = 0)
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(TubeMesh::Vertex),
        (void*)0
    );
    glEnableVertexAttribArray(0);

    // normal (location = 1)
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(TubeMesh::Vertex),
        (void*)(sizeof(Vec3D))
    );
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

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
// LINE
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
// MESH
// =========================

void PipeRenderer::uploadMesh(
    const std::vector<TubeMesh::Vertex>& vertices,
    const std::vector<unsigned int>& indices)
{
    mode = RenderMode::MESH;

    vertexCount = vertices.size();
    indexCount = indices.size();

    glBindVertexArray(VAO);

    // upload interleaved vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(TubeMesh::Vertex),
        vertices.data(),
        GL_DYNAMIC_DRAW);

    // upload indices
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
    else
    {
        glDrawElements(GL_TRIANGLES,
            (GLsizei)indexCount,
            GL_UNSIGNED_INT,
            0);
    }

    glBindVertexArray(0);
}