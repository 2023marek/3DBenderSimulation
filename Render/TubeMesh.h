#pragma once
#include <vector>
#include "Core/Math/Vec3D.h"

class TubeMesh
{
public:
    // ? Use float for GPU vertex data, not double!
    struct Vertex
    {
        float position[3];
        float normal[3];
    };

    TubeMesh();

    // Generate tube along centerline
    void generate(const std::vector<Vec3D>& centerline,
        const std::vector<Vec3D>& tangents,
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
    const std::vector<unsigned int>& getIndices() const { return indices; }

    void clear(); // reset mesh

private:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};