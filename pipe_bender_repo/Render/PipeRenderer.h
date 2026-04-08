#pragma once
#include <vector>
#include "../Core/PipeAxis3D.h"

class PipeRenderer
{
public:
    PipeRenderer();

    void update(const PipeAxis3D& pipe);
    void draw();

private:
    unsigned int VAO, VBO;
    int vertexCount = 0;
};