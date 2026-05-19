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

    // Single line strip, used by CADPreview
    void uploadLine(const std::vector<float>& vertices);

    // Multiple line strips, used by ManufacturingPlayback
    void uploadLineStrips(const std::vector<std::vector<float>>& strips);

    void uploadMesh(const std::vector<TubeMesh::Vertex>& vertices,
        const std::vector<unsigned int>& indices);

    void draw();
    void init();

private:
    struct LineRange
    {
        int first = 0;
        int count = 0;
    };

private:
    void setupBuffers();

private:
    RenderMode mode = RenderMode::LINE;

    unsigned int lineVAO = 0;
    unsigned int meshVAO = 0;

    unsigned int lineVBO = 0;
    unsigned int meshVBO = 0;
    unsigned int meshEBO = 0;

    void setupLineBuffers();
    void setupMeshBuffers();

    size_t vertexCount = 0;
    size_t indexCount = 0;

    // For drawing several independent GL_LINE_STRIP batches
    std::vector<LineRange> lineRanges;
};