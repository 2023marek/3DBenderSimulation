#pragma once

#include <vector>

#include "Core/Geometry/PipeNode.h"
#include "Core/Geometry/Frame.h"

// =====================================================
// PIPE CURVE NODE QUERY
//
// Utility for querying sampled PipeNode lists by index.
//
// Current use:
// - support InsertAtNodeIndex metadata test
//
// Later:
// - mouse picking can select node index
// - insertion can start from selected node frame
// =====================================================

struct PipeCurveNodeQueryResult
{
    bool valid = false;

    size_t nodeIndex = 0;
    size_t nodeCount = 0;

    Frame frame;
};

class PipeCurveNodeQuery
{
public:
    static PipeCurveNodeQueryResult findFrameAtNodeIndex(
        const std::vector<PipeNode>& nodes,
        size_t index)
    {
        PipeCurveNodeQueryResult result;

        result.nodeIndex =
            index;

        result.nodeCount =
            nodes.size();

        if (index >= nodes.size())
            return result;

        result.valid =
            true;

        result.frame =
            frameFromNode(
                nodes[index]
            );

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