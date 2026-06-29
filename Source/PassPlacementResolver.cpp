#include "Core/Forming/PassPlacementResolver.h"

#include "Core/Sampling/PipeCurveSampler.h"
#include "Core/Sampling/PipeCurveSampleQuery.h"
#include "Core/Sampling/PipeCurveNodeQuery.h"

PassPlacementResolution resolvePassPlacementFrame(
    const PassPlacement& placement,
    const PipeCurve& baseCurve,
    double sampleStep
)
{
    PassPlacementResolution result;

    if (placement.mode
        == PassPlacementMode::ExplicitStartFrame)
    {
        result.valid = true;
        result.frame = placement.startFrame;
        result.arcLength = 0.0;

        return result;
    }

    auto baseNodes =
        PipeCurveSampler::sample(
            baseCurve,
            sampleStep
        );

    if (baseNodes.empty())
        return result;


    if (placement.mode
        == PassPlacementMode::AppendToPrevious)
    {
        const PipeNode& lastNode =
            baseNodes.back();

        result.valid = true;
        result.frame.P = lastNode.pos;
        result.frame.T = lastNode.T;
        result.frame.N = lastNode.N;
        result.frame.B = lastNode.B;

        result.arcLength =
            PipeCurveNodeQuery::arcLengthAtNodeIndex(
                baseNodes,
                baseNodes.size() - 1
            );

        return result;
    }

    double resolvedArcLength = 0.0;

    if (placement.mode
        == PassPlacementMode::InsertAtArcLength)
    {
        resolvedArcLength =
            placement.arcLength;
    }
    else if (placement.mode
        == PassPlacementMode::InsertAtNodeIndex)
    {
        resolvedArcLength =
            PipeCurveNodeQuery::arcLengthAtNodeIndex(
                baseNodes,
                placement.nodeIndex
            );
    }
    else
    {
        return result;
    }

    auto frameQuery =
        PipeCurveSampleQuery::findFrameAtArcLength(
            baseNodes,
            resolvedArcLength
        );

    if (!frameQuery.valid)
        return result;

    result.valid = true;
    result.frame = frameQuery.frame;
    result.arcLength = resolvedArcLength;

    return result;
}