#pragma once
#include <vector>
#include "../Core/Math/Vec3D.h"

class TubeMesh
{
public:
    struct Vertex
    {
        Vec3D position;
        Vec3D normal;
    };

    TubeMesh();

    // Generate tube along centerline
    void generate(const std::vector<Vec3D>& centerline,
        double radius,
        int segments);

    // Frame construction (stable orientation)
    void computeInitialFrame(const Vec3D& tangent,
        Vec3D& normal,
        Vec3D& binormal);

    void updateFramePTF(const Vec3D& prevT,
        const Vec3D& currT,
        Vec3D& normal,
        Vec3D& binormal);

    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<int>& getIndices() const { return indices; }

    void clear(); // reset mesh

private:
    std::vector<Vertex> vertices;
    std::vector<int> indices;
};