#pragma once

#include <vector>

#include "Core/Curve/PipeCurveSegment.h"

// =====================================================
// PIPE CURVE
//
// Ordered list of curvature-driven pipe segments.
//
// This is the future common representation for:
// - CAD preview
// - manufacturing output
// - physics correction
// - springback
// - collision sampling
// =====================================================

struct PipeCurve
{
    std::vector<PipeCurveSegment> segments;

    void clear()
    {
        segments.clear();
    }

    bool empty() const
    {
        return segments.empty();
    }

    size_t size() const
    {
        return segments.size();
    }

    void addSegment(const PipeCurveSegment& segment)
    {
        segments.push_back(segment);
    }

    double totalLength() const
    {
        double total = 0.0;

        for (const auto& segment : segments)
        {
            total += segment.length;
        }

        return total;
    }
};
