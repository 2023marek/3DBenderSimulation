#pragma once

#include <vector>
#include "../Core/Math/Vec3D.h"

class TubeMesh
{
public:
    TubeMesh(double radius = 5.0, int radialSegments = 8);

    void build(const std::vector<Vec3D>& points,
        const std::vector<Vec3D>& tangents);

    const std::vector<float>& getVertices() const;
    const std::vector<unsigned int>& getIndices() const;

private:
    double radius;
    int radialSegments;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

private:
    void buildRing(const Vec3D& P, const Vec3D& T,
        std::vector<Vec3D>& ringPoints);
};
