#pragma once

#include <cmath>
#include <limits>
#include <vector>

#include "Core/Geometry/PipeNode.h"
#include "Core/Geometry/Frame.h"

// =====================================================
// PIPE CURVE SAMPLE QUERY
//
// Utility for querying sampled PipeNode lists.
//
// Current use:
// - find approximate frame at arc length s
//
// This is sample-based for now.
// Later we can add analytic exact frame queries.
// =====================================================

struct PipeCurveSampleQueryResult
{
    bool valid = false;

    double targetS = 0.0;
    double nearestS = 0.0;
    double error = 0.0;

    size_t nodeIndex = 0;

    Frame frame;
};

class PipeCurveSampleQuery
{
public:
    static PipeCurveSampleQueryResult findFrameAtArcLength(
        const std::vector<PipeNode>& nodes,
        double targetS)
    {
        PipeCurveSampleQueryResult result;
        result.targetS = targetS;

        if (nodes.empty())
            return result;

        if (targetS <= 0.0)
        {
            result.valid = true;
            result.nearestS = 0.0;
            result.error = std::abs(targetS);
            result.nodeIndex = 0;
            result.frame = frameFromNode(nodes.front());
            return result;
        }

        double accumulated = 0.0;

        result.valid = true;
        result.nodeIndex = 0;
        result.nearestS = 0.0;
        result.error = std::numeric_limits<double>::max();
        result.frame = frameFromNode(nodes.front());

        for (size_t i = 1; i < nodes.size(); ++i)
        {
            double step =
                (nodes[i].pos - nodes[i - 1].pos).length();

            accumulated += step;

            double error =
                std::abs(accumulated - targetS);

            if (error < result.error)
            {
                result.error = error;
                result.nearestS = accumulated;
                result.nodeIndex = i;
                result.frame = frameFromNode(nodes[i]);
            }
        }

        return result;
    }

private:
    static Frame frameFromNode(
        const PipeNode& node)
    {
        Frame f;

        f.P = node.pos;
        f.T = node.T;
        f.N = node.N;
        f.B = node.B;

        return f;
    }
};
