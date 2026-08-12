#pragma once

#include <vector>

#include "Core/Forming/StretchHelixWrappingInput.h"
#include "Core/Forming/StretchHelixWrappingKinematics.h"
#include "Core/Forming/StretchHelixWrappingState.h"

#include "Core/Geometry/Frame.h"
#include "Core/Geometry/CurvatureTorsionProfile.h"
#include "Core/Geometry/SpatialCurveIntegrationResult.h"

#include "Core/Geometry/PipeNode.h"

class StretchHelixFormingProcess
{
public:
    StretchHelixFormingProcess() = default;

    // =====================================================
    // CONFIGURATION
    // =====================================================

    bool initialize(
        const StretchHelixWrappingInput& input,
        const Frame& startFrame
    );

    void reset();

    void advanceTime(
        double dt
    );

    // =====================================================
    // STATE
    // =====================================================

    bool isValid() const;

    bool isComplete() const;

    // =====================================================
    // READ-ONLY ACCESS
    // =====================================================

    const StretchHelixWrappingInput&
        getInput() const;

    const StretchHelixWrappingKinematics&
        getKinematics() const;

    const StretchHelixWrappingState&
        getState() const;

    const SpatialCurveIntegrationResult&
        getReferenceResult() const;

    const std::vector<PipeNode>&
        getCurrentNodes() const;

private:
    bool rebuildKinematics();

    bool rebuildReferenceGeometry();

    bool rebuildCurrentGeometry();

private:
    StretchHelixWrappingInput
        input;

    StretchHelixWrappingKinematics
        kinematics;

    StretchHelixWrappingState
        state;

    Frame
        startFrame;

    SpatialCurveIntegrationResult
        referenceResult;

    std::vector<PipeNode>
        currentNodes;

    bool valid =
        false;
};


