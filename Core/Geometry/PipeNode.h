#pragma once

#include "Core/Math/Vec3D.h"

// =====================================================
// PIPE NODE
//
// Sample point on a pipe centerline.
//
// This is not only a point.
// It also stores the local frame orientation at that point.
//
// Used by:
// - pipe rendering
// - manufacturing zones
// - CAD preview
// - dimensions
// - collision later
// =====================================================

// =====================================================
// PIPE NODE
//
// Sample point on a pipe centerline.
//
// pos = centerline point
// T   = tangent
// N   = normal
// B   = binormal
// =====================================================

struct PipeNode
{
    Vec3D pos;

    Vec3D T;
    Vec3D N;
    Vec3D B;

    glm::vec3 getPosition() const
    {
        return glm::vec3(
            static_cast<float>(pos.x),
            static_cast<float>(pos.y),
            static_cast<float>(pos.z)
        );
    }
};