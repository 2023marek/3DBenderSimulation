#pragma once

#pragma once

#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>

#include "Core/Operations.h"
#include "Core/Math/Vec3D.h"
#include "Core/Geometry/Frame.h"
#include "Core/Geometry/PipeNode.h"
#include "Core/Geometry/PipeSegment.h"

#include "Core/Curve/PipeCurve.h"
#include "Core/Curve/PipeCurveSegment.h"
#include "Core/Sampling/PipeCurveSampler.h"
// =====================================================
// GEOMETRIC PIPE MODEL
//
// OWNER:
// Ideal CAD / geometric pipe representation.
//
// Responsibilities:
// - store operation history
// - rebuild ideal centerline from scratch
// - support CAD preview
// - later: YBC <-> XYZ conversion
// - later: editing / import / export
//
// This class does NOT simulate manufacturing.
// No incoming stock.
// No active zone.
// No frozen geometry.
// No physical springback.
// =====================================================

class GeometricPipeModel
{
public:
    using Node = PipeNode;
    using Segment = PipeSegment;

    GeometricPipeModel()
        : ds(0.5)
    {
        resetFrame();
    }

    explicit GeometricPipeModel(double sampleStep)
        : ds(sampleStep)
    {
        resetFrame();
    }

    void clear()
    {
        operations.clear();
        segments.clear();
        curve.clear();
        nodes.clear();

        resetFrame();

        dirty = true;
    }

    void addFeed(double length)
    {
        Operation op;
        op.type = Operation::FEED;
        op.length = length;

        operations.push_back(op);
        dirty = true;
    }

    void addBend(
        double radius,
        double angle,
        BendDirection bendDirection = BendDirection::CCW)
    {
        Operation op;
        op.type = Operation::BEND;
        op.R = radius;
        op.angle = angle;
        op.bendDirection = bendDirection;

        operations.push_back(op);
        dirty = true;
    }

    void addRotate(
        double angle,
        RotationDirection rotationDirection = RotationDirection::CCW)
    {
        Operation op;
        op.type = Operation::ROTATE;
        op.angle = angle;
        op.rotationDirection = rotationDirection;

        operations.push_back(op);
        dirty = true;
    }

    void setOperations(const std::vector<Operation>& ops)
    {
        operations = ops;
        dirty = true;
    }

    const std::vector<Operation>& getOperations() const
    {
        return operations;
    }

    const std::vector<Node>& getNodes() const
    {
        buildIfDirty();
        return nodes;
    }

    const std::vector<Segment>& getSegments() const
    {
        buildIfDirty();
        return segments;
    }

    const PipeCurve& getCurve() const
    {
        buildIfDirty();
        return curve;
    }

    void build() const
    {
        // =====================================================
        // CAD CURVATURE-DRIVEN BUILD PATH
        //
        // operations
        //      ?
        // PipeCurve segments
        //      ?
        // PipeCurveSampler
        //      ?
        // PipeNode samples for rendering
        //
        // Important:
        // Nodes are not the primary model.
        // Nodes are only sampled output.
        // =====================================================

        curve.clear();
        segments.clear();
        nodes.clear();

        resetFrame();

        buildCurveFromOperations();

        nodes =
            PipeCurveSampler::sample(
                curve,
                ds
            );

        std::cout << "[CAD CURVE BUILD] curveSegments="
            << curve.size()
            << " nodes="
            << nodes.size()
            << std::endl;

        // Temporary legacy/debug segment list.
        // Later this can be removed when all CAD code reads PipeCurve.
        buildSegments();

        dirty = false;
    }

private:
    double ds = 0.5;

    std::vector<Operation> operations;
    // Curvature-driven model.
    // This is now the primary CAD geometric representation.
    mutable PipeCurve curve;
    // Cached rebuild data.
    // These may change even inside const getters.
    mutable bool dirty = true;
    mutable std::vector<Segment> segments;
    mutable std::vector<Node> nodes;
    mutable Frame currentFrame;





private:
    void buildIfDirty() const
    {
        if (dirty)
            build();
    }

    void resetFrame() const
    {
        currentFrame.P = { 0.0, 0.0, 0.0 };
        currentFrame.T = { 1.0, 0.0, 0.0 };
        currentFrame.N = { 0.0, 1.0, 0.0 };
        currentFrame.B = { 0.0, 0.0, 1.0 };

    }

    void buildSegments() const
    {
        for (const auto& op : operations)
        {
            Segment s;

            if (op.type == Operation::FEED)
            {
                s.type = Segment::LINE;
                s.length = op.length;
            }
            else if (op.type == Operation::BEND)
            {
                s.type = Segment::ARC;

                if (op.R > 1e-9)
                    s.curvature = 1.0 / op.R;
                else
                    s.curvature = 0.0;

                s.angle = op.angle;
                s.bendDirection = op.bendDirection;
            }
            else if (op.type == Operation::ROTATE)
            {
                s.type = Segment::ROTATE;
                s.rotAngle = op.angle;
                s.rotationDirection = op.rotationDirection;
            }

            segments.push_back(s);
        }
    }

   




   

private:
    
    void buildCurveFromOperations() const
    {
        // =====================================================
        // Convert user operations into curvature-driven segments.
        //
        // This is the new CAD path:
        //
        // FEED   -> Line segment
        // BEND   -> Circular arc segment
        // ROTATE -> Rotation-only frame segment
        //
        // Later:
        // HELIX / ROLL / STRETCH operations will also produce
        // curve segments here or through a ManufacturingPass.
        // =====================================================

        curve.clear();

        for (const auto& op : operations)
        {
            if (op.type == Operation::FEED)
            {
                curve.addSegment(
                    PipeCurveSegment::makeLine(
                        op.length
                    )
                );
            }
            else if (op.type == Operation::BEND)
            {
                curve.addSegment(
                    PipeCurveSegment::makeCircularArc(
                        op.R,
                        op.angle,
                        op.bendDirection
                    )
                );
            }
            else if (op.type == Operation::ROTATE)
            {
                curve.addSegment(
                    PipeCurveSegment::makeRotationOnly(
                        op.angle,
                        op.rotationDirection
                    )
                );
            }
        }
    }
};