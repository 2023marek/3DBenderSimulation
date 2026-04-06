#pragma once
#include <vector>
#include <cmath>
#include <fstream>
#include <iostream>

#include "../Core/Math/Vec3D.h"

static constexpr double PI = 3.14159265358979323846;

class PipeAxis3D
{
public:
    bool dirty = true;

    // =========================
    // FRAME (core of 3D)
    // =========================
    struct Frame
    {
        Vec3D P; // position
        Vec3D T; // tangent
        Vec3D N; // normal
        Vec3D B; // binormal
    };

    struct Node
    {
        Vec3D pos;
		Vec3D T; // tangent at node (for visualization)
    };

    enum class SegmentType
    {
        LINE,
        ARC,
        ROTATE
    };

    struct Segment
    {
        SegmentType type = SegmentType::LINE;

        double length = 0.0;

        // ARC
        double R = 0.0;
        double angle = 0.0;

        // ROTATE
        double rotAngle = 0.0;
    };

public:

    explicit PipeAxis3D(double step)
        : ds(step)
    {
    }

    // =========================
    // INPUT
    // =========================
    void addFeed(double L)
    {
        Segment s;
        s.type = SegmentType::LINE;
        s.length = L;
        segments.push_back(s);
        dirty = true;
    }

    void addBend(double R, double angle)
    {
        Segment s;
        s.type = SegmentType::ARC;
        s.R = R;
        s.angle = angle;
        s.length = R * angle;
        segments.push_back(s);
        dirty = true;
    }

    void addRotate(double angle)
    {
        Segment s;
        s.type = SegmentType::ROTATE;
        s.rotAngle = angle;
        segments.push_back(s);
        dirty = true;
    }

    // =========================
// SAFE EDITING API (3D)
// =========================

// --- LINE ---
    void setFeedLength(size_t i, double L)
    {
        if (i >= segments.size()) return;

        auto& s = segments[i];
        if (s.type != SegmentType::LINE) return;

        s.length = L;
        dirty = true;
    }

    // --- ARC: change radius, keep angle ---
    void setBendRadius(size_t i, double R)
    {
        if (i >= segments.size()) return;

        auto& s = segments[i];
        if (s.type != SegmentType::ARC) return;

        s.R = R;

        // IMPORTANT:
        // angle stays constant ? arc length must change
        s.length = s.R * s.angle;

        dirty = true;
    }

    // --- ARC: change angle, keep radius ---
    void setBendAngle(size_t i, double angle)
    {
        if (i >= segments.size()) return;

        auto& s = segments[i];
        if (s.type != SegmentType::ARC) return;

        s.angle = angle;

        // IMPORTANT:
        // radius fixed ? arc length changes
        s.length = s.R * s.angle;

        dirty = true;
    }

    // --- ARC: full control ---
    void modifyBend(size_t i, double R, double angle)
    {
        if (i >= segments.size()) return;

        auto& s = segments[i];
        if (s.type != SegmentType::ARC) return;

        s.R = R;
        s.angle = angle;
        s.length = R * angle;

        dirty = true;
    }

    // --- ROTATE ---
    void setRotation(size_t i, double angle)
    {
        if (i >= segments.size()) return;

        auto& s = segments[i];
        if (s.type != SegmentType::ROTATE) return;

        s.rotAngle = angle;

        dirty = true;
    }
	// ---HELPER ---
    void printSegments() const
    {
        for (size_t i = 0; i < segments.size(); i++)
        {
            const auto& s = segments[i];

            if (s.type == SegmentType::LINE)
            {
                std::cout << i << ": LINE L=" << s.length << "\n";
            }
            else if (s.type == SegmentType::ARC)
            {
                std::cout << i << ": ARC R=" << s.R
                    << " angle=" << s.angle
                    << " L=" << s.length << "\n";
            }
            else
            {
                std::cout << i << ": ROTATE angle=" << s.rotAngle << "\n";
            }
        }
    }
    // =========================
    // BUILD
    // =========================
    void build()
    {
        if (!dirty) return;

        nodes.clear();

        Frame f;
        f.P = { 0,0,0 };
        f.T = { 1,0,0 };
        f.N = { 0,1,0 };
        f.B = { 0,0,1 };

        nodes.push_back({ f.P ,f.T});

        for (const auto& s : segments)
        {
            if (s.type == SegmentType::LINE)
                buildLine(f, s.length);

            else if (s.type == SegmentType::ROTATE)
                applyRotation(f, s.rotAngle);

            else if (s.type == SegmentType::ARC)
                buildArc(f, s.R, s.angle);
        }

        dirty = false;
    }

    const std::vector<Node>& getNodes() const
    {
        return nodes;
    }

private:

    std::vector<Segment> segments;
    std::vector<Node> nodes;

    double ds;

private:

    // =========================
    // MATH CORE
    // =========================
    Vec3D rotateAroundAxis(const Vec3D& v, const Vec3D& axis, double angle)
    {
        Vec3D a = normalize(axis);

        return v * cos(angle)
            + cross(a, v) * sin(angle)
            + a * dot(a, v) * (1 - cos(angle));
    }

    // =========================
    // OPERATIONS
    // =========================
    void buildLine(Frame& f, double length)
    {
        int n = std::max(1, (int)(length / ds));

        for (int i = 0; i < n; i++)
        {
            f.P = f.P + f.T * ds;
            nodes.push_back({ f.P });
        }
    }

    void applyRotation(Frame& f, double angle)
    {
        // rotate around T (twist)
        f.N = rotateAroundAxis(f.N, f.T, angle);
        f.B = rotateAroundAxis(f.B, f.T, angle);
    }

    void buildArc(Frame& f, double R, double angle)
    {
        double arcLength = R * angle;
        int n = std::max(1, (int)(arcLength / ds));

        double dAlpha = angle / n;

        for (int i = 0; i < n; i++)
        {
            // rotate frame around B (bend)
            f.T = rotateAroundAxis(f.T, f.B, dAlpha);
            f.N = rotateAroundAxis(f.N, f.B, dAlpha);

            // move along arc
            f.P = f.P + f.T * (R * dAlpha);

            nodes.push_back({ f.P });
        }
    }
};