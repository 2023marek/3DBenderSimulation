#pragma once

#include <vector>
#include <cmath>
#include <fstream>
#include <iostream>
#include "../Core/Math/Vec2D.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

class PipeAxis2D
{
public:
    bool dirty = true;
public:

    // =========================
    // ===== RENDER LAYER ======
    // =========================
    struct Node
    {
        Vec2D pos;
        double theta = 0.0;
    };

    // =========================
    // ===== INPUT LAYER =======
    // =========================
    enum class BendDirection
    {
        CCW = 1,
        CW = -1
    };

    struct Operation
    {
        enum Type
        {
            FEED,
            BEND
        };

        Type type = FEED;

        double length = 0.0;
        double R = 0.0;
        double angle = 0.0;
        BendDirection dir = BendDirection::CCW;
    };

    // =========================
    // === SIMULATION LAYER ====
    // =========================
    struct Segment
    {
        enum Type
        {
            LINE,
            ARC
        };

        Type type = LINE;

        double length = 0.0;

        // ARC only
        double R = 0.0;
        double angle = 0.0;
        BendDirection dir = BendDirection::CCW;
    };

public:

    explicit PipeAxis2D(double segmentLength)
        : ds(segmentLength)
    {
    }

    // =========================
    // INPUT API
    // =========================
    void addFeed(double L)
    {
        Operation op;
        op.type = Operation::FEED;
        op.length = L;
        ops.push_back(op);
    }

    void addBend(double R, double angle, BendDirection dir)
    {
        Operation op;
        op.type = Operation::BEND;
        op.R = R;
        op.angle = angle;
        op.dir = dir;
        ops.push_back(op);
    }
    // =========================
// SAFE EDITING API
// =========================

// --- LINE ---
    void setFeedLength(size_t i, double L)
       
    {
        dirty = true;
        if (i >= segments.size()) return;

        auto& s = segments[i];
        if (s.type != Segment::LINE) return;

        s.length = L;
    }

    // --- ARC: change radius, keep angle ---
    void setBendRadius(size_t i, double R)
    {
		dirty = true;
        if (i >= segments.size()) return;

        auto& s = segments[i];
        if (s.type != Segment::ARC) return;

        s.R = R;

        // IMPORTANT:
        // angle stays the same ? arc becomes longer/shorter
        // so we must recompute length
        s.length = s.R * s.angle;
    }

    // --- ARC: change angle, keep radius ---
    void setBendAngle(size_t i, double angle)
    {
		dirty = true;
        if (i >= segments.size()) return;

        auto& s = segments[i];
        if (s.type != Segment::ARC) return;

        s.angle = angle;

        // IMPORTANT:
        // radius stays the same ? arc length changes
        s.length = s.R * s.angle;
    }

    // --- ARC: change length, keep radius ---
    void setArcLengthKeepRadius(size_t i, double L)
    {
		dirty = true;
        if (i >= segments.size()) return;

        auto& s = segments[i];
        if (s.type != Segment::ARC) return;

        s.length = L;

        // IMPORTANT:
        // radius fixed ? angle must change
        s.angle = s.length / s.R;
    }

    // --- ARC: change length, keep angle ---
    void setArcLengthKeepAngle(size_t i, double L)
    {
		dirty = true;
        if (i >= segments.size()) return;

        auto& s = segments[i];
        if (s.type != Segment::ARC) return;

        s.length = L;

        // IMPORTANT:
        // angle fixed ? radius must change
        s.R = s.length / s.angle;
    }
    void modifyBend(size_t i, double R, double angle)
    {
        if (i >= segments.size()) return;

        auto& s = segments[i];
        if (s.type != Segment::ARC) return;

        s.R = R;
        s.angle = angle;
        s.length = R * angle;

        dirty = true; // ?? MISSING before
    }
    
    // =========================
    // BUILD PIPELINE
    // =========================
  
    void build()
    {
        if (!dirty) return;

        buildNodesFromSegments();  // ONLY rebuild geometry
        dirty = false;
    }
    void rebuildFromOperations()
    {
        buildSegments();           // ops ? segments
        dirty = true;
    }
    const std::vector<Node>& getNodes() const
    {
        return nodes;
    }
    //std::vector<Segment>& getSegments()
    //{
       // return segments;
    //}
    const std::vector<Segment>& getSegments() const
    {
        return segments;
    }

	// =========================
	// DEBUGING HELPER
	// =========================
    void printSegments() const
    {
        for (size_t i = 0; i < segments.size(); i++)
        {
            const auto& s = segments[i];

            if (s.type == Segment::LINE)
            {
                std::cout << i << ": LINE L=" << s.length << "\n";
            }
            else
            {
                std::cout << i << ": ARC R=" << s.R
                    << " angle=" << s.angle
                    << " L=" << s.length << "\n";
            }
        }
    }
    // =========================
    // SIMULATION EXAMPLE
    // =========================
    void applySpringback(double factor)
    {
        for (auto& s : segments)
        {
            if (s.type == Segment::ARC)
            {
                s.R *= factor;
                s.angle /= factor;
                s.length = s.R * s.angle;
            }
        }

        buildNodesFromSegments();
    }

private:

    std::vector<Operation> ops;     // input
    std::vector<Segment> segments; // simulation (persistent curvature)
    std::vector<Node> nodes;       // rendering

    double ds;

private:

    // =========================
    // SIMULATION LAYER
    // =========================
    void buildSegments()
       
    {
       
        segments.clear();

        for (const auto& op : ops)
        {
            if (op.type == Operation::FEED)
            {
                Segment s;
                s.type = Segment::LINE;
                s.length = op.length;

                segments.push_back(s);
            }
            else
            {
                Segment s;
                s.type = Segment::ARC;
                s.R = op.R;
                s.angle = op.angle;
                s.dir = op.dir;
                s.length = op.R * op.angle;

                segments.push_back(s);
            }
		} dirty = true;
    }

    // =========================
    // RENDER LAYER
    // =========================
    void buildNodesFromSegments()
    {
        nodes.clear();

        Vec2D P{ 0.0, 0.0 };
        double theta = 0.0;

        nodes.push_back({ P, theta });

        for (const auto& seg : segments)
        {
            if (seg.type == Segment::LINE)
            {
                buildFeed(P, theta, seg.length);
            }
            else
            {
                buildBend(P, theta, seg.R, seg.angle, seg.dir);
            }
        }
    }

    void buildFeed(Vec2D& P, double& theta, double length)
    {
        int n = static_cast<int>(std::round(length / ds));

        for (int i = 0; i < n; i++)
        {
            P.x += ds * std::cos(theta);
            P.y += ds * std::sin(theta);

            nodes.push_back({ P, theta });
        }
    }

    void buildBend(Vec2D& P,
        double& theta,
        double R,
        double angle,
        BendDirection dir)
    {
        double arcLength = R * angle;

        int n = static_cast<int>(std::round(arcLength / ds));
        if (n <= 0) return;

        double dAlpha = angle / n;
        int s = static_cast<int>(dir);

        Vec2D T{ std::cos(theta), std::sin(theta) };
        Vec2D N;

        if (dir == BendDirection::CCW)
            N = { -T.y, T.x };
        else
            N = { T.y, -T.x };

        Vec2D C
        {
            P.x + R * N.x,
            P.y + R * N.y
        };

        double phi0 = std::atan2(P.y - C.y, P.x - C.x);

        for (int i = 1; i <= n; i++)
        {
            double phi = phi0 + s * i * dAlpha;

            P.x = C.x + R * std::cos(phi);
            P.y = C.y + R * std::sin(phi);

            theta += s * dAlpha;

            nodes.push_back({ P, theta });
        }
    }
};