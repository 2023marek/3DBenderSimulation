#include "Core/Forming/StretchHelixFormingProcess.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include "Core/Forming/StretchHelixWrappingKinematicsBuilder.h"
#include "Core/Forming/StretchHelixWrappingStateBuilder.h"
#include "Core/Forming/StretchHelixWrappingStateAdvancer.h"

#include "Core/Geometry/ConstantCurvatureTorsionProfileBuilder.h"
#include "Core/Geometry/SpatialCurveIntegrator.h"
#include "Core/Forming/StretchBendingProcessInput.h"
#include "Core/Forming/StretchBendingEvaluator.h"

bool StretchHelixFormingProcess::initialize(
    const StretchHelixWrappingInput& newInput,
    const Frame& newStartFrame)
{
    valid =
        false;

    mechanicsValid =
        false;

    input =
        newInput;

    startFrame =
        newStartFrame;
    activeFormingFrame =
        newStartFrame;
    referenceResult.clear();
    loadedReferenceResult.clear();
    finalResult.clear();
    currentNodes.clear();

    if (!input.isValid())
        return false;

    // =====================================================
    // H2 — KINEMATICS
    // =====================================================

    if (!rebuildKinematics())
    {
        std::cout
            << "[STRETCH HELIX PROCESS INIT FAIL]"
            << " reason=Kinematics"
            << std::endl;

        return false;
    }

    // =====================================================
    // H8/H9 — MECHANICS
    //
    // Mechanical rejection must NOT destroy the geometric
    // preview. Tight debug geometry is intentionally allowed
    // to remain visible.
    // =====================================================

    mechanicsValid =
        rebuildStretchEvaluation();

    // =====================================================
    // TARGET REFERENCE GEOMETRY
    //
    // Always build if the geometry itself is valid.
    // =====================================================

    if (!rebuildReferenceGeometry())
    {
        std::cout
            << "[STRETCH HELIX PROCESS INIT FAIL]"
            << " reason=ReferenceGeometry"
            << std::endl;

        return false;
    }

    // =====================================================
    // H9 — LOADED / FINAL PHYSICAL REFERENCES
    //
    // Build only when the mechanical evaluation succeeded.
    // =====================================================

    if (mechanicsValid)
    {
        if (!rebuildLoadedReferenceGeometry())
        {
            std::cout
                << "[STRETCH HELIX PROCESS INIT FAIL]"
                << " reason=LoadedGeometry"
                << std::endl;

            return false;
        }

        if (!rebuildFinalGeometry())
        {
            std::cout
                << "[STRETCH HELIX PROCESS INIT FAIL]"
                << " reason=FinalGeometry"
                << std::endl;

            return false;
        }



    }

    // =====================================================
    // WRAPPING STATE
    // =====================================================

    state =
        StretchHelixWrappingStateBuilder::buildInitial(
            input
        );

    if (!state.isValidForLength(
        input.pipeArcLength
    ))
    {
        return false;
    }

    // =====================================================
    // CURRENT GEOMETRY
    // =====================================================

    if (!rebuildCurrentGeometry())
    {
        std::cout
            << "[STRETCH HELIX PROCESS INIT FAIL]"
            << " reason=CurrentGeometry"
            << std::endl;

        return false;
    }

    valid =
        true;

    return true;
}

bool StretchHelixFormingProcess::
rebuildKinematics()
{
    kinematics =
        StretchHelixWrappingKinematicsBuilder::build(
            input
        );

    return
        kinematics.valid;
}

bool StretchHelixFormingProcess::
rebuildReferenceGeometry()
{
    referenceResult.clear();

    if (!input.isValid())
        return false;

    if (!kinematics.valid)
        return false;

    const CurvatureTorsionProfile profile =
        ConstantCurvatureTorsionProfileBuilder::build(
            input.pipeArcLength,
            kinematics.curvature,
            kinematics.torsion
        );

    if (!profile.valid)
        return false;

    SpatialCurveIntegrator integrator;

    referenceResult =
        integrator.integrate(
            startFrame,
            profile,
            input.sampleStep
        );

    return
        referenceResult.valid
        && referenceResult.isComplete()
        && referenceResult.nodes.size() >= 2;
}

bool StretchHelixFormingProcess::
rebuildCurrentGeometry()
{
    currentNodes.clear();

    if (!input.isValid())
        return false;

    if (!state.isValidForLength(
        input.pipeArcLength
    ))
    {
        return false;
    }

    if (!appendIncomingGeometry(
        currentNodes
    ))
    {
        return false;
    }

    if (!appendActiveZoneGeometry(
        currentNodes
    ))
    {
        return false;
    }

    if (!appendFormedGeometry(
        currentNodes
    ))
    {
        return false;
    }

    // Empty geometry is allowed only when all manufacturing
    // zones have momentarily collapsed, but normally we
    // should now have either incoming or formed geometry.
    if (state.wrappedLength <= 1e-12)
    {
        return
            currentNodes.size() >= 2;
    }

    const double totalFormedLength =
        state.wrappedLength;

    const double activeLength =
        std::min(
            activeZoneLength,
            totalFormedLength
        );

    const double frozenLength =
        std::max(
            0.0,
            totalFormedLength
            - activeLength
        );

    const double incomingLength =
        std::max(
            0.0,
            input.pipeArcLength
            - totalFormedLength
        );

    std::cout
        << "[MH1 ZONES]"
        << " incoming="
        << incomingLength
        << " active="
        << activeLength
        << " frozen="
        << frozenLength
        << " sum="
        << (
            incomingLength
            + activeLength
            + frozenLength
            )
        << " total="
        << input.pipeArcLength
        << " nodes="
        << currentNodes.size()
        << std::endl;

    return
        !currentNodes.empty();
}

void StretchHelixFormingProcess::
advanceTime(
    double dt)
{
    if (!valid)
        return;

    if (!std::isfinite(dt)
        || dt <= 0.0)
    {
        return;
    }

    switch (stage)
    {
    case StretchBendingManufacturingStage::Ready:
    {
        stage =
            StretchBendingManufacturingStage::Forming;

        advanceWrapping(
            dt
        );

        break;
    }

    case StretchBendingManufacturingStage::Forming:
    {
        advanceWrapping(
            dt
        );

        break;
    }

    case StretchBendingManufacturingStage::LoadedHold:
    {
        stage =
            StretchBendingManufacturingStage::Unloading;

        advanceUnloading(
            dt
        );

        break;
    }

    case StretchBendingManufacturingStage::Unloading:
    {
        advanceUnloading(
            dt
        );

        break;
    }

    case StretchBendingManufacturingStage::Complete:
    case StretchBendingManufacturingStage::Invalid:
    case StretchBendingManufacturingStage::ApplyingTension:
    default:
        break;
    }
}
void StretchHelixFormingProcess::reset()
{
    if (!input.isValid()
        || !kinematics.valid)
    {
        valid =
            false;

        return;
    }

    state =
        StretchHelixWrappingStateBuilder::buildInitial(
            input
        );

    if (!state.isValidForLength(
        input.pipeArcLength
    ))
    {
        valid =
            false;

        return;
    }

    if (!rebuildCurrentGeometry())
    {
        valid =
            false;

        return;
    }

    valid =
        true;

    stage =
        StretchBendingManufacturingStage::Ready;

    unloadingElapsedTime =
        0.0;

    unloadingFraction =
        0.0;
}
bool StretchHelixFormingProcess::
isValid() const
{
    return valid;
}


bool StretchHelixFormingProcess::
isComplete() const
{
    return
        valid
        && stage ==
        StretchBendingManufacturingStage::Complete;
}


const StretchHelixWrappingInput&
StretchHelixFormingProcess::
getInput() const
{
    return input;
}


const StretchHelixWrappingKinematics&
StretchHelixFormingProcess::
getKinematics() const
{
    return kinematics;
}


const StretchHelixWrappingState&
StretchHelixFormingProcess::
getState() const
{
    return state;
}


const SpatialCurveIntegrationResult&
StretchHelixFormingProcess::
getReferenceResult() const
{
    return referenceResult;
}


const std::vector<PipeNode>&
StretchHelixFormingProcess::
getCurrentNodes() const
{
    return currentNodes;
}

bool StretchHelixFormingProcess::
setRotationSpeed(
    double rotationSpeed)
{
    StretchHelixWrappingInput updatedInput =
        input;

    updatedInput.rotationSpeed =
        rotationSpeed;

    if (!updatedInput.isValid())
        return false;

    input =
        updatedInput;

    if (!rebuildKinematics())
        return false;

    if (!rebuildReferenceGeometry())
        return false;

    reset();

    return
        valid;
}

//

bool StretchHelixFormingProcess::
setAxialSpeed(
    double axialSpeed)
{
    StretchHelixWrappingInput updatedInput =
        input;

    updatedInput.rotationSpeed =
        axialSpeed;

    if (!updatedInput.isValid())
        return false;

    input =
        updatedInput;

    if (!rebuildKinematics())
        return false;

    if (!rebuildReferenceGeometry())
        return false;

    reset();

    return
        valid;
}


const StretchBendingEvaluationResult&
StretchHelixFormingProcess::
getStretchEvaluation() const
{
    return stretchEvaluation;
}

bool StretchHelixFormingProcess::
rebuildStretchEvaluation()
{
    if (!input.isValid())
        return false;

    if (!kinematics.valid)
        return false;

    StretchBendingProcessInput mechanicalInput;

    // =====================================================
    // PIPE / MATERIAL
    // =====================================================

    mechanicalInput.pipeSection =
        input.pipeSection;

    mechanicalInput.material =
        input.material;

    // =====================================================
    // HELIX GEOMETRY
    //
    // H2 derived these from the machine commands.
    // =====================================================

    mechanicalInput.geometry.targetArcLength =
        input.pipeArcLength;

    mechanicalInput.geometry.targetCurvature =
        kinematics.curvature;

    mechanicalInput.geometry.targetTorsion =
        kinematics.torsion;

    // =====================================================
    // STRETCH COMMAND
    // =====================================================

    mechanicalInput.axialStretchStrain =
        input.axialStretchStrain;

    // =====================================================
    // NUMERICS / PROCESS
    // =====================================================

    mechanicalInput.feedSpeed =
        input.axialSpeed;

    mechanicalInput.sampleStep =
        input.sampleStep;

    // H8 does not yet model springback.
    mechanicalInput.springbackRatio =
        input.springbackRatio;

    mechanicalInput.compensateSpringback =
        input.compensateSpringback; 

    mechanicalInput.enabled =
        true;

    if (!mechanicalInput.isValid())
        return false;

    StretchBendingEvaluator evaluator;

    stretchEvaluation =
        evaluator.evaluate(
            mechanicalInput
        );


    // =================================================
   // H9 — COPY SPRINGBACK RESULT INTO PROCESS STATE
   // =================================================

    targetFinalCurvature =
        stretchEvaluation.finalTargetCurvature;

    loadedCurvature =
        stretchEvaluation.loadedCurvatureCommand;

    predictedFinalCurvature =
        stretchEvaluation.predictedFinalCurvature;

    std::cout
        << "[STRETCH HELIX SPRINGBACK]"
        << " evaluationValid="
        << stretchEvaluation.valid
        << " predictionValid="
        << stretchEvaluation.springbackPredictionValid
        << " compensationApplied="
        << stretchEvaluation.springbackCompensationApplied
        << " targetKappa="
        << targetFinalCurvature
        << " loadedKappa="
        << loadedCurvature
        << " predictedFinalKappa="
        << predictedFinalCurvature
        << " finalError="
        << stretchEvaluation.finalCurvatureError
        << " ratio="
        << stretchEvaluation.springbackRatio
        << std::endl;
    return
        stretchEvaluation.valid;


    std::cout
        << "[STRETCH HELIX MECHANICS]"
        << " valid="
        << stretchEvaluation.valid
        << " status="
        << stretchBendingEvaluationStatusToString(
            stretchEvaluation.status
        )
        << " kappa="
        << kinematics.curvature
        << " torsion="
        << kinematics.torsion
        << " axialStrain="
        << input.axialStretchStrain
        << " bendingStrain="
        << stretchEvaluation.bendingStrain
        << " innerStrain="
        << stretchEvaluation.innerWallStrain
        << " outerStrain="
        << stretchEvaluation.outerWallStrain
        << " tension="
        << stretchEvaluation.commandedTension
        << std::endl;

    
}

bool StretchHelixFormingProcess::
isMechanicallyFeasible() const
{
    return mechanicsValid;
}

double StretchHelixFormingProcess::
getTargetFinalCurvature() const
{
    return targetFinalCurvature;
}

double StretchHelixFormingProcess::
getLoadedCurvature() const
{
    return loadedCurvature;
}

double StretchHelixFormingProcess::
getPredictedFinalCurvature() const
{
    return predictedFinalCurvature;
}

const SpatialCurveIntegrationResult&
StretchHelixFormingProcess::
getLoadedReferenceResult() const
{
    return loadedReferenceResult;
}

bool StretchHelixFormingProcess::
rebuildLoadedReferenceGeometry()
{
    loadedReferenceResult.clear();

    std::cout
        << "[STRETCH HELIX LOADED BUILD INPUT]"
        << " mechanicsValid="
        << mechanicsValid
        << " targetKappa="
        << targetFinalCurvature
        << " loadedKappa="
        << loadedCurvature
        << " predictedFinalKappa="
        << predictedFinalCurvature
        << " torsion="
        << kinematics.torsion
        << " length="
        << input.pipeArcLength
        << " sampleStep="
        << input.sampleStep
        << std::endl;

    if (!input.isValid())
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=InvalidInput"
            << std::endl;

        return false;
    }

    if (!kinematics.valid)
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=InvalidKinematics"
            << std::endl;

        return false;
    }

    if (!std::isfinite(loadedCurvature))
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=LoadedCurvatureNotFinite"
            << std::endl;

        return false;
    }

    if (loadedCurvature <= 0.0)
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=LoadedCurvatureNotPositive"
            << " loadedKappa="
            << loadedCurvature
            << std::endl;

        return false;
    }

    const CurvatureTorsionProfile loadedProfile =
        ConstantCurvatureTorsionProfileBuilder::build(
            input.pipeArcLength,
            loadedCurvature,
            kinematics.torsion
        );

    std::cout
        << "[STRETCH HELIX LOADED PROFILE]"
        << " valid="
        << loadedProfile.valid
        << " samples="
        << loadedProfile.samples.size()
        << " length="
        << loadedProfile.totalArcLength
        << std::endl;

    if (!loadedProfile.valid)
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=InvalidLoadedProfile"
            << std::endl;

        return false;
    }

    SpatialCurveIntegrator integrator;

    loadedReferenceResult =
        integrator.integrate(
            startFrame,
            loadedProfile,
            input.sampleStep
        );

    std::cout
        << "[STRETCH HELIX LOADED RESULT]"
        << " valid="
        << loadedReferenceResult.valid
        << " complete="
        << loadedReferenceResult.isComplete()
        << " nodes="
        << loadedReferenceResult.nodes.size()
        << " length="
        << loadedReferenceResult.integratedArcLength
        << std::endl;

    if (!loadedReferenceResult.valid)
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=IntegrationInvalid"
            << std::endl;

        return false;
    }

    if (!loadedReferenceResult.isComplete())
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=IntegrationIncomplete"
            << std::endl;

        return false;
    }

    if (loadedReferenceResult.nodes.size() < 2)
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=TooFewNodes"
            << std::endl;

        return false;
    }

    return true;
}
const SpatialCurveIntegrationResult&
StretchHelixFormingProcess::
getFinalResult() const
{
    return finalResult;
}

bool StretchHelixFormingProcess::
rebuildFinalGeometry()
{
    finalResult.clear();

    if (!input.isValid())
        return false;

    if (!kinematics.valid)
        return false;

    if (!std::isfinite(
        predictedFinalCurvature
    ))
    {
        return false;
    }

    if (predictedFinalCurvature <= 0.0)
        return false;

    const CurvatureTorsionProfile finalProfile =
        ConstantCurvatureTorsionProfileBuilder::build(
            input.pipeArcLength,
            predictedFinalCurvature,
            kinematics.torsion
        );

    if (!finalProfile.valid)
        return false;

    SpatialCurveIntegrator integrator;

    finalResult =
        integrator.integrate(
            startFrame,
            finalProfile,
            input.sampleStep
        );

    return
        finalResult.valid
        && finalResult.isComplete();
}

StretchBendingManufacturingStage
StretchHelixFormingProcess::
getStage() const
{
    return stage;
}


double StretchHelixFormingProcess::
getUnloadingFraction() const
{
    return unloadingFraction;
}

void StretchHelixFormingProcess::
advanceWrapping(
    double dt)
{
    if (state.complete)
    {
        stage =
            StretchBendingManufacturingStage::LoadedHold;

        unloadingElapsedTime =
            0.0;

        unloadingFraction =
            0.0;
        std::cout
            << "[STRETCH HELIX STAGE]"
            << " stage="
            << stretchBendingManufacturingStageToString(
                stage
            )
            << std::endl;
        return;
    }

    StretchHelixWrappingStateAdvancer::advance(
        state,
        dt,
        input,
        kinematics
    );

    if (!rebuildCurrentGeometry())
    {
        valid =
            false;

        return;
    }

    if (state.complete)
    {
        stage =
            StretchBendingManufacturingStage::LoadedHold;

        unloadingElapsedTime =
            0.0;

        unloadingFraction =
            0.0;

        std::cout
            << "[STRETCH HELIX STAGE]"
            << " stage="
            << stretchBendingManufacturingStageToString(
                stage
            )
            << std::endl;
    }
}

void StretchHelixFormingProcess::
advanceUnloading(
    double dt)
{
    if (!mechanicsValid)
    {
        stage =
            StretchBendingManufacturingStage::Complete;

        return;
    }

    if (!std::isfinite(unloadingDuration)
        || unloadingDuration <= 0.0)
    {
        stage =
            StretchBendingManufacturingStage::Complete;

        return;
    }

    unloadingElapsedTime +=
        dt;

    unloadingFraction =
        std::clamp(
            unloadingElapsedTime
            / unloadingDuration,
            0.0,
            1.0
        );

    if (!rebuildUnloadingGeometry())
    {
        valid =
            false;

        return;
    }

    if (unloadingFraction >= 1.0)
    {
        unloadingFraction =
            1.0;

        stage =
            StretchBendingManufacturingStage::Complete;

        std::cout
            << "[STRETCH HELIX STAGE]"
            << " stage="
            << stretchBendingManufacturingStageToString(
                stage
            )
            << std::endl;
    }
}

bool StretchHelixFormingProcess::
rebuildUnloadingGeometry()
{
    currentNodes.clear();

    if (!mechanicsValid)
        return false;

    const double fraction =
        std::clamp(
            unloadingFraction,
            0.0,
            1.0
        );

    const double currentCurvature =
        loadedCurvature
        + fraction
        * (
            predictedFinalCurvature
            - loadedCurvature
            );

    if (!std::isfinite(currentCurvature)
        || currentCurvature <= 0.0)
    {
        return false;
    }

    const CurvatureTorsionProfile profile =
        ConstantCurvatureTorsionProfileBuilder::build(
            input.pipeArcLength,
            currentCurvature,
            kinematics.torsion
        );

    if (!profile.valid)
        return false;

    SpatialCurveIntegrator integrator;

    const SpatialCurveIntegrationResult result =
        integrator.integrate(
            startFrame,
            profile,
            input.sampleStep
        );

    if (!result.valid
        || !result.isComplete()
        || result.nodes.size() < 2)
    {
        return false;
    }

    currentNodes =
        result.nodes;

    std::cout
        << "[STRETCH HELIX UNLOADING]"
        << " fraction="
        << fraction
        << " currentKappa="
        << currentCurvature
        << " loadedKappa="
        << loadedCurvature
        << " finalKappa="
        << predictedFinalCurvature
        << " nodes="
        << currentNodes.size()
        << std::endl;

    return true;
}

bool StretchHelixFormingProcess::
appendActiveZoneGeometry(
    std::vector<PipeNode>& nodes) const
{
    if (!std::isfinite(activeZoneLength)
        || activeZoneLength <= 0.0)
    {
        return true;
    }

    if (!std::isfinite(input.sampleStep)
        || input.sampleStep <= 0.0)
    {
        return false;
    }

    const double formedLength =
        std::max(
            0.0,
            state.wrappedLength
        );

    const double activeLength =
        std::min(
            activeZoneLength,
            formedLength
        );

    if (activeLength <= 1e-12)
        return true;
//

    const std::size_t segmentCount =
        std::max<std::size_t>(
            1,
            static_cast<std::size_t>(
                std::ceil(
                    activeLength
                    / input.sampleStep
                )
                )
        );

    for (std::size_t i = 1;
        i <= segmentCount;
        ++i)
    {
        const double fraction =
            static_cast<double>(i)
            / static_cast<double>(
                segmentCount
                );

        PipeNode node;

        node.pos =
            activeFormingFrame.P
            + activeFormingFrame.T
            * (
                activeLength
                * fraction
                );

        nodes.push_back(
            node
        );
    }

    return true;
}


bool StretchHelixFormingProcess::
appendFormedGeometry(
    std::vector<PipeNode>& nodes) const
{
    const SpatialCurveIntegrationResult* formingReference =
        &referenceResult;

    if (
        mechanicsValid
        && loadedReferenceResult.valid
        && loadedReferenceResult.isComplete()
        )
    {
        formingReference =
            &loadedReferenceResult;
    }

    if (!formingReference->valid
        || !formingReference->isComplete())
    {
        return false;
    }

    const std::vector<PipeNode>& referenceNodes =
        formingReference->nodes;

    if (referenceNodes.size() < 2)
        return false;

    const double totalFormedLength =
        std::clamp(
            state.wrappedLength,
            0.0,
            input.pipeArcLength
        );

    const double activeLength =
        std::min(
            activeZoneLength,
            totalFormedLength
        );

    const double frozenLength =
        std::max(
            0.0,
            totalFormedLength
            - activeLength
        );

    if (frozenLength <= 1e-12)
        return true;

    const double normalizedLength =
        frozenLength
        / input.pipeArcLength;

    const std::size_t lastIndex =
        referenceNodes.size() - 1;

    const std::size_t formedLastIndex =
        std::min(
            lastIndex,
            static_cast<std::size_t>(
                std::llround(
                    normalizedLength
                    * static_cast<double>(
                        lastIndex
                        )
                )
                )
        );

    std::cout
        << "[MH1 FORMED CHECK]"
        << " totalFormed="
        << totalFormedLength
        << " active="
        << activeLength
        << " frozen="
        << frozenLength
        << " referenceNodes="
        << referenceNodes.size()
        << " formedLastIndex="
        << formedLastIndex
        << std::endl;

    if (formedLastIndex < 1)
        return true;

    const Vec3D activeExit =
        activeFormingFrame.P
        + activeFormingFrame.T
        * activeLength;

    const Vec3D referenceOrigin =
        referenceNodes.front().pos;

    for (std::size_t i = 1;
        i <= formedLastIndex;
        ++i)
    {
        PipeNode node =
            referenceNodes[i];

        const Vec3D localPosition =
            referenceNodes[i].pos
            - referenceOrigin;

        node.pos =
            activeExit
            + localPosition;

        nodes.push_back(
            node
        );
    }

    std::cout
        << "[MH1 FORMED APPEND]"
        << " appended="
        << formedLastIndex
        << " totalCurrentNodes="
        << nodes.size()
        << std::endl;

    return true;
}


bool StretchHelixFormingProcess::
appendIncomingGeometry(
    std::vector<PipeNode>& nodes) const
{
    const double incomingLength =
        std::max(
            0.0,
            input.pipeArcLength
            - state.wrappedLength
        );

    if (incomingLength <= 0.0)
    {
        return true;
    }

    if (!std::isfinite(input.sampleStep)
        || input.sampleStep <= 0.0)
    {
        return false;
    }

    const std::size_t segmentCount =
        std::max<std::size_t>(
            1,
            static_cast<std::size_t>(
                std::ceil(
                    incomingLength
                    / input.sampleStep
                )
                )
        );

    nodes.reserve(
        nodes.size()
        + segmentCount
        + 1
    );

    // Start at the far/free end of the incoming stock
    // and finish exactly at the fixed active point.
    for (std::size_t i = 0;
        i <= segmentCount;
        ++i)
    {
        const double fraction =
            static_cast<double>(i)
            / static_cast<double>(
                segmentCount
                );

        const double distanceFromActive =
            incomingLength
            * (
                1.0
                - fraction
                );

        PipeNode node;

        node.pos =
            activeFormingFrame.P
            - activeFormingFrame.T
            * distanceFromActive;

        nodes.push_back(
            node
        );
    }

    std::cout
        << "[MH1 INCOMING CHECK]"
        << " wrappedLength="
        << state.wrappedLength
        << " incomingLength="
        << incomingLength
        << " activeP=("
        << activeFormingFrame.P.x
        << ", "
        << activeFormingFrame.P.y
        << ", "
        << activeFormingFrame.P.z
        << ")";

    if (!nodes.empty())
    {
        const Vec3D delta =
            nodes.back().pos
            - nodes.front().pos;

        std::cout
            << " geometricLength="
            << delta.length();
    }

    std::cout
        << std::endl;


    return true;
}