#pragma once

#include <vector>

#include "Core/Forming/StretchHelixWrappingInput.h"
#include "Core/Forming/StretchHelixWrappingKinematics.h"
#include "Core/Forming/StretchHelixWrappingState.h"

#include "Core/Geometry/Frame.h"
#include "Core/Geometry/CurvatureTorsionProfile.h"
#include "Core/Geometry/SpatialCurveIntegrationResult.h"

#include "Core/Geometry/PipeNode.h"
#include "Core/Forming/StretchBendingEvaluationResult.h"

#include "Core/Forming/StretchBendingManufacturingStage.h"




class StretchHelixFormingProcess
{
public:
    StretchHelixFormingProcess() = default;

    // =====================================================
    // CONFIGURATION
    // =====================================================

    bool initialize(
        const StretchHelixWrappingInput& newInput,
        const Frame& newStartFrame,
        const Frame& newSupportAxisFrame
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

    bool setRotationSpeed(
        double rotationSpeed
    );

    bool setAxialSpeed(
        double axialSpeed
    );
    bool isMechanicallyFeasible() const;
    // getters
    const StretchBendingEvaluationResult&
        getStretchEvaluation() const;

    double getTargetFinalCurvature() const;

    double getLoadedCurvature() const;

    double getPredictedFinalCurvature() const;

    const SpatialCurveIntegrationResult&
        getLoadedReferenceResult() const;
    const SpatialCurveIntegrationResult&
        getFinalResult() const;

    StretchBendingManufacturingStage
        getStage() const;

    double getUnloadingFraction() const;
private:

    struct ActiveZoneBoundaryFrames
    {
        Frame entry;
        Frame exit;

        bool valid =
            false;
    };

    bool resolveActiveZoneBoundaryFrames(
        ActiveZoneBoundaryFrames& boundaries
    ) const;


    bool rebuildKinematics();

    bool rebuildReferenceGeometry();

    bool rebuildCurrentGeometry();

    bool rebuildStretchEvaluation();

    bool mechanicsValid =
        false;
private:
    StretchHelixWrappingInput
        input;

    StretchHelixWrappingKinematics
        kinematics;

    StretchHelixWrappingState
        state;

    Frame
        activeFormingFrame;

    Frame startFrame;

    SpatialCurveIntegrationResult
        referenceResult;

    std::vector<PipeNode>
        currentNodes;

    bool valid =
        false;

    StretchBendingEvaluationResult
        stretchEvaluation;

    double targetFinalCurvature =
        0.0;

    double loadedCurvature =
        0.0;

    double predictedFinalCurvature =
        0.0;

    SpatialCurveIntegrationResult
        loadedReferenceResult;

   double activeZoneLength =
        20.0;
    bool rebuildLoadedReferenceGeometry();

    SpatialCurveIntegrationResult
        finalResult;

    bool rebuildFinalGeometry();

    bool appendIncomingGeometry(
        std::vector<PipeNode>& nodes
    ) const;

    bool appendActiveZoneGeometry(
        std::vector<PipeNode>& nodes
    ) const;

    bool appendFormedGeometry(
        std::vector<PipeNode>& nodes
    ) const;

    StretchBendingManufacturingStage stage =
        StretchBendingManufacturingStage::Ready;

    double unloadingFraction =
        0.0;

    double unloadingElapsedTime =
        0.0;

    double unloadingDuration =
        1.0;

    void advanceWrapping(
        double dt
    );

    void advanceUnloading(
        double dt
    );

    bool rebuildUnloadingGeometry();
    Frame supportAxisFrame;

    std::vector<PipeNode>
        formedHistoryNodes;

    double previousWrappedLength =
        0.0;

    double previousSupportRotationAngle =
        0.0;

    bool formedHistoryInitialized =
        false;

    bool updateFormedHistory();

    bool appendFormedHistory(
        std::vector<PipeNode>& nodes
    ) const;

    double previousSupportAxialPosition =
        0.0;
    std::size_t previousFormedReferenceIndex =
        0;


    double finalHelixCurvature =
        0.0;

    double finalHelixTorsion =
        0.0;

    double finalHelixRadius =
        0.0;

    double finalHelixRisePerRadian =
        0.0;

    double finalHelixPitch =
        0.0;


    double loadedHelixCurvature =
        0.0;

    double loadedHelixTorsion =
        0.0;

    double loadedHelixRadius =
        0.0;

    double loadedHelixRisePerRadian =
        0.0;

    double loadedHelixPitch =
        0.0;
};


