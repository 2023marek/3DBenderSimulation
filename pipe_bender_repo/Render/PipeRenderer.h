#pragma once

#include <vector>
#include "../Core/PipeAxis3D.h"
#include "RenderMode.h"       

class PipeRenderer
{
public:
    // =========================
    // LIFECYCLE
    // =========================
    PipeRenderer();
    ~PipeRenderer();

    // =========================
    // MODE
    // =========================
    void setMode(RenderMode m);

    // =========================
    // UPLOAD DATA
    // =========================
    void uploadLine(const std::vector<float>& vertices);

    void uploadMesh(const std::vector<float>& vertices,
        const std::vector<float>& normals,
        const std::vector<unsigned int>& indices);

    // =========================
    // DRAW
    // =========================
    void draw();

private:
    void setupBuffers();

private:
    RenderMode mode = RenderMode::LINE;

    unsigned int VAO = 0;
    unsigned int VBO = 0; // positions
    unsigned int NBO = 0; // normals
    unsigned int EBO = 0; // indices

    size_t vertexCount = 0;
    size_t indexCount = 0;
};