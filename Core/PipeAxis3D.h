#pragma once

#include <vector>
#include <cmath>
#include <iostream>
#include "../Core/Math/Vec3D.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

// =========================================================================
// PHASE 5: CNC-COMPLIANT PIPE AXIS 3D
// =========================================================================
//
// KEY PRINCIPLE:
//   "Never mutate geometry directly. Store operations, rebuild from scratch."
//
// This is the industry-standard approach used in professional CNC software:
//   ? Deterministic (same operations always produce same geometry)
//   ? Editable (change any operation, rebuild automatically)
//   ? Realistic (matches real CNC machine behavior)
//   ? Scalable (handles unlimited operations)
//
// =========================================================================

class PipeAxis3D
{
public:
    // =====================================================================
    // FRAME (3D coordinate system along pipe)
    // =====================================================================
    struct Frame
    {
        Vec3D P;  // Position
        Vec3D T;  // Tangent (direction pipe is pointing)
        Vec3D N;  // Normal (perpendicular to tangent)
        Vec3D B;  // Binormal (perpendicular to both)
    };

    // =====================================================================
    // NODE (Point on pipe centerline)
    // =====================================================================
    struct Node
    {
        Vec3D pos;  // 3D position
        Vec3D T;    // Tangent at this point
    };

    // =====================================================================
    // SEGMENT (Operation abstraction)
    // =====================================================================
    enum class SegmentType
    {
        LINE,    // Straight feed
        ARC,     // Curved bend
        ROTATE   // Twist around axis
    };

    struct Segment
    {
        SegmentType type = SegmentType::LINE;

        // LINE parameters
        double length = 0.0;

        // ARC parameters
        double R = 0.0;           // Radius
        double angle = 0.0;       // Total angle

        // ROTATE parameters
        double rotAngle = 0.0;    // Twist angle

        // Computed
        double arcLength() const { return R * angle; }
    };

public:
    // =====================================================================
    // CONSTRUCTOR
    // =====================================================================
    explicit PipeAxis3D(double stepSize = 5.0)
        : ds(stepSize), dirty(true)
    {
    }

    // =====================================================================
    // OPERATION INTERFACE (Store operations)
    // =====================================================================

    /// Add straight feed
    void addFeed(double length)
    {
        Segment s;
        s.type = SegmentType::LINE;
        s.length = length;
        segments.push_back(s);
        markDirty();
    }

    /// Add arc bend
    void addBend(double radius, double angle)
    {
        Segment s;
        s.type = SegmentType::ARC;
        s.R = radius;
        s.angle = angle;
        segments.push_back(s);
        markDirty();
    }

    /// Add rotation (twist)
    void addRotate(double angle)
    {
        Segment s;
        s.type = SegmentType::ROTATE;
        s.rotAngle = angle;
        segments.push_back(s);
        markDirty();
    }

    // =====================================================================
    // EDITING API (Modify operations)
    // =====================================================================

    /// Change feed length
    void setFeedLength(size_t index, double length)
    {
        if (index < segments.size() && segments[index].type == SegmentType::LINE)
        {
            segments[index].length = length;
            markDirty();
        }
    }

    /// Change bend radius
    void setBendRadius(size_t index, double radius)
    {
        if (index < segments.size() && segments[index].type == SegmentType::ARC)
        {
            segments[index].R = radius;
            markDirty();
        }
    }

    /// Change bend angle
    void setBendAngle(size_t index, double angle)
    {
        if (index < segments.size() && segments[index].type == SegmentType::ARC)
        {
            segments[index].angle = angle;
            markDirty();
        }
    }

    /// Clear all operations and rebuild
    void clear()
    {
        segments.clear();
        nodes.clear();
        markDirty();
    }

    // =====================================================================
    // BUILD (Reconstruct geometry from operations)
    // =====================================================================

    void build()
    {
        if (!dirty) return;

        // Step 1: Clear previous nodes
        nodes.clear();

        // Step 2: Initialize starting frame (origin, pointing +X)
        Frame frame;
        frame.P = Vec3D{ 0, 0, 0 };
        frame.T = Vec3D{ 1, 0, 0 };  // Pointing forward
        frame.N = Vec3D{ 0, 1, 0 };  // Up
        frame.B = Vec3D{ 0, 0, 1 };  // Right

        // Step 3: Add initial node
        nodes.push_back({ frame.P, frame.T });

        // Step 4: Process each segment
        for (const auto& segment : segments)
        {
            if (segment.type == SegmentType::LINE)
            {
                buildLine(frame, segment.length);
            }
            else if (segment.type == SegmentType::ARC)
            {
                buildArc(frame, segment.R, segment.angle);
            }
            else if (segment.type == SegmentType::ROTATE)
            {
                buildRotate(frame, segment.rotAngle);
            }
        }

        dirty = false;
    }

    // =====================================================================
    // QUERY
    // =====================================================================

    const std::vector<Node>& getNodes() const { return nodes; }
    const std::vector<Segment>& getSegments() const { return segments; }
    size_t getSegmentCount() const { return segments.size(); }

    // =====================================================================
    // PROPERTIES
    // =====================================================================

    double getTotalLength() const
    {
        double total = 0.0;
        for (const auto& seg : segments)
        {
            if (seg.type == SegmentType::LINE)
                total += seg.length;
            else if (seg.type == SegmentType::ARC)
                total += seg.arcLength();
        }
        return total;
    }

    void printSegments() const
    {
        std::cout << "\n=== Pipe Segments ===\n";
        for (size_t i = 0; i < segments.size(); ++i)
        {
            const auto& seg = segments[i];
            if (seg.type == SegmentType::LINE)
            {
                std::cout << i << ": FEED " << seg.length << " mm\n";
            }
            else if (seg.type == SegmentType::ARC)
            {
                std::cout << i << ": BEND R=" << seg.R << " mm, angle="
                    << (seg.angle * 180.0 / PI) << " deg\n";
            }
            else if (seg.type == SegmentType::ROTATE)
            {
                std::cout << i << ": ROTATE " << (seg.rotAngle * 180.0 / PI) << " deg\n";
            }
        }
        std::cout << "====================\n\n";
    }

private:
    // =====================================================================
    // DATA
    // =====================================================================
    std::vector<Segment> segments;  // Operations to execute
    std::vector<Node> nodes;        // Resulting geometry
    double ds;                      // Segment step size (mm)
    bool dirty;                     // Rebuild needed?

    // =====================================================================
    // IMPLEMENTATION
    // =====================================================================

    void markDirty() { dirty = true; }

    Vec3D rotateAroundAxis(const Vec3D& v, const Vec3D& axis, double angle) const
    {
        Vec3D a = normalize(axis);
        return v * cos(angle)
            + cross(a, v) * sin(angle)
            + a * dot(a, v) * (1.0 - cos(angle));
    }

    /// Build straight line segment
    void buildLine(Frame& frame, double length)
    {
        int stepCount = std::max(1, (int)(length / ds));

        for (int i = 0; i < stepCount; ++i)
        {
            // Move along current tangent
            frame.P = frame.P + frame.T * ds;
            nodes.push_back({ frame.P, frame.T });
        }
    }

    /// Build arc (bend) segment
    void buildArc(Frame& frame, double radius, double angle)
    {
        double arcLength = radius * angle;
        int stepCount = std::max(1, (int)(arcLength / ds));
        double angleStep = angle / stepCount;

        // Circle center: perpendicular to tangent at distance radius
        Vec3D center = frame.P + frame.N * radius;

        // Initial angle on circle
        double phi = atan2(frame.P.y - center.y, frame.P.x - center.x);

        for (int i = 0; i < stepCount; ++i)
        {
            // Rotate frame around binormal (creates bend)
            frame.T = rotateAroundAxis(frame.T, frame.B, angleStep);
            frame.N = rotateAroundAxis(frame.N, frame.B, angleStep);
            // B stays same (rotation axis)

            // Move along arc
            phi += angleStep;
            frame.P.x = center.x + radius * cos(phi);
            frame.P.y = center.y + radius * sin(phi);

            nodes.push_back({ frame.P, frame.T });
        }
    }

    /// Build rotation (twist) segment
    void buildRotate(Frame& frame, double angle)
    {
        // Twist frame around tangent
        // Position unchanged, but N and B rotate
        frame.N = rotateAroundAxis(frame.N, frame.T, angle);
        frame.B = rotateAroundAxis(frame.B, frame.T, angle);
        // No new nodes added - geometry unchanged
    }
};