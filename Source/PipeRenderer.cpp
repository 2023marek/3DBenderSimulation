#include "Render/PipeRenderer.h"
#include <glad/gl.h>
#include <iostream>

// =========================
// CONSTRUCTOR / DESTRUCTOR
// =========================

PipeRenderer::PipeRenderer()
{
    
}


PipeRenderer::~PipeRenderer()
{
    if (lineVBO) glDeleteBuffers(1, &lineVBO);
    if (meshVBO) glDeleteBuffers(1, &meshVBO);
    if (meshEBO) glDeleteBuffers(1, &meshEBO);

    if (lineVAO) glDeleteVertexArrays(1, &lineVAO);
    if (meshVAO) glDeleteVertexArrays(1, &meshVAO);
}

void PipeRenderer::init()
{
    setupBuffers();  // now safe
    std::cout << "[GL] init PipeRenderer\n";
}

// =========================
// SETUP
// =========================

void PipeRenderer::setupLineBuffers()
{
    glad_glGenVertexArrays(1, &lineVAO);
    glad_glGenBuffers(1, &lineVBO);

    glad_glBindVertexArray(lineVAO);
    glad_glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

    // Position only (raw floats)
    glad_glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        3 * sizeof(float),  // stride for positions
        (void*)0);
    glad_glEnableVertexAttribArray(0);

    glad_glBindVertexArray(0);
}

void PipeRenderer::setupMeshBuffers()
{
    glad_glGenVertexArrays(1, &meshVAO);
    glad_glGenBuffers(1, &meshVBO);
    glad_glGenBuffers(1, &meshEBO);

    glad_glBindVertexArray(meshVAO);
    glad_glBindBuffer(GL_ARRAY_BUFFER, meshVBO);

    // Position: 3 floats (0-11 bytes)
    glad_glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        sizeof(TubeMesh::Vertex),
        (void*)offsetof(TubeMesh::Vertex, position));
    glad_glEnableVertexAttribArray(0);

    // Normal: 3 floats (12-23 bytes)
    glad_glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
        sizeof(TubeMesh::Vertex),
        (void*)offsetof(TubeMesh::Vertex, normal));
    glad_glEnableVertexAttribArray(1);

    glad_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshEBO);
    glad_glBindVertexArray(0);
}

void PipeRenderer::setupBuffers()
{
    setupLineBuffers();
    setupMeshBuffers();
}

void PipeRenderer::uploadLine(const std::vector<float>& vertices)
{
    vertexCount = vertices.size() / 3;

    lineRanges.clear();

    if (vertexCount > 0)
    {
        lineRanges.push_back({
            0,
            static_cast<int>(vertexCount)
            });
    }

    glad_glBindVertexArray(lineVAO);
    glad_glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

    glad_glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.empty() ? nullptr : vertices.data(),
        GL_DYNAMIC_DRAW);

    glad_glBindVertexArray(0);
}
void PipeRenderer::uploadLineStrips(
    const std::vector<std::vector<float>>& strips)
{
    // =====================================================
    // MULTI-STRIP LINE UPLOAD
    //
    // Used by Manufacturing mode.
    //
    // Each strip is drawn independently with GL_LINE_STRIP.
    // This prevents OpenGL from connecting zones together.
    // =====================================================

    std::vector<float> combined;
    lineRanges.clear();

    int first = 0;

    for (const auto& strip : strips)
    {
        int count =
            static_cast<int>(strip.size() / 3);

        if (count < 2)
            continue;

        lineRanges.push_back({
            first,
            count
            });

        combined.insert(
            combined.end(),
            strip.begin(),
            strip.end()
        );

        first += count;
    }

    vertexCount =
        static_cast<size_t>(first);

    glad_glBindVertexArray(lineVAO);
    glad_glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

    glad_glBufferData(GL_ARRAY_BUFFER,
        combined.size() * sizeof(float),
        combined.empty() ? nullptr : combined.data(),
        GL_DYNAMIC_DRAW);

    glad_glBindVertexArray(0);
}

void PipeRenderer::uploadMesh(
    const std::vector<TubeMesh::Vertex>& vertices,
    const std::vector<unsigned int>& indices)
{
    vertexCount = vertices.size();
    indexCount = indices.size();

    glad_glBindVertexArray(meshVAO);

    // Upload interleaved vertex data (position + normal)
    glad_glBindBuffer(GL_ARRAY_BUFFER, meshVBO);
    glad_glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(TubeMesh::Vertex),
        vertices.data(),
        GL_DYNAMIC_DRAW);

    // Upload indices
    glad_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshEBO);
    glad_glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_DYNAMIC_DRAW);

    glad_glBindVertexArray(0);
}

void PipeRenderer::draw()
{
    if (mode == RenderMode::LINE)
    {
        glBindVertexArray(lineVAO);

        for (const auto& range : lineRanges)
        {
            if (range.count >= 2)
            {
                glDrawArrays(
                    GL_LINE_STRIP,
                    range.first,
                    range.count
                );
            }
        }
    }
    else
    {
        glad_glBindVertexArray(meshVAO);
        glad_glDrawElements(GL_TRIANGLES, (GLsizei)indexCount,
            GL_UNSIGNED_INT, 0);
    }

    glad_glBindVertexArray(0);
}

void PipeRenderer::setMode(RenderMode m)
{
    mode = m;
}