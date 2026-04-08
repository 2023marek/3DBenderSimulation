#include "../Render/PipeRenderer.h"
#include <glad/glad.h>

PipeRenderer::PipeRenderer()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void PipeRenderer::update(const PipeAxis3D& pipe)
{
    std::vector<float> vertices;

    for (const auto& n : pipe.getNodes())
    {
        vertices.push_back((float)n.pos.x);
        vertices.push_back((float)n.pos.y);
        vertices.push_back((float)n.pos.z);
    }

    vertexCount = vertices.size() / 3;

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_DYNAMIC_DRAW);
}

void PipeRenderer::draw()
{
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINE_STRIP, 0, vertexCount);
}