
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "../Render/ShaderGL.h"

const char* vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(aPos, 1.0);
}
)";
const char* fragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(0.2, 0.8, 0.3, 1.0);
}
)";

Shader::Shader(const char* vertexSrc, const char* fragmentSrc)
{
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSrc, NULL);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSrc, NULL);
    glCompileShader(fs);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}