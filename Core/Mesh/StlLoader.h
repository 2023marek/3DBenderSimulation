#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "Core/Mesh/TriangleMesh.h"
#include "Core/Math/Vec3D.h"

// =====================================================
// STL LOADER
//
// Loads ASCII and binary STL files into TriangleMesh.
//
// This loader does NOT render.
// This loader does NOT know about MachinePart.
// This loader only converts STL -> neutral TriangleMesh.
// =====================================================

class StlLoader
{
public:
    static bool load(
        const std::string& path,
        TriangleMesh& mesh)
    {
        mesh.clear();
        mesh.sourcePath = path;

        std::ifstream file(
            path,
            std::ios::binary | std::ios::ate
        );

        if (!file)
        {
            std::cerr << "[STL LOADER ERROR] Cannot open file: "
                << path
                << std::endl;
            return false;
        }

        std::streamsize fileSize =
            file.tellg();

        file.seekg(0, std::ios::beg);

        bool binary =
            looksLikeBinaryStl(file, fileSize);

        file.close();

        bool ok = false;

        if (binary)
        {
            ok = loadBinary(path, mesh);
        }
        else
        {
            ok = loadAscii(path, mesh);
        }

        if (ok)
        {
            mesh.computeBounds();

            std::cout << "[STL LOADER] Loaded "
                << path
                << " triangles="
                << mesh.triangleCount()
                << " format="
                << (binary ? "binary" : "ascii")
                << std::endl;
        }

        return ok;
    }

private:
    static bool looksLikeBinaryStl(
        std::ifstream& file,
        std::streamsize fileSize)
    {
        if (fileSize < 84)
            return false;

        file.seekg(80, std::ios::beg);

        std::uint32_t triangleCount = 0;

        file.read(
            reinterpret_cast<char*>(&triangleCount),
            sizeof(std::uint32_t)
        );

        std::streamsize expectedSize =
            84 + static_cast<std::streamsize>(triangleCount) * 50;

        return expectedSize == fileSize;
    }

    static Vec3D readVec3Float(
        std::ifstream& file)
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        file.read(reinterpret_cast<char*>(&x), sizeof(float));
        file.read(reinterpret_cast<char*>(&y), sizeof(float));
        file.read(reinterpret_cast<char*>(&z), sizeof(float));

        return {
            static_cast<double>(x),
            static_cast<double>(y),
            static_cast<double>(z)
        };
    }

    static Vec3D safeNormal(
        const Vec3D& declaredNormal,
        const Vec3D& v0,
        const Vec3D& v1,
        const Vec3D& v2)
    {
        if (declaredNormal.lengthSquared() > 1e-12)
            return declaredNormal.normalized();

        Vec3D computed =
            cross(v1 - v0, v2 - v0);

        if (computed.lengthSquared() < 1e-12)
            return { 0.0, 0.0, 1.0 };

        return computed.normalized();
    }

    static bool loadBinary(
        const std::string& path,
        TriangleMesh& mesh)
    {
        std::ifstream file(
            path,
            std::ios::binary
        );

        if (!file)
            return false;

        char header[80];
        file.read(header, 80);

        std::uint32_t triangleCount = 0;

        file.read(
            reinterpret_cast<char*>(&triangleCount),
            sizeof(std::uint32_t)
        );

        mesh.triangles.reserve(triangleCount);

        for (std::uint32_t i = 0; i < triangleCount; ++i)
        {
            Vec3D normal =
                readVec3Float(file);

            Vec3D v0 =
                readVec3Float(file);

            Vec3D v1 =
                readVec3Float(file);

            Vec3D v2 =
                readVec3Float(file);

            std::uint16_t attributeByteCount = 0;

            file.read(
                reinterpret_cast<char*>(&attributeByteCount),
                sizeof(std::uint16_t)
            );

            MeshTriangle tri;
            tri.v0 = v0;
            tri.v1 = v1;
            tri.v2 = v2;
            tri.normal = safeNormal(normal, v0, v1, v2);

            mesh.triangles.push_back(tri);
        }

        return true;
    }

    static bool loadAscii(
        const std::string& path,
        TriangleMesh& mesh)
    {
        std::ifstream file(path);

        if (!file)
            return false;

        std::string token;

        Vec3D currentNormal = { 0.0, 0.0, 1.0 };

        while (file >> token)
        {
            if (token == "facet")
            {
                std::string normalWord;
                file >> normalWord;

                if (normalWord == "normal")
                {
                    file >> currentNormal.x
                        >> currentNormal.y
                        >> currentNormal.z;
                }
            }
            else if (token == "vertex")
            {
                Vec3D v0;
                Vec3D v1;
                Vec3D v2;

                file >> v0.x >> v0.y >> v0.z;

                file >> token; // vertex
                file >> v1.x >> v1.y >> v1.z;

                file >> token; // vertex
                file >> v2.x >> v2.y >> v2.z;

                MeshTriangle tri;
                tri.v0 = v0;
                tri.v1 = v1;
                tri.v2 = v2;
                tri.normal =
                    safeNormal(currentNormal, v0, v1, v2);

                mesh.triangles.push_back(tri);
            }
        }

        if (mesh.triangles.empty())
        {
            std::cerr << "[STL LOADER ERROR] No triangles found in ASCII STL: "
                << path
                << std::endl;

            return false;
        }

        return true;
    }
};