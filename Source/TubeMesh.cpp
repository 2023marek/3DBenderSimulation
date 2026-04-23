#include "../Render/TubeMesh.h"
#include <cmath>
#include <algorithm>  // std::clamp (C++17)
#include <utility>    // min/max backup

// =========================
// SAFE M_PI (MSVC FIX)
// =========================
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =========================
// CLAMP FUNCTION (C++17 Fallback)
// =========================
// If std::clamp is not available, provide our own
// This ensures compatibility with older g++ versions
namespace clamp_helper {
    template<typename T>
    inline T clamp(T value, T min_val, T max_val) {
        return (value < min_val) ? min_val :
            (value > max_val) ? max_val : value;
    }
}

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

    // =====================================================================
    // FIX: Use clamp_helper::clamp instead of std::clamp
    // This ensures C++11/14 compatibility while supporting C++17
    // =====================================================================
    double dotVal = clamp_helper::clamp(dot(prevT, currT), -1.0, 1.0);
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
    const std::vector<Vec3D>& T,
    double radius,
    int segments)
{
    clear();

    if (C.size() < 2) return;

    Vec3D Tprev = (T.size() > 0) ? T[0] : normalize(C[1] - C[0]);

    Vec3D N, B;
    computeInitialFrame(Tprev, N, B);

    for (size_t i = 0; i < C.size(); i++)
    {
        Vec3D currT;

        if (T.size() > i && i < C.size() - 1)
            currT = T[i];
        else if (i < C.size() - 1)
            currT = normalize(C[i + 1] - C[i]);
        else
            currT = Tprev;

        if (i > 0)
            updateFramePTF(Tprev, currT, N, B);

        for (int s = 0; s < segments; s++)
        {
            double theta = 2.0 * M_PI * s / segments;

            Vec3D offset =
                N * std::cos(theta) * radius +
                B * std::sin(theta) * radius;

            Vec3D pos = C[i] + offset;

            Vertex v;
            v.position[0] = (float)pos.x;
            v.position[1] = (float)pos.y;
            v.position[2] = (float)pos.z;

            Vec3D norm = normalize(offset);
            v.normal[0] = (float)norm.x;
            v.normal[1] = (float)norm.y;
            v.normal[2] = (float)norm.z;

            vertices.push_back(v);
        }

        Tprev = currT;
    }

    // =========================
    // GENERATE TRIANGLE INDICES
    // =========================
    for (size_t i = 0; i < C.size() - 1; i++)
    {
        for (int s = 0; s < segments; s++)
        {
            int nextS = (s + 1) % segments;

            unsigned int i0 = static_cast<unsigned int>(i * segments + s);
            unsigned int i1 = static_cast<unsigned int>(i * segments + nextS);
            unsigned int i2 = static_cast<unsigned int>((i + 1) * segments + s);
            unsigned int i3 = static_cast<unsigned int>((i + 1) * segments + nextS);

            // triangle 1
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            // triangle 2
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }
}