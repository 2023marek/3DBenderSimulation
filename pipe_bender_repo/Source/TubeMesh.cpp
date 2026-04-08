#include "../Render/TubeMesh.h"
#include <cmath>
static constexpr double PI = 3.14159265358979323846;

void TubeMesh::build(const std::vector<Vec3D>& points,
    const std::vector<Vec3D>& tangents)
{
    vertices.clear();
    indices.clear();

    if (points.size() < 2) return;

    std::vector<std::vector<Vec3D>> rings;

    for (size_t i = 0; i < points.size(); i++)
    {
        std::vector<Vec3D> ring;

        buildRing(points[i], tangents[i], ring);

        rings.push_back(ring);
    }

    // Build vertices
    for (const auto& ring : rings)
    {
        for (const auto& p : ring)
        {
            vertices.push_back((float)p.x);
            vertices.push_back((float)p.y);
            vertices.push_back((float)p.z);
        }
    }

    // Build indices (triangle strip)
    int ringSize = radialSegments;

    for (size_t i = 0; i < rings.size() - 1; i++)
    {
        for (int j = 0; j < ringSize; j++)
        {
            int current = i * ringSize + j;
            int next = (i + 1) * ringSize + j;

            int nextJ = (j + 1) % ringSize;

            int currentNext = i * ringSize + nextJ;
            int nextNext = (i + 1) * ringSize + nextJ;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(currentNext);

            indices.push_back(currentNext);
            indices.push_back(next);
            indices.push_back(nextNext);
        }
    }
}

void TubeMesh::buildRing(const Vec3D& P,
    const Vec3D& T,
    std::vector<Vec3D>& ring)
{
    Vec3D t = normalize(T);

    Vec3D up = (std::abs(t.z) < 0.9) ? Vec3D{ 0,0,1 } : Vec3D{ 0,1,0 };

    Vec3D N = normalize(cross(t, up));
    Vec3D B = normalize(cross(t, N));

    for (int i = 0; i < radialSegments; i++)
    {
        double theta = 2.0 * PI * i / radialSegments;

        Vec3D offset =
            N * (std::cos(theta) * radius) +
            B * (std::sin(theta) * radius);

        ring.push_back(P + offset);
    }
}
const std::vector<float>& TubeMesh::getVertices() const
{
    return vertices;
}

const std::vector<unsigned int>& TubeMesh::getIndices() const
{
    return indices;
}
TubeMesh::TubeMesh(double radius, int radialSegments)
    : radius(radius), radialSegments(radialSegments)
{
}
