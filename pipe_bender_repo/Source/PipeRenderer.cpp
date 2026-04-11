#include "../Render/PipeRenderer.h"
#include "../Render/RenderMode.h"
#include <glad/glad.h>

// ==========================================================
// CONSTRUCTOR / DESTRUCTOR
// ==========================================================

PipeRenderer::PipeRenderer()
{
    setupBuffers();
}

PipeRenderer::~PipeRenderer()
{
    // Clean GPU resources
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &NBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}

// ==========================================================
// INTERNAL: BUFFER SETUP
// ==========================================================
/*
    GPU LAYOUT:

    VAO
     ??? VBO (positions) ? location = 0
     ??? NBO (normals)   ? location = 1
     ??? EBO (indices)

    ASCII:

        [VAO]
         ?
         ??? (0) POSITION ? VBO
         ??? (1) NORMAL   ? NBO
         ??? INDICES      ? EBO
*/

void PipeRenderer::setupBuffers()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &NBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // =========================
    // POSITION ATTRIBUTE (0)
    // =========================
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(
        0,                  // location
        3,                  // vec3
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        (void*)0
    );
    glEnableVertexAttribArray(0);

    // =========================
    // NORMAL ATTRIBUTE (1)
    // =========================
    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    glVertexAttribPointer(
        1,                  // location
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        (void*)0
    );
    glEnableVertexAttribArray(1);

    // IMPORTANT: EBO is stored inside VAO state
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glBindVertexArray(0);
}

// ==========================================================
// MODE
// ==========================================================

void PipeRenderer::setMode(RenderMode m)
{
    mode = m;
}

// ==========================================================
// UPLOAD: LINE DATA
// ==========================================================
/*
    Expected input:
    vertices = [x,y,z, x,y,z, ...]

    No indices needed (GL_LINE_STRIP)
*/

void PipeRenderer::uploadLine(const std::vector<float>& vertices)
{
    mode = RenderMode::LINE;
    vertexCount = vertices.size() / 3;

    glBindVertexArray(VAO);

    // Upload positions
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_DYNAMIC_DRAW
    );

    // No normals / indices needed for line mode
}

// ==========================================================
// UPLOAD: MESH DATA
// ==========================================================
/*
    Expected:
    vertices = positions
    normals  = normals
    indices  = triangle indices

    ASCII:

        vertices:  v0 v1 v2 v3 ...
        normals:   n0 n1 n2 n3 ...
        indices:   0  1  2  | 2 3 0
*/

void PipeRenderer::uploadMesh(
    const std::vector<float>& vertices,
    const std::vector<float>& normals,
    const std::vector<unsigned int>& indices)
{
    mode = RenderMode::MESH;

    vertexCount = vertices.size() / 3;
    indexCount = indices.size();

    glBindVertexArray(VAO);

    // =========================
    // POSITIONS
    // =========================
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_DYNAMIC_DRAW
    );

    // =========================
    // NORMALS
    // =========================
    glBindBuffer(GL_ARRAY_BUFFER, NBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        normals.size() * sizeof(float),
        normals.data(),
        GL_DYNAMIC_DRAW
    );

    // =========================
    // INDICES
    // =========================
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_DYNAMIC_DRAW
    );
}

// ==========================================================
// DRAW
// ==========================================================
/*
    FINAL RENDER STEP

    main.cpp should ONLY call:
        renderer.draw();

    Everything else is hidden inside renderer.
*/

void PipeRenderer::draw()
{
    glBindVertexArray(VAO);

    if (mode == RenderMode::LINE)
    {
        // Draw pipe axis
        glDrawArrays(
            GL_LINE_STRIP,
            0,
            (GLsizei)vertexCount
        );
    }
    else
    {
        // Draw full mesh
        glDrawElements(
            GL_TRIANGLES,
            (GLsizei)indexCount,
            GL_UNSIGNED_INT,
            0
        );
    }

    glBindVertexArray(0);
}