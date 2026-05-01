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
    void init(); 
private:
    void setupBuffers();
      

private:
    RenderMode mode = RenderMode::LINE;

   
private:

    // TWO separate VAOs
    unsigned int lineVAO = 0;
    unsigned int meshVAO = 0;

    unsigned int lineVBO = 0;
    unsigned int meshVBO = 0;
    unsigned int meshEBO = 0;

    // Setup functions
    void setupLineBuffers();
    void setupMeshBuffers();

    size_t vertexCount = 0;
    size_t indexCount = 0;
};