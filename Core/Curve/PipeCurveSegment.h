#pragma once

#include <functional>
#include <vector>

#include "Core/Math/Vec3D.h"
#include "Core/Geometry/Frame.h"
#include "Core/BendDirection.h"
#include "Core/Operations.h"

// =====================================================
// PIPE CURVE SEGMENT
//
// Shared curvature-driven geometric representation.
//
// This is NOT rendering.
// This is NOT OpenGL.
// This is NOT manufacturing zone state.
//
// It describes pipe centerline mathematically.
// Later both CAD and manufacturing should produce these.
// =====================================================

enum class PipeCurveSegmentType
{
    Line,
    CircularArc,
    RotationOnly,
    Helix,
    VariableCurvature
};

// =====================================================
// VARIABLE CURVATURE SAMPLE
//
// Describes curvature as a function of arc length:
//
//      kappa = curvature
//      tau   = torsion
//
// For now this is simple and explicit.
// Later we can replace / extend it with analytic functions.
// =====================================================

struct CurvatureSample
{
    double s = 0.0;       // arc length position
    double kappa = 0.0;   // curvature = 1 / radius
    double tau = 0.0;     // torsion
};

// =====================================================
// PIPE CURVE SEGMENT
// =====================================================

struct PipeCurveSegment
{
    PipeCurveSegmentType type =
        PipeCurveSegmentType::Line;

    // Common length.
    double length = 0.0;

    // Circular arc.
    double radius = 0.0;
    double angle = 0.0;
    BendDirection bendDirection = BendDirection::CCW;

    // Rotation-only segment.
    double rotationAngle = 0.0;
    RotationDirection rotationDirection = RotationDirection::CCW;

    // Helix / torsion-capable segment.
    double curvature = 0.0; // kappa
    double torsion = 0.0;   // tau

    // Variable curvature representation.
    std::vector<CurvatureSample> curvatureSamples;

    // Optional start frame.
    // Usually sampler receives a start frame externally,
    // but this is useful for future cached passes.
    Frame startFrame;

    // =====================================================
    // FACTORY HELPERS
    // =====================================================

    static PipeCurveSegment makeLine(double length)
    {
        PipeCurveSegment s;
        s.type = PipeCurveSegmentType::Line;
        s.length = length;
        return s;
    }

    static PipeCurveSegment makeCircularArc(
        double radius,
        double angle,
        BendDirection direction)
    {
        PipeCurveSegment s;
        s.type = PipeCurveSegmentType::CircularArc;
        s.radius = radius;
        s.angle = angle;
        s.length = radius > 0.0
            ? radius * angle
            : 0.0;
        s.curvature = radius > 0.0
            ? 1.0 / radius
            : 0.0;
        s.bendDirection = direction;
        return s;
    }

    static PipeCurveSegment makeRotationOnly(
        double angle,
        RotationDirection direction)
    {
        PipeCurveSegment s;
        s.type = PipeCurveSegmentType::RotationOnly;
        s.rotationAngle = angle;
        s.rotationDirection = direction;
        return s;
    }

    static PipeCurveSegment makeHelix(
        double length,
        double curvature,
        double torsion)
    {
        PipeCurveSegment s;
        s.type = PipeCurveSegmentType::Helix;
        s.length = length;
        s.curvature = curvature;
        s.torsion = torsion;
        return s;
    }

    static PipeCurveSegment makeVariableCurvature(
        const std::vector<CurvatureSample>& samples)
    {
        PipeCurveSegment s;
        s.type = PipeCurveSegmentType::VariableCurvature;
        s.curvatureSamples = samples;

        if (!samples.empty())
        {
            s.length =
                samples.back().s;
        }

        return s;
    }
};