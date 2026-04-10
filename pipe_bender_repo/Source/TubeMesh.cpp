#include "../Render/TubeMesh.h"
#include <cmath>

// =========================
// LOCAL UTILS
// =========================
static constexpr double PI = 3.14159265358979323846;

static double clamp(double v, double a, double b)
{
    return std::max(a, std::min(b, v));
}

// =========================
// CONSTRUCTOR
// =========================
TubeMesh::TubeMesh(double r, int segments)
    : radius(r), radialSegments(segments)
{
}

// =========================
// SETTERS
// =========================
void TubeMesh::setRadius(double r)
{
    radius = r;
}

void TubeMesh::setRadialSegments(int n)
{
    radialSegments = n;
}

// =========================
// GETTERS
// =========================
const std::vector<float>& TubeMesh::getVertices() const { return vertices; }
const std::vector<float>& TubeMesh::getNormals() const { return normals; }
const std::vector<unsigned int>& TubeMesh::getIndices() const { return indices; }

// =========================
// BUILD
// =========================
void TubeMesh::build(const std::vector<Vec3D>& points,
    const std::vector<Vec3D>& tangents)
{
    vertices.clear();
    normals.clear();
    indices.clear();

    if (points.size() < 2 || tangents.size() < 2)
        return;

    // =========================
    // FRAME ARRAYS
    // =========================
    std::vector<Vec3D> frameN;
    std::vector<Vec3D> frameB;

    // --- INITIAL FRAME ---
    Vec3D t0 = normalize(tangents[0]);

    Vec3D up = (std::abs(t0.z) < 0.9) ? Vec3D{ 0,0,1 } : Vec3D{ 0,1,0 };

    Vec3D N0 = normalize(cross(t0, up));
    Vec3D B0 = normalize(cross(t0, N0));

    frameN.push_back(N0);
    frameB.push_back(B0);

    // =========================
    // PARALLEL TRANSPORT
    // =========================
    for (size_t i = 1; i < points.size(); i++)
    {
        Vec3D tPrev = normalize(tangents[i - 1]);
        Vec3D tCurr = normalize(tangents[i]);

        Vec3D axis = cross(tPrev, tCurr);
        double len = length(axis);

        if (len < 1e-6)
        {
            frameN.push_back(frameN.back());
            frameB.push_back(frameB.back());
            continue;
        }

        axis = normalize(axis);

        double cosA = clamp(dot(tPrev, tCurr), -1.0, 1.0);
        double angle = std::acos(cosA);

        Vec3D Nprev = frameN.back();
        Vec3D Bprev = frameB.back();

        // Rodrigues rotation
        auto rotate = [&](const Vec3D& v)
            {
                return v * std::cos(angle)
                    + cross(axis, v) * std::sin(angle)
                    + axis * dot(axis, v) * (1 - std::cos(angle));
            };

        Vec3D Nnew = normalize(rotate(Nprev));
        Vec3D Bnew = normalize(rotate(Bprev));

        frameN.push_back(Nnew);
        frameB.push_back(Bnew);
    }

    // =========================
    // BUILD RINGS
    // =========================
    std::vector<std::vector<Vec3D>> rings;

    for (size_t i = 0; i < points.size(); i++)
    {
        std::vector<Vec3D> ring;
        buildRing(points[i], frameN[i], frameB[i], ring);
        rings.push_back(ring);
    }

    // =========================
    // BUILD VERTICES + NORMALS
    // =========================
    for (size_t i = 0; i < rings.size(); i++)
    {
        const auto& ring = rings[i];
        Vec3D center = points[i];

        for (const auto& p : ring)
        {
            Vec3D normal = normalize(p - center);

            vertices.push_back((float)p.x);
            vertices.push_back((float)p.y);
            vertices.push_back((float)p.z);

            normals.push_back((float)normal.x);
            normals.push_back((float)normal.y);
            normals.push_back((float)normal.z);
        }
    }

    // =========================
    // BUILD INDICES
    // =========================
    int ringSize = radialSegments;

    for (size_t i = 0; i < rings.size() - 1; i++)
    {
        for (int j = 0; j < ringSize; j++)
        {
            int nextJ = (j + 1) % ringSize;

            int a = i * ringSize + j;
            int b = (i + 1) * ringSize + j;
            int c = i * ringSize + nextJ;
            int d = (i + 1) * ringSize + nextJ;

            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(c);

            indices.push_back(c);
            indices.push_back(b);
            indices.push_back(d);
        }
    }
}

// =========================
// BUILD SINGLE RING
// =========================
void TubeMesh::buildRing(const Vec3D& center,
    const Vec3D& N,
    const Vec3D& B,
    std::vector<Vec3D>& ring)
{
    for (int i = 0; i < radialSegments; i++)
    {
        double theta = 2.0 * PI * i / radialSegments;

        Vec3D offset =
            N * (std::cos(theta) * radius) +
            B * (std::sin(theta) * radius);

        ring.push_back(center + offset);
    }
}