#pragma once
#include <string>

class Shader
{
public:
    unsigned int ID;

    Shader(const char* vertexSrc, const char* fragmentSrc);

    void use();

    void setMat4(const std::string& name, const float* value);
};