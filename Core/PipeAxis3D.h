#pragma once
#include <glm/glm.hpp> 
#include <vector>
#include <cmath>
#include <iostream>
#include "Core/Math/Vec3D.h"


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
    //======================================================================
    // OPERATION (User input for pipe construction)
    // INPUT LAYER 
    // =======================================================================
    // 
    struct Operation
    {
        enum Type
        {
            FEED,
            BEND,
            ROTATE
        };

        Type type = FEED;

        double length = 0.0;
        double R = 0.0;
        double angle = 0.0;
        double rotAngle = 0.0;
    };
    // =====================================================================
    // NODE (Point on pipe centerline)
    // =====================================================================
    struct Node
    {
        Vec3D pos;
        Vec3D T;

        glm::vec3 getPosition() const
        {
            return glm::vec3(
                (float)pos.x,
                (float)pos.y,
                (float)pos.z
            );
        }
    };


    struct Segment
    {
        enum Type
        {
            LINE,
            ARC,
            ROTATE
        };

        Type type = LINE;

        double length = 0.0;   // used for LINE
        double curvature = 0.0; // ? = 1/R (ARC)
        double angle = 0.0;     // ARC bending angle
        double rotAngle = 0.0;  // twist

        double arcLength() const
        {
            if (curvature == 0.0) return 0.0;
            return angle / curvature; // R * angle
        }
    };



public:
    // =====================================================================
    // CONSTRUCTOR
    // =====================================================================
    explicit PipeAxis3D(double stepSize = 1.0)
        : ds(stepSize), dirty(true)
    {
    }

    // =====================================================================
    // OPERATION INTERFACE (Store operations)
    // =====================================================================

    /// Add straight feed
    void addFeed(double length)
    {
        Operation op;
        op.type = Operation::FEED;
        op.length = length;
        ops.push_back(op);
        markDirty();
    }

    /// Add arc bend
    void addBend(double radius, double angle)
    {
        Operation op;
        op.type = Operation::BEND;
        op.R = radius;
        op.angle = angle;
        ops.push_back(op);
        markDirty();
    }

    // Add rotation (twist)
    void addRotate(double angle)
    {
        Operation op;
        op.type = Operation::ROTATE;
        op.rotAngle = angle;
        ops.push_back(op);
        markDirty();
    }

    // =====================================================================
    // EDITING API (Modify operations)
    // =====================================================================

    // ================================
// EDITING ? ALWAYS MODIFY ops!
// ================================

    void setFeedLength(size_t index, double length)
    {
        if (index < ops.size() && ops[index].type == Operation::FEED)
        {
            ops[index].length = length;
            markDirty();
        }
    }

    void setBendRadius(size_t index, double radius)
    {
        if (index < ops.size() && ops[index].type == Operation::BEND)
        {
            ops[index].R = radius;
            markDirty();
        }
    }

    void setBendAngle(size_t index, double angle)
    {
        if (index < ops.size() && ops[index].type == Operation::BEND)
        {
            ops[index].angle = angle;
            markDirty();
        }
    }

    // Clear all operations and rebuild
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

        // 1. SIMULATION
        buildSegments();

        // 2. RENDER
        buildNodes();

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
        Segment seg;
        double total = 0.0;
        for (const auto& seg : segments)
        {
            if (seg.type == Segment::LINE)
                total += seg.length;
            else if (seg.type == Segment::ARC)
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
            if (seg.type == Segment::LINE)
            {
                std::cout << i << ": FEED " << seg.length << " mm\n";
            }
            else if (seg.type == Segment::ARC)
            {
                std::cout << i << ": BEND R=" << seg.curvature << " mm, angle="
                    << (seg.angle * 180.0 / PI) << " deg\n";
            }
            else if (seg.type == Segment::ROTATE)
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
    std::vector<Operation> ops;     // INPUT
    std::vector<Segment> segments;  // SIMULATION Operations to execute
    std::vector<Node> nodes;        // RENDER Resulting geometry  
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
	
    

   

    void orthonormalizeFrame(Frame& f)
    {
        // =====================================================
        // STEP 1
        // Normalize tangent
        // =====================================================

        f.T = f.T.normalized();

        // =====================================================
        // STEP 2
        // Rebuild binormal from T and N
        // =====================================================

        f.B = cross(f.T, f.N).normalized();

        // =====================================================
        // STEP 3
        // Rebuild normal from corrected B and T
        // =====================================================

        f.N = cross(f.B, f.T).normalized();
    }

    // ========================================================================
   //Build SEGMENTS
     //========================================================================

    void buildSegments()
    {
        segments.clear();

        for (const auto& op : ops)
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
                s.curvature = 1.0 / op.R;   // ? key idea
                s.angle = op.angle;
            }
            else if (op.type == Operation::ROTATE)
            {
                s.type = Segment::ROTATE;
                s.rotAngle = op.rotAngle;
            }

            segments.push_back(s);
        }
    }

    void buildNodes()
    {
        nodes.clear();

        Frame frame;
        frame.P = { 0,0,0 };
        frame.T = { 1,0,0 };
        frame.N = { 0,1,0 };
        frame.B = { 0,0,1 };

        nodes.push_back({ frame.P, frame.T });

        for (const auto& seg : segments)
        {
            if (seg.type == Segment::LINE)
                buildLine(frame, seg.length);

            else if (seg.type == Segment::ARC)
                buildArc(frame, seg.curvature, seg.angle);

            else if (seg.type == Segment::ROTATE)
                buildRotate(frame, seg.rotAngle);
        }
    }


    //======================================================================
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

    void buildArc(Frame& frame, double curvature, double angle)
    {
        //double R = 1.0 / curvature;

        //double arcLength = R * angle;
		double arcLength = angle / curvature; // !!! key idea 
        int steps = std::max(1, (int)(arcLength / ds));
        double dA = curvature * ds;

        //Vec3D center = frame.P + frame.N * R;

        //double phi = atan2(frame.P.y - center.y, frame.P.x - center.x);

        for (int i = 0; i < steps; ++i)
        {
            frame.T = rotateAroundAxis(frame.T, frame.B, dA);
            frame.N = rotateAroundAxis(frame.N, frame.B, dA);
            orthonormalizeFrame(frame);

            //phi += dA;
            frame.P = frame.P + frame.T * ds;
            //frame.P.x = center.x + R * cos(phi);
            //frame.P.y = center.y + R * sin(phi);

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

    // ============================================

    // MATH

   
};