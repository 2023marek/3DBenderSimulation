#pragma once

#include <vector>
#include "../Core/Math/Vec3D.h"

class TubeMesh
{
public:
    // =========================
    // CONSTRUCTOR
    // =========================
    TubeMesh(double radius = 1.0, int radialSegments = 8);

    // =========================
    // BUILD
    // =========================
    void build(const std::vector<Vec3D>& points,
        const std::vector<Vec3D>& tangents);

    // =========================
    // GETTERS
    // =========================
    const std::vector<float>& getVertices() const;
    const std::vector<float>& getNormals() const;
    const std::vector<unsigned int>& getIndices() const;

    // =========================
    // CONFIG
    // =========================
    void setRadius(double r);
    void setRadialSegments(int n);

private:
    // =========================
    // PARAMETERS
    // =========================
    double radius;
    int radialSegments;

    // =========================
    // GPU DATA
    // =========================
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<unsigned int> indices;

private:
    // =========================
    // INTERNAL
    // =========================
    void buildRing(const Vec3D& center,
        const Vec3D& N,
        const Vec3D& B,
        std::vector<Vec3D>& ring);
};