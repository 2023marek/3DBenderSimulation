#include "../Render/ShaderGL.h"
#include <glad/glad.h>
#include <iostream>

ShaderGL::ShaderGL(const char* vertexSrc, const char* fragmentSrc)
{
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    programID = glCreateProgram();
    glAttachShader(programID, vs);
    glAttachShader(programID, fs);
    glLinkProgram(programID);

    checkCompileErrors(programID, "PROGRAM");

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void ShaderGL::use() const
{
    glUseProgram(programID);
}

void ShaderGL::setMat4(const std::string& name, const glm::mat4& mat) const
{
    int loc = glGetUniformLocation(programID, name.c_str());
    if (loc != -1)
        glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);
}

void ShaderGL::setMat3(const std::string& name, const glm::mat3& mat) const
{
    int loc = glGetUniformLocation(programID, name.c_str());
    if (loc != -1)
        glUniformMatrix3fv(loc, 1, GL_FALSE, &mat[0][0]);
}

void ShaderGL::setVec3(const std::string& name, const glm::vec3& value) const
{
    int loc = glGetUniformLocation(programID, name.c_str());
    if (loc != -1)
        glUniform3f(loc, value.x, value.y, value.z);
}

void ShaderGL::setFloat(const std::string& name, float value) const
{
    int loc = glGetUniformLocation(programID, name.c_str());
    if (loc != -1)
        glUniform1f(loc, value);
}

unsigned int ShaderGL::compileShader(unsigned int type, const char* src)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    checkCompileErrors(shader, (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT");

    return shader;
}

void ShaderGL::checkCompileErrors(unsigned int obj, std::string type)
{
    int success;
    char infoLog[1024];

    if (type != "PROGRAM")
    {
        glGetShaderiv(obj, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(obj, 1024, NULL, infoLog);
            std::cout << "Shader error (" << type << "):\n" << infoLog << "\n";
        }
    }
    else
    {
        glGetProgramiv(obj, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(obj, 1024, NULL, infoLog);
            std::cout << "Program link error:\n" << infoLog << "\n";
        }
    }
}