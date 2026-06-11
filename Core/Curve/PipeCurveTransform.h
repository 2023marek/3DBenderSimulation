#pragma once

#include <vector>

#include "Core/Geometry/Frame.h"
#include "Core/Geometry/PipeNode.h"
#include "Core/Math/Vec3D.h"

// =====================================================
// PIPE CURVE TRANSFORM
//
// Utility for transforming sampled PipeNode lists from
// local coordinates into a target frame.
//
// Current phase:
// - sampled-node transform only
//
// Later:
// - transform PipeCurveSegment analytically
// - preserve segment-level curvature model
// =====================================================

class PipeCurveTransform
{
public:
    static std::vector<PipeNode> transformNodesToFrame(
        const std::vector<PipeNode>& localNodes,
        const Frame& targetFrame)
    {
        std::vector<PipeNode> result;

        result.reserve(
            localNodes.size()
        );

        for (const auto& node : localNodes)
        {
            PipeNode transformed;

            transformed.pos =
                transformPoint(
                    node.pos,
                    targetFrame
                );

            transformed.T =
                transformDirection(
                    node.T,
                    targetFrame
                );

            transformed.N =
                transformDirection(
                    node.N,
                    targetFrame
                );

            transformed.B =
                transformDirection(
                    node.B,
                    targetFrame
                );

            result.push_back(
                transformed
            );
        }

        return result;
    }

private:
    static Vec3D transformPoint(
        const Vec3D& localPoint,
        const Frame& frame)
    {
        Vec3D T =
            frame.T.normalized();

        Vec3D N =
            frame.N.normalized();

        Vec3D B =
            frame.B.normalized();

        return frame.P
            + T * localPoint.x
            + N * localPoint.y
            + B * localPoint.z;
    }

    static Vec3D transformDirection(
        const Vec3D& localDirection,
        const Frame& frame)
    {
        Vec3D T =
            frame.T.normalized();

        Vec3D N =
            frame.N.normalized();

        Vec3D B =
            frame.B.normalized();

        Vec3D result =
            T * localDirection.x
            + N * localDirection.y
            + B * localDirection.z;

        if (result.lengthSquared() < 1e-12)
            return { 1.0, 0.0, 0.0 };

        return result.normalized();
    }
};
