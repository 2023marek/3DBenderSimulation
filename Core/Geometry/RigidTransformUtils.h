#pragma once

#include <cmath>

#include "Core/Math/Vec3D.h"
#include "Core/Geometry/PipeNode.h"

// =====================================================
// RIGID TRANSFORM UTILITIES
//
// Generic Core geometry transformations.
//
// No knowledge of:
//     machine
//     stretch helix
//     manufacturing process
//     OpenGL
//
// These helpers only transform geometric objects.
// =====================================================

namespace RigidTransformUtils
{

    inline Vec3D rotateAroundAxis(
        const Vec3D& v,
        const Vec3D& axis,
        double angle)
    {
        Vec3D k =
            axis.normalized();

        if (k.lengthSquared() < 1e-12)
            return v;

        const double c =
            std::cos(angle);

        const double s =
            std::sin(angle);

        return
            v * c
            + cross(k, v) * s
            + k * dot(k, v) * (1.0 - c);
    }


    inline Vec3D rotatePointAroundAxis(
        const Vec3D& point,
        const Vec3D& axisPoint,
        const Vec3D& axisDirection,
        double angle)
    {
        const Vec3D relative =
            point - axisPoint;

        const Vec3D rotated =
            rotateAroundAxis(
                relative,
                axisDirection,
                angle
            );

        return
            axisPoint
            + rotated;
    }


    inline void rotateNodeAroundAxis(
        PipeNode& node,
        const Vec3D& axisPoint,
        const Vec3D& axisDirection,
        double angle)
    {
        const Vec3D axis =
            axisDirection.normalized();

        if (axis.lengthSquared() < 1e-12)
            return;

        node.pos =
            rotatePointAroundAxis(
                node.pos,
                axisPoint,
                axis,
                angle
            );

        node.T =
            rotateAroundAxis(
                node.T,
                axis,
                angle
            ).normalized();

        node.N =
            rotateAroundAxis(
                node.N,
                axis,
                angle
            ).normalized();

        node.B =
            rotateAroundAxis(
                node.B,
                axis,
                angle
            ).normalized();
    }

}
