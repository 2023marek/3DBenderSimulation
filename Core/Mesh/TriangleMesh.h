#pragma once

#include <vector>
#include <string>
#include <limits>

#include "Core/Math/Vec3D.h"

// =====================================================
// TRIANGLE MESH
//
// Neutral mesh data loaded from STL or other formats.
//
// This is NOT OpenGL data.
// This is NOT machine-specific.
// Renderer later converts this to GPU buffers.
// =====================================================

struct MeshTriangle
{
    Vec3D normal;

    Vec3D v0;
    Vec3D v1;
    Vec3D v2;
};

struct TriangleMesh
{
    std::string sourcePath;

    std::vector<MeshTriangle> triangles;

    Vec3D minBounds = { 0.0, 0.0, 0.0 };
    Vec3D maxBounds = { 0.0, 0.0, 0.0 };

    void clear()
    {
        sourcePath.clear();
        triangles.clear();

        minBounds = { 0.0, 0.0, 0.0 };
        maxBounds = { 0.0, 0.0, 0.0 };
    }

    bool empty() const
    {
        return triangles.empty();
    }

    size_t triangleCount() const
    {
        return triangles.size();
    }

    void computeBounds()
    {
        if (triangles.empty())
        {
            minBounds = { 0.0, 0.0, 0.0 };
            maxBounds = { 0.0, 0.0, 0.0 };
            return;
        }

        double minX = std::numeric_limits<double>::max();
        double minY = std::numeric_limits<double>::max();
        double minZ = std::numeric_limits<double>::max();

        double maxX = -std::numeric_limits<double>::max();
        double maxY = -std::numeric_limits<double>::max();
        double maxZ = -std::numeric_limits<double>::max();

        auto update =
            [&](const Vec3D& p)
            {
                minX = std::min(minX, p.x);
                minY = std::min(minY, p.y);
                minZ = std::min(minZ, p.z);

                maxX = std::max(maxX, p.x);
                maxY = std::max(maxY, p.y);
                maxZ = std::max(maxZ, p.z);
            };

        for (const auto& tri : triangles)
        {
            update(tri.v0);
            update(tri.v1);
            update(tri.v2);
        }

        minBounds = { minX, minY, minZ };
        maxBounds = { maxX, maxY, maxZ };
    }
};
