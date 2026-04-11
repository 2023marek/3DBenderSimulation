#include "../Render/TubeMesh.h"
#include <cmath>
#include <algorithm> // std::clamp

// =========================
// SAFE M_PI (MSVC FIX)
// =========================
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

TubeMesh::TubeMesh() {}

void TubeMesh::clear()
{
    vertices.clear();
    indices.clear();
}
void TubeMesh::computeInitialFrame(const Vec3D& tangent,
    Vec3D& normal,
    Vec3D& binormal)
{
    // choose stable up vector
    Vec3D up = (std::fabs(tangent.y) < 0.9)
        ? Vec3D{ 0, 1, 0 }
    : Vec3D{ 1, 0, 0 };

    normal = normalize(cross(tangent, up));
    binormal = cross(tangent, normal);
}
void TubeMesh::updateFramePTF(const Vec3D& prevT,
    const Vec3D& currT,
    Vec3D& normal,
    Vec3D& binormal)
{
    Vec3D axis = cross(prevT, currT);
    double axisLen = length(axis);

    // if almost no rotation ? skip
    if (axisLen < 1e-9)
        return;

    axis = normalize(axis);

    double dotVal = std::clamp(dot(prevT, currT), -1.0, 1.0);
    double angle = std::acos(dotVal);

    auto rotate = [&](const Vec3D& v)
        {
            // Rodrigues rotation formula
            Vec3D term1 = v * std::cos(angle);
            Vec3D term2 = cross(axis, v) * std::sin(angle);
            Vec3D term3 = axis * (dot(axis, v) * (1.0 - std::cos(angle)));
            return term1 + term2 + term3;
        };

    normal = normalize(rotate(normal));
    binormal = normalize(rotate(binormal));
}
void TubeMesh::generate(const std::vector<Vec3D>& C,
    double radius,
    int segments)
{
    clear();

    if (C.size() < 2) return;

    Vec3D Tprev = normalize(C[1] - C[0]);

    Vec3D N, B;
    computeInitialFrame(Tprev, N, B);

    for (size_t i = 0; i < C.size(); i++)
    {
        Vec3D T;

        if (i < C.size() - 1)
            T = normalize(C[i + 1] - C[i]);
        else
            T = Tprev;

        if (i > 0)
            updateFramePTF(Tprev, T, N, B);

        for (int s = 0; s < segments; s++)
        {
            double theta = 2.0 * M_PI * s / segments;

            Vec3D offset =
                N * std::cos(theta) * radius +
                B * std::sin(theta) * radius;

            Vertex v;
            v.position = C[i] + offset;
            v.normal = normalize(offset);

            vertices.push_back(v);
        }

        Tprev = T;
    }
}