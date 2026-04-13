#pragma once

#include <string>
#include <glm/glm.hpp>

class ShaderGL
{
public:
    ShaderGL(const char* vertexSrc, const char* fragmentSrc);

    void use() const;

    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setMat3(const std::string& name, const glm::mat3& mat) const;  // ? ADD THIS
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setFloat(const std::string& name, float value) const;

    unsigned int getID() const { return programID; }

private:
    unsigned int programID;

    unsigned int compileShader(unsigned int type, const char* src);
    void checkCompileErrors(unsigned int obj, std::string type);
};