#pragma once

#include <string>
#include <glm/glm.hpp>

class ShaderGL
{
public:
    // =========================
    // CONSTRUCTOR
    // =========================
    ShaderGL(const char* vertexSrc, const char* fragmentSrc);

    // =========================
    // USE
    // =========================
    void use() const;

    // =========================
    // UNIFORMS
    // =========================
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setFloat(const std::string& name, float value) const;

private:
    unsigned int programID;

private:
    unsigned int compileShader(unsigned int type, const char* src);
    void checkCompileErrors(unsigned int shader, std::string type);
};