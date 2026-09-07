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

        // Temporary legacy argument.
    // MH1.19 derives the actual required support axis
    // from loaded helix compensation.
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

 //
    double getRequiredSupportOuterRadius() const;

    const Frame&
        getRequiredSupportAxisFrame() const;

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

    Frame finalHelixStartFrame;
    Frame loadedHelixStartFrame;

    SpatialCurveIntegrationResult
        referenceResult;

    std::vector<PipeNode>
        currentNodes;


    // ============================================================
// MH1.20.10
//
// Temporary diagnostic snapshot of the LAST geometry produced
// while the process is still in Wrapping.
//
// This allows us to compare:
//
//     final Wrapping geometry
//
// against:
//
//     first LoadedHold geometry
//
// without relying on visual inspection.
// ============================================================

    std::vector<PipeNode> mh12010LastWrappingNodes;

    bool mh12010WrappingSnapshotValid = false;

    bool valid =
        false;
    // ------------------------------------------------------------
    // MH1.20.10C.18
    //
    // Track the exact formed-history front from the end of one
    // update into the beginning of the next update.
    //
    // Diagnostic only.
    // ------------------------------------------------------------

    std::size_t mh12010C19FrontLocalSourceIndex = 0;

    double mh12010C19FrontRepresentedLocalLength = 0.0;

    double mh12010C19FrontActualDeltaLength = 0.0;

    bool mh12010C19FrontOriginValid = false;


    Vec3D mh12010C18PreviousHistoryFrontPosition;

    bool mh12010C18PreviousHistoryFrontValid = false;
    StretchBendingEvaluationResult
        stretchEvaluation;

    double mh12010C21EStoredFrontRadiusError = 0.0;
    bool mh12010C21EStoredFrontRadiusValid = false;

    std::vector<PipeNode> mh12010C21HRigidHistoryNodes;

    bool mh12010C21HRigidHistoryValid = false;



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

    //
    double requiredSupportOuterRadius =
        0.0;

    Frame requiredSupportAxisFrame;

    bool rebuildRequiredSupportGeometry();
};


