#pragma once

#include "Core/Math/Vec3D.h"

// =====================================================
// PIPE NODE
//
// Sample point on a pipe centerline.
//
// pos = centerline point
// T   = tangent
// N   = normal
// B   = binormal
//
// Used by:
// - pipe rendering
// - manufacturing zones
// - CAD preview
// - dimensions
// - collision later
// =====================================================

struct PipeNode
{
    Vec3D pos;

    Vec3D T;
    Vec3D N;
    Vec3D B;
};