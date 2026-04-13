#pragma once
#include <string>
#include <map>
#include "ShaderGL.h"

class ShaderManager
{
public:
    static ShaderManager& instance();

    // Load shader from files
    ShaderGL* load(const std::string& name,
        const std::string& vertPath,
        const std::string& fragPath);

    // Get already-loaded shader
    ShaderGL* get(const std::string& name);

    // Reload all shaders (for development)
    void reloadAll();

private:
    ShaderManager() = default;

    // Helper: read file to string
    std::string readFile(const std::string& filePath);

    // Shader cache
    std::map<std::string, ShaderGL*> shaders;
};