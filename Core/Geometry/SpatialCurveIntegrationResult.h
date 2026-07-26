#pragma once

#include <cstddef>
#include <vector>

#include "Core/Geometry/Frame.h"
#include "Core/Geometry/PipeNode.h"
#include "Core/Geometry/SpatialCurveIntegrationStatus.h"

struct SpatialCurveIntegrationResult
{
    std::vector<PipeNode> nodes;
    std::vector<double> arcLengths;

    Frame startFrame;
    Frame endFrame;

    double requestedArcLength =
        0.0;

    double integratedArcLength =
        0.0;

    double sampleStep =
        0.0;

    size_t requestedStepCount =
        0;

    size_t completedStepCount =
        0;

    SpatialCurveIntegrationStatus status =
        SpatialCurveIntegrationStatus::NotStarted;

    bool valid =
        false;

    void clear()
    {
        nodes.clear();
        arcLengths.clear();

        startFrame =
            Frame{};

        endFrame =
            Frame{};

        requestedArcLength =
            0.0;

        integratedArcLength =
            0.0;

        sampleStep =
            0.0;

        requestedStepCount =
            0;

        completedStepCount =
            0;

        status =
            SpatialCurveIntegrationStatus::NotStarted;

        valid =
            false;
    }

    bool hasConsistentNodeData() const
    {
        return nodes.size() >= 2
            && arcLengths.size()
            == nodes.size();
    }

    bool isComplete(
        double tolerance = 1e-9
    ) const
    {
        return valid
            && status
            == SpatialCurveIntegrationStatus::Completed
            && hasConsistentNodeData()
            && integratedArcLength
            >= requestedArcLength - tolerance;
    }
};