#pragma once

#include <vector>
#include "RenderMode.h"
#include "TubeMesh.h"

class PipeRenderer
{
public:
    PipeRenderer();
    ~PipeRenderer();

    void setMode(RenderMode m);

    // line rendering (axis)
    void uploadLine(const std::vector<float>& vertices);

    // mesh rendering (tube)
    void uploadMesh(const std::vector<TubeMesh::Vertex>& vertices,
        const std::vector<unsigned int>& indices);

    void draw();

private:
    void setupBuffers();

private:
    RenderMode mode = RenderMode::LINE;

    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    size_t vertexCount = 0;
    size_t indexCount = 0;
};