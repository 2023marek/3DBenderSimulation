#include "../Render/ShaderManager.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Singleton instance
static ShaderManager* g_instance = nullptr;

ShaderManager& ShaderManager::instance()
{
    if (!g_instance)
    {
        g_instance = new ShaderManager();
    }
    return *g_instance;
}

std::string ShaderManager::readFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "ERROR: Cannot open shader file: " << filePath << "\n";
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    std::cout << "Loaded shader: " << filePath << "\n";
    return buffer.str();
}

ShaderGL* ShaderManager::load(const std::string& name,
    const std::string& vertPath,
    const std::string& fragPath)
{
    // Check if already loaded
    if (shaders.find(name) != shaders.end())
    {
        std::cout << "Shader '" << name << "' already loaded. Returning existing.\n";
        return shaders[name];
    }

    // Read shader files
    std::string vertSource = readFile(vertPath);
    std::string fragSource = readFile(fragPath);

    if (vertSource.empty() || fragSource.empty())
    {
        std::cerr << "ERROR: Failed to load shader files for '" << name << "'\n";
        return nullptr;
    }

    // Create shader program
    ShaderGL* shader = new ShaderGL(vertSource.c_str(), fragSource.c_str());

    // Store in map
    shaders[name] = shader;

    std::cout << "Shader '" << name << "' loaded successfully.\n";
    return shader;
}

ShaderGL* ShaderManager::get(const std::string& name)
{
    auto it = shaders.find(name);
    if (it != shaders.end())
    {
        return it->second;
    }

    std::cerr << "WARNING: Shader '" << name << "' not found.\n";
    return nullptr;
}

void ShaderManager::reloadAll()
{
    std::cout << "Reloading all shaders...\n";

    // For now, just notify
    // Full reload would require storing file paths
    std::cout << "To implement full reload, store shader file paths in ShaderManager\n";
}