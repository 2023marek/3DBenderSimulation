#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "Core/Curve/PipeCurve.h"
#include "Core/Geometry/PipeNode.h"
#include "Core/Geometry/Frame.h"
#include "Core/Math/Vec3D.h"

// =====================================================
// PIPE CURVE SAMPLER
//
// Converts curvature-driven PipeCurve into PipeNode samples.
//
// This is the shared discretization layer.
//
// Important rule:
//
//      Curve segments are the model.
//      Nodes are only sampled output.
//
// Later used by:
// - GeometricPipeModel
// - ManufacturingPipeSimulator
// - PhysicalPipeSimulator
// - CollisionSystem
// - DimensionSystem
// =====================================================

class PipeCurveSampler
{
public:
    static std::vector<PipeNode> sample(
        const PipeCurve& curve,
        double ds = 0.5)
    {
        std::vector<PipeNode> nodes;

        Frame frame =
            defaultFrame();

        appendNode(nodes, frame);

        sampleInto(
            curve,
            frame,
            nodes,
            ds
        );

        return nodes;
    }

    static void sampleInto(
        const PipeCurve& curve,
        Frame& frame,
        std::vector<PipeNode>& nodes,
        double ds = 0.5)
    {
        if (ds <= 1e-9)
            ds = 0.5;

        for (const auto& segment : curve.segments)
        {
            sampleSegment(
                segment,
                frame,
                nodes,
                ds
            );
        }
    }

private:
    static Frame defaultFrame()
    {
        Frame f;

        f.P = { 0.0, 0.0, 0.0 };
        f.T = { 1.0, 0.0, 0.0 };
        f.N = { 0.0, 1.0, 0.0 };
        f.B = { 0.0, 0.0, 1.0 };

        return f;
    }

    static void appendNode(
        std::vector<PipeNode>& nodes,
        const Frame& frame)
    {
        PipeNode node;

        node.pos = frame.P;
        node.T = frame.T;
        node.N = frame.N;
        node.B = frame.B;

        nodes.push_back(node);
    }

    static void sampleSegment(
        const PipeCurveSegment& segment,
        Frame& frame,
        std::vector<PipeNode>& nodes,
        double ds)
    {
        switch (segment.type)
        {
        case PipeCurveSegmentType::Line:
            sampleLine(segment, frame, nodes, ds);
            break;

        case PipeCurveSegmentType::CircularArc:
            sampleCircularArc(segment, frame, nodes, ds);
            break;

        case PipeCurveSegmentType::RotationOnly:
            sampleRotationOnly(segment, frame, nodes);
            break;

        case PipeCurveSegmentType::Helix:
            sampleConstantCurvatureTorsion(segment, frame, nodes, ds);
            break;

        case PipeCurveSegmentType::VariableCurvature:
            sampleVariableCurvature(segment, frame, nodes, ds);
            break;

        default:
            break;
        }
    }

    static void sampleLine(
        const PipeCurveSegment& segment,
        Frame& frame,
        std::vector<PipeNode>& nodes,
        double ds)
    {
        if (segment.length <= 0.0)
            return;

        int steps =
            std::max(
                1,
                static_cast<int>(
                    std::ceil(segment.length / ds)
                    )
            );

        double stepLength =
            segment.length / static_cast<double>(steps);

        for (int i = 0; i < steps; ++i)
        {
            frame.P =
                frame.P
                + frame.T.normalized() * stepLength;

            appendNode(nodes, frame);
        }
    }

    static void sampleCircularArc(
        const PipeCurveSegment& segment,
        Frame& frame,
        std::vector<PipeNode>& nodes,
        double ds)
    {
        if (segment.radius <= 1e-9)
            return;

        if (segment.angle <= 0.0)
            return;

        double arcLength =
            segment.radius * segment.angle;

        int steps =
            std::max(
                1,
                static_cast<int>(
                    std::ceil(arcLength / ds)
                    )
            );

        double dA =
            segment.angle / static_cast<double>(steps);

        double sign =
            bendDirectionSign(segment.bendDirection);

        for (int i = 0; i < steps; ++i)
        {
            Vec3D oldT =
                frame.T;

            double signedDA =
                dA * sign;

            frame.T =
                rotateAroundAxis(
                    frame.T,
                    frame.B,
                    signedDA
                ).normalized();

            transportFrame(
                oldT,
                frame.T,
                frame
            );

            Vec3D midT =
                (oldT + frame.T).normalized();

            if (midT.lengthSquared() < 1e-12)
                midT = frame.T;

            frame.P =
                frame.P
                + midT * (segment.radius * dA);

            appendNode(nodes, frame);
        }
    }

    static void sampleRotationOnly(
        const PipeCurveSegment& segment,
        Frame& frame,
        std::vector<PipeNode>& nodes)
    {
        double signedAngle =
            segment.rotationAngle
            * rotationDirectionSign(segment.rotationDirection);

        if (std::abs(signedAngle) < 1e-12)
            return;

        frame.N =
            rotateAroundAxis(
                frame.N,
                frame.T,
                signedAngle
            ).normalized();

        frame.B =
            rotateAroundAxis(
                frame.B,
                frame.T,
                signedAngle
            ).normalized();

        orthonormalizeFrame(frame);

        appendNode(nodes, frame);
    }

    static void sampleConstantCurvatureTorsion(
        const PipeCurveSegment& segment,
        Frame& frame,
        std::vector<PipeNode>& nodes,
        double ds)
    {
        if (segment.length <= 0.0)
            return;

        int steps =
            std::max(
                1,
                static_cast<int>(
                    std::ceil(segment.length / ds)
                    )
            );

        double h =
            segment.length / static_cast<double>(steps);

        for (int i = 0; i < steps; ++i)
        {
            integrateCurvatureTorsionStep(
                frame,
                segment.curvature,
                segment.torsion,
                h
            );

            appendNode(nodes, frame);
        }
    }

    static void sampleVariableCurvature(
        const PipeCurveSegment& segment,
        Frame& frame,
        std::vector<PipeNode>& nodes,
        double ds)
    {
        if (segment.curvatureSamples.empty())
            return;

        if (segment.length <= 0.0)
            return;

        int steps =
            std::max(
                1,
                static_cast<int>(
                    std::ceil(segment.length / ds)
                    )
            );

        double h =
            segment.length / static_cast<double>(steps);

        for (int i = 0; i < steps; ++i)
        {
            double s =
                h * static_cast<double>(i);

            double kappa =
                interpolateKappa(segment, s);

            double tau =
                interpolateTau(segment, s);

            integrateCurvatureTorsionStep(
                frame,
                kappa,
                tau,
                h
            );

            appendNode(nodes, frame);
        }
    }

private:
    static void integrateCurvatureTorsionStep(
        Frame& frame,
        double kappa,
        double tau,
        double ds)
    {
        // =====================================================
        // Simple curvature/torsion integration.
        //
        // kappa bends tangent around B.
        // tau twists frame around T.
        //
        // This is intentionally simple for Phase 7B.
        // Later we can improve with RK4 or exact helix formulas.
        // =====================================================

        Vec3D oldT =
            frame.T;

        double dBend =
            kappa * ds;

        if (std::abs(dBend) > 1e-12)
        {
            frame.T =
                rotateAroundAxis(
                    frame.T,
                    frame.B,
                    dBend
                ).normalized();

            transportFrame(
                oldT,
                frame.T,
                frame
            );
        }

        double dTwist =
            tau * ds;

        if (std::abs(dTwist) > 1e-12)
        {
            frame.N =
                rotateAroundAxis(
                    frame.N,
                    frame.T,
                    dTwist
                ).normalized();

            frame.B =
                rotateAroundAxis(
                    frame.B,
                    frame.T,
                    dTwist
                ).normalized();

            orthonormalizeFrame(frame);
        }

        Vec3D midT =
            (oldT + frame.T).normalized();

        if (midT.lengthSquared() < 1e-12)
            midT = frame.T;

        frame.P =
            frame.P
            + midT * ds;

        orthonormalizeFrame(frame);
    }

    static double interpolateKappa(
        const PipeCurveSegment& segment,
        double s)
    {
        return interpolateValue(
            segment.curvatureSamples,
            s,
            true
        );
    }

    static double interpolateTau(
        const PipeCurveSegment& segment,
        double s)
    {
        return interpolateValue(
            segment.curvatureSamples,
            s,
            false
        );
    }

    static double interpolateValue(
        const std::vector<CurvatureSample>& samples,
        double s,
        bool useKappa)
    {
        if (samples.empty())
            return 0.0;

        if (s <= samples.front().s)
            return useKappa
            ? samples.front().kappa
            : samples.front().tau;

        if (s >= samples.back().s)
            return useKappa
            ? samples.back().kappa
            : samples.back().tau;

        for (size_t i = 1; i < samples.size(); ++i)
        {
            if (s <= samples[i].s)
            {
                const CurvatureSample& a =
                    samples[i - 1];

                const CurvatureSample& b =
                    samples[i];

                double range =
                    b.s - a.s;

                if (std::abs(range) < 1e-12)
                    return useKappa ? b.kappa : b.tau;

                double t =
                    (s - a.s) / range;

                double va =
                    useKappa ? a.kappa : a.tau;

                double vb =
                    useKappa ? b.kappa : b.tau;

                return va + (vb - va) * t;
            }
        }

        return 0.0;
    }

private:
    static Vec3D rotateAroundAxis(
        const Vec3D& v,
        const Vec3D& axis,
        double angle)
    {
        Vec3D k =
            axis.normalized();

        if (k.lengthSquared() < 1e-12)
            return v;

        return v * std::cos(angle)
            + cross(k, v) * std::sin(angle)
            + k * dot(k, v) * (1.0 - std::cos(angle));
    }

    static void transportFrame(
        const Vec3D& oldT,
        const Vec3D& newT,
        Frame& frame)
    {
        Vec3D axis =
            cross(oldT, newT);

        if (axis.lengthSquared() < 1e-12)
        {
            orthonormalizeFrame(frame);
            return;
        }

        axis =
            axis.normalized();

        double d =
            dot(
                oldT.normalized(),
                newT.normalized()
            );

        d =
            std::max(
                -1.0,
                std::min(1.0, d)
            );

        double angle =
            std::acos(d);

        frame.N =
            rotateAroundAxis(
                frame.N,
                axis,
                angle
            ).normalized();

        frame.B =
            rotateAroundAxis(
                frame.B,
                axis,
                angle
            ).normalized();

        orthonormalizeFrame(frame);
    }

    static void orthonormalizeFrame(
        Frame& frame)
    {
        frame.T =
            frame.T.normalized();

        frame.N =
            (
                frame.N
                - frame.T * dot(frame.N, frame.T)
                ).normalized();

        frame.B =
            cross(
                frame.T,
                frame.N
            ).normalized();

        frame.N =
            cross(
                frame.B,
                frame.T
            ).normalized();
    }
};
