#pragma once

#include <filesystem>
#include <iostream>
#include <string>

// =====================================================
// ASSET PATH RESOLVER
//
// Converts project-relative asset paths into real
// filesystem paths.
//
// Example:
//      Assets/Machine/bend_die.stl
//
// Works from build folders like:
//      out/build/debug
//
// by walking upward until the asset is found.
// =====================================================

class AssetPathResolver
{
public:
    static std::string resolve(
        const std::string& path)
    {
        namespace fs = std::filesystem;

        if (path.empty())
            return "";

        fs::path inputPath(path);

        // Absolute path already valid.
        if (inputPath.is_absolute())
        {
            if (fs::exists(inputPath))
                return inputPath.string();

            std::cerr << "[ASSET PATH ERROR] Absolute path does not exist: "
                << inputPath.string()
                << std::endl;

            return "";
        }

        // Try relative to current working directory.
        fs::path current =
            fs::current_path();

        fs::path direct =
            current / inputPath;

        if (fs::exists(direct))
            return fs::weakly_canonical(direct).string();

        // Walk upward from current directory.
        fs::path dir =
            current;

        while (!dir.empty())
        {
            fs::path candidate =
                dir / inputPath;

            if (fs::exists(candidate))
                return fs::weakly_canonical(candidate).string();

            fs::path parent =
                dir.parent_path();

            if (parent == dir)
                break;

            dir = parent;
        }

        std::cerr << "[ASSET PATH ERROR] Could not resolve asset: "
            << path
            << " from working directory: "
            << current.string()
            << std::endl;

        return "";
    }
};
