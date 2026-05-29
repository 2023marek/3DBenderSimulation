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

    void build() const
    {
        segments.clear();
        nodes.clear();

        resetFrame();

        buildSegments();
        buildNodes();

        dirty = false;
    }

private:
    double ds = 0.5;

    std::vector<Operation> operations;

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

    void buildNodes() const
    {
        nodes.push_back({
            currentFrame.P,
            currentFrame.T,
            currentFrame.N,
            currentFrame.B
            });

        for (const auto& s : segments)
        {
            if (s.type == Segment::LINE)
            {
                buildLine(s.length);
            }
            else if (s.type == Segment::ARC)
            {
                buildArc(s);
            }
            else if (s.type == Segment::ROTATE)
            {
                buildRotate(s);
            }
        }

        
    }




    void buildLine(double length) const
    {
        if (length <= 0.0 || ds <= 1e-9)
            return;

        int steps =
            std::max(1, static_cast<int>(std::ceil(length / ds)));

        double stepLength =
            length / static_cast<double>(steps);

        for (int i = 1; i <= steps; ++i)
        {
            currentFrame.P =
                currentFrame.P +
                currentFrame.T.normalized() * stepLength;

            nodes.push_back({
                currentFrame.P,
                currentFrame.T,
                currentFrame.N,
                currentFrame.B
                });
        }
    }

    void buildArc(const Segment& s) const
    {
        if (std::abs(s.curvature) < 1e-12)
            return;

        if (s.angle <= 0.0)
            return;

        double radius =
            1.0 / std::abs(s.curvature);

        double arcLength =
            radius * s.angle;

        int steps =
            std::max(1, static_cast<int>(std::ceil(arcLength / ds)));

        double dA =
            s.angle / static_cast<double>(steps);

        for (int i = 1; i <= steps; ++i)
        {
            Vec3D prevT = currentFrame.T;

            double signedDA =
                dA * bendDirectionSign(s.bendDirection);

            currentFrame.T =
                rotateAroundAxis(
                    currentFrame.T,
                    currentFrame.B,
                    signedDA
                ).normalized();

            transportFrame(prevT, currentFrame.T, currentFrame);

            Vec3D midT =
                (prevT + currentFrame.T).normalized();

            if (midT.lengthSquared() < 1e-12)
                midT = currentFrame.T;

            currentFrame.P =
                currentFrame.P + midT * (radius * dA);

            nodes.push_back({
                currentFrame.P,
                currentFrame.T,
                currentFrame.N,
                currentFrame.B
                });
        }
    }

    void buildRotate(const Segment& s) const
    {
        double signedAngle =
            s.rotAngle * rotationDirectionSign(s.rotationDirection);

        currentFrame.N =
            rotateAroundAxis(
                currentFrame.N,
                currentFrame.T,
                signedAngle
            ).normalized();

        currentFrame.B =
            rotateAroundAxis(
                currentFrame.B,
                currentFrame.T,
                signedAngle
            ).normalized();

        orthonormalizeFrame(currentFrame);

        nodes.push_back({
            currentFrame.P,
            currentFrame.T,
            currentFrame.N,
            currentFrame.B
            });
    }

private:
    Vec3D rotateAroundAxis(
        const Vec3D& v,
        const Vec3D& axis,
        double angle) const
    {
        Vec3D k = axis.normalized();

        if (k.lengthSquared() < 1e-12)
            return v;

        return v * std::cos(angle)
            + cross(k, v) * std::sin(angle)
            + k * dot(k, v) * (1.0 - std::cos(angle));
    }

    void transportFrame(
        const Vec3D& oldT,
        const Vec3D& newT,
        Frame& frame) const
    {
        Vec3D axis = cross(oldT, newT);

        if (axis.lengthSquared() < 1e-12)
        {
            orthonormalizeFrame(frame);
            return;
        }

        axis = axis.normalized();

        double d =
            dot(oldT.normalized(), newT.normalized());

        d = std::max(-1.0, std::min(1.0, d));

        double angle =
            std::acos(d);

        frame.N =
            rotateAroundAxis(frame.N, axis, angle).normalized();

        frame.B =
            rotateAroundAxis(frame.B, axis, angle).normalized();

        orthonormalizeFrame(frame);
    }

    void orthonormalizeFrame(Frame& frame) const
    {
        frame.T = frame.T.normalized();

        frame.N =
            (frame.N - frame.T * dot(frame.N, frame.T)).normalized();

        frame.B =
            cross(frame.T, frame.N).normalized();

        frame.N =
            cross(frame.B, frame.T).normalized();
    }
};