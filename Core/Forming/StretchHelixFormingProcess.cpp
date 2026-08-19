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
#include "Core/Geometry/RigidTransformUtils.h"

namespace
{

    Vec3D evaluateHermitePosition(
        const Vec3D& p0,
        const Vec3D& p1,
        const Vec3D& m0,
        const Vec3D& m1,
        double u)
    {
        const double u2 =
            u * u;

        const double u3 =
            u2 * u;

        const double h00 =
            2.0 * u3
            - 3.0 * u2
            + 1.0;

        const double h10 =
            u3
            - 2.0 * u2
            + u;

        const double h01 =
            -2.0 * u3
            + 3.0 * u2;

        const double h11 =
            u3
            - u2;

        return
            p0 * h00
            + m0 * h10
            + p1 * h01
            + m1 * h11;
    }

    Vec3D evaluateHermiteTangent(
        const Vec3D& p0,
        const Vec3D& p1,
        const Vec3D& m0,
        const Vec3D& m1,
        double u)
    {
        const double u2 =
            u * u;

        const double dh00 =
            6.0 * u2
            - 6.0 * u;

        const double dh10 =
            3.0 * u2
            - 4.0 * u
            + 1.0;

        const double dh01 =
            -6.0 * u2
            + 6.0 * u;

        const double dh11 =
            3.0 * u2
            - 2.0 * u;

        return
            p0 * dh00
            + m0 * dh10
            + p1 * dh01
            + m1 * dh11;
    }

    double measureNodeArcLength(
        const std::vector<PipeNode>& nodes)
    {
        if (nodes.size() < 2)
            return 0.0;

        double length = 0.0;

        for (std::size_t i = 1;
            i < nodes.size();
            ++i)
        {
            const Vec3D delta =
                nodes[i].pos
                - nodes[i - 1].pos;

            length +=
                delta.length();
        }

        return length;
    }

}

bool StretchHelixFormingProcess::initialize(
    const StretchHelixWrappingInput& newInput,
    const Frame& newStartFrame,
    const Frame& newSupportAxisFrame)
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

    supportAxisFrame =
        newSupportAxisFrame;

    referenceResult.clear();
    loadedReferenceResult.clear();
    finalResult.clear();
    currentNodes.clear();

    if (!input.isValid())
        return false;

    if (supportAxisFrame.T.lengthSquared() < 1e-12)
    {
        std::cout
            << "[STRETCH HELIX PROCESS INIT FAIL]"
            << " reason=InvalidSupportAxis"
            << std::endl;

        return false;
    }

    supportAxisFrame.T =
        supportAxisFrame.T.normalized();

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
    // =====================================================

    mechanicsValid =
        rebuildStretchEvaluation();

    // =====================================================
    // TARGET REFERENCE GEOMETRY
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
    // LOADED / FINAL REFERENCES
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
        std::cout
            << "[STRETCH HELIX PROCESS INIT FAIL]"
            << " reason=InvalidWrappingState"
            << std::endl;

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

    // =====================================================
    // UPDATE PERSISTENT FORMED MATERIAL
    // =====================================================

    if (!updateFormedHistory())
    {
        return false;
    }

    // =====================================================
    // BUILD CURRENT DISPLAY GEOMETRY
    // =====================================================

    if (!appendIncomingGeometry(
        currentNodes
    ))
    {
        return false;
    }

    if (!appendFormedHistory(
        currentNodes
    ))
    {
        return false;
    }

    const double formedLength =
        std::clamp(
            state.wrappedLength,
            0.0,
            input.pipeArcLength
        );

    const double incomingLength =
        std::max(
            0.0,
            input.pipeArcLength
            - formedLength
        );

    std::cout
        << "[MH1.17 HISTORY]"
        << " incoming="
        << incomingLength
        << " formed="
        << formedLength
        << " historyNodes="
        << formedHistoryNodes.size()
        << " currentNodes="
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

    formedHistoryNodes.clear();

    previousWrappedLength =
        0.0;

    previousSupportRotationAngle =
        0.0;

    formedHistoryInitialized =
        false;

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

    if (!std::isfinite(input.sampleStep)
        || input.sampleStep <= 0.0)
    {
        return false;
    }

    ActiveZoneBoundaryFrames boundaries;

    if (!resolveActiveZoneBoundaryFrames(
        boundaries
    ))
    {
        return false;
    }

    if (!boundaries.valid)
        return false;

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

    const Vec3D p0 =
        boundaries.entry.P;

    const Vec3D p1 =
        boundaries.exit.P;

    const Vec3D m0 =
        boundaries.entry.T
        * activeLength;

    const Vec3D m1 =
        boundaries.exit.T
        * activeLength;
    const Vec3D startTangent =
        evaluateHermiteTangent(
            p0,
            p1,
            m0,
            m1,
            0.0
        ).normalized();

    const Vec3D endTangent =
        evaluateHermiteTangent(
            p0,
            p1,
            m0,
            m1,
            1.0
        ).normalized();

    const double startAlignment =
        dot(
            startTangent,
            boundaries.entry.T.normalized()
        );

    const double endAlignment =
        dot(
            endTangent,
            boundaries.exit.T.normalized()
        );

    std::cout
        << "[MH1 ACTIVE ACCEPTANCE]"
        << " startAlignment="
        << startAlignment
        << " endAlignment="
        << endAlignment
        << std::endl;


    for (std::size_t i = 1;
        i <= segmentCount;
        ++i)
    {
        const double u =
            static_cast<double>(i)
            / static_cast<double>(
                segmentCount
                );

        PipeNode node;

        node.pos =
            evaluateHermitePosition(
                p0,
                p1,
                m0,
                m1,
                u
            );

        node.T =
            evaluateHermiteTangent(
                p0,
                p1,
                m0,
                m1,
                u
            ).normalized();

        Vec3D interpolatedN =
            boundaries.entry.N
            * (1.0 - u)
            + boundaries.exit.N
            * u;

        interpolatedN =
            interpolatedN
            - node.T
            * dot(
                interpolatedN,
                node.T
            );

        if (interpolatedN.lengthSquared() < 1e-12)
        {
            interpolatedN =
                boundaries.entry.N;
        }

        node.N =
            interpolatedN.normalized();

        node.B =
            cross(
                node.T,
                node.N
            ).normalized();

        node.N =
            cross(
                node.B,
                node.T
            ).normalized();

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

    // =====================================================
    // WORKSHOP MODEL — FORMED LENGTH
    // =====================================================

    const double formedLength =
        std::clamp(
            state.wrappedLength,
            0.0,
            input.pipeArcLength
        );

    if (formedLength <= 1e-12)
        return true;

    const double normalizedLength =
        formedLength
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
        << " formedLength="
        << formedLength
        << " referenceNodes="
        << referenceNodes.size()
        << " formedLastIndex="
        << formedLastIndex
        << std::endl;

    if (formedLastIndex < 1)
        return true;

    // =====================================================
    // FIXED FORMING POINT
    // =====================================================

    const Vec3D formingPoint =
        activeFormingFrame.P;

    const Vec3D referenceOrigin =
        referenceNodes.front().pos;

    // =====================================================
    // PHYSICAL SUPPORT AXIS FROM MACHINE MODEL
    // =====================================================

    const Vec3D supportAxisPoint =
        supportAxisFrame.P;

    Vec3D supportAxisDirection =
        supportAxisFrame.T;

    if (supportAxisDirection.lengthSquared() < 1e-12)
        return false;

    supportAxisDirection =
        supportAxisDirection.normalized();

    const double supportAngle =
        state.supportRotationAngle;

    std::cout
        << "[MH1 SUPPORT MOTION]"
        << " angle="
        << supportAngle
        << " axialPosition="
        << state.supportAxialPosition
        << " formingPoint=("
        << formingPoint.x
        << ", "
        << formingPoint.y
        << ", "
        << formingPoint.z
        << ")"
        << " axisPoint=("
        << supportAxisPoint.x
        << ", "
        << supportAxisPoint.y
        << ", "
        << supportAxisPoint.z
        << ")"
        << " axisDir=("
        << supportAxisDirection.x
        << ", "
        << supportAxisDirection.y
        << ", "
        << supportAxisDirection.z
        << ")"
        << std::endl;

    // =====================================================
    // APPEND FORMED GEOMETRY
    // =====================================================

    for (std::size_t i = 1;
        i <= formedLastIndex;
        ++i)
    {
        PipeNode node =
            referenceNodes[i];

        // First anchor the reference helix at the fixed
        // forming point.
        node.pos =
            formingPoint
            + (
                referenceNodes[i].pos
                - referenceOrigin
                );

        // Then rotate only the formed material around
        // the REAL support-tube axis.
        RigidTransformUtils::
            rotateNodeAroundAxis(
                node,
                supportAxisPoint,
                supportAxisDirection,
                supportAngle
            );

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


bool StretchHelixFormingProcess::
resolveActiveZoneBoundaryFrames(
    ActiveZoneBoundaryFrames& boundaries) const
{
    boundaries =
        ActiveZoneBoundaryFrames{};

    boundaries.entry =
        activeFormingFrame;

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

    if (activeLength <= 1e-12)
    {
        boundaries.exit =
            boundaries.entry;

        boundaries.valid =
            true;

        return true;
    }

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
        || !formingReference->isComplete()
        || formingReference->nodes.size() < 2)
    {
        return false;
    }

    boundaries.exit =
        boundaries.entry;

    boundaries.exit.P =
        activeFormingFrame.P
        + activeFormingFrame.T
        * activeLength;

    const PipeNode& formedReferenceNode =
        formingReference->nodes[1];

    boundaries.exit.T =
        formedReferenceNode.T;

    boundaries.exit.N =
        formedReferenceNode.N;

    boundaries.exit.B =
        formedReferenceNode.B;

    Vec3D supportAxisDirection =
        activeFormingFrame.T
        * kinematics.torsion
        + activeFormingFrame.B
        * kinematics.curvature;

    if (supportAxisDirection.lengthSquared() < 1e-12)
        return false;

    supportAxisDirection =
        supportAxisDirection.normalized();

    boundaries.exit.T =
        RigidTransformUtils::rotateAroundAxis(
            boundaries.exit.T,
            supportAxisDirection,
            state.supportRotationAngle
        ).normalized();

    boundaries.exit.N =
        RigidTransformUtils::rotateAroundAxis(
            boundaries.exit.N,
            supportAxisDirection,
            state.supportRotationAngle
        ).normalized();

    boundaries.exit.B =
        RigidTransformUtils::rotateAroundAxis(
            boundaries.exit.B,
            supportAxisDirection,
            state.supportRotationAngle
        ).normalized();
    const double tangentAlignment =
        dot(
            boundaries.entry.T.normalized(),
            boundaries.exit.T.normalized()
        );

    std::cout
        << "[MH1 ACTIVE TRANSITION]"
        << " activeLength="
        << activeLength
        << " rotationAngle="
        << state.supportRotationAngle
        << " entryExitDot="
        << tangentAlignment
        << " entryT=("
        << boundaries.entry.T.x
        << ", "
        << boundaries.entry.T.y
        << ", "
        << boundaries.entry.T.z
        << ")"
        << " exitT=("
        << boundaries.exit.T.x
        << ", "
        << boundaries.exit.T.y
        << ", "
        << boundaries.exit.T.z
        << ")"
        << std::endl;
    boundaries.valid =
        true;

    return true;
}

bool StretchHelixFormingProcess::
updateFormedHistory()
{

    // =====================================================
        // MH1.17 — FORMED HISTORY IS UPDATED ONLY WHILE
        // MATERIAL IS ACTUALLY ENTERING THE FORMING PROCESS.
        //
        // During LoadedHold / Unloading / Complete,
        // no new material is added to the history.
        // H10 owns the full-pipe unloading geometry.
        // =====================================================

    if (
        stage != StretchBendingManufacturingStage::Ready
        &&
        stage != StretchBendingManufacturingStage::Forming
        )
    {
        return true;
    }



    if (!formedHistoryInitialized)
    {
        formedHistoryNodes.clear();

        previousWrappedLength =
            state.wrappedLength;

        previousSupportRotationAngle =
            state.supportRotationAngle;

        formedHistoryInitialized =
            true;

        return true;
    }

    const double currentWrappedLength =
        std::clamp(
            state.wrappedLength,
            0.0,
            input.pipeArcLength
        );

    const double deltaLength =
        currentWrappedLength
        - previousWrappedLength;

    const double deltaAngle =
        state.supportRotationAngle
        - previousSupportRotationAngle;

    if (!std::isfinite(deltaLength)
        || !std::isfinite(deltaAngle))
    {
        return false;
    }

    if (deltaLength < -1e-12)
    {
        // History should only grow during forming.
        return false;
    }

    const Vec3D supportAxisPoint =
        supportAxisFrame.P;

    Vec3D supportAxisDirection =
        supportAxisFrame.T;

    if (supportAxisDirection.lengthSquared() < 1e-12)
        return false;

    supportAxisDirection =
        supportAxisDirection.normalized();
    if (std::abs(deltaAngle) > 1e-12)
    {
        for (PipeNode& node :
            formedHistoryNodes)
        {
            RigidTransformUtils::
                rotateNodeAroundAxis(
                    node,
                    supportAxisPoint,
                    supportAxisDirection,
                    deltaAngle
                );
        }
    }
    if (deltaLength <= 1e-12)
    {
        previousWrappedLength =
            currentWrappedLength;

        previousSupportRotationAngle =
            state.supportRotationAngle;

        return true;
    }

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
        || !formingReference->isComplete()
        || formingReference->nodes.size() < 2)
    {
        return false;
    }

    const std::size_t incrementSegments =
        std::max<std::size_t>(
            1,
            static_cast<std::size_t>(
                std::llround(
                    deltaLength
                    / input.sampleStep
                )
                )
        );

    const std::size_t maxSegments =
        formingReference->nodes.size() - 1;

    const std::size_t segmentCount =
        std::min(
            incrementSegments,
            maxSegments
        );

    const std::vector<PipeNode>& referenceNodes =
        formingReference->nodes;

    const Vec3D referenceOrigin =
        referenceNodes.front().pos;

    const Vec3D formingPoint =
        activeFormingFrame.P;

    std::vector<PipeNode>
        newIncrementNodes;

    newIncrementNodes.reserve(
        segmentCount
    );

    for (std::size_t i = 1;
        i <= segmentCount;
        ++i)
    {
        PipeNode node =
            referenceNodes[i];

        node.pos =
            formingPoint
            + (
                referenceNodes[i].pos
                - referenceOrigin
                );

        newIncrementNodes.push_back(
            node
        );
    }

    // =====================================================
// MH1.17 — JUNCTION DIAGNOSTIC
//
// Check connection:
//
// new increment ----> old formed history
//
// newLast              oldFirst
//    *--------------------*
// =====================================================

    if (!newIncrementNodes.empty()
        && !formedHistoryNodes.empty())
    {
        const PipeNode& newLast =
            newIncrementNodes.back();

        const PipeNode& oldFirst =
            formedHistoryNodes.front();

        //
        const Vec3D junctionCorrection =
            oldFirst.pos
            - newLast.pos;
        const std::size_t count =
            newIncrementNodes.size();

        if (count > 1)
        {
            for (std::size_t i = 0;
                i < count;
                ++i)
            {
                const double fraction =
                    static_cast<double>(i + 1)
                    / static_cast<double>(count);

                newIncrementNodes[i].pos +=
                    junctionCorrection
                    * fraction;
            }
        }




        const Vec3D positionDelta =
            oldFirst.pos
            - newLast.pos;

        const double positionGap =
            positionDelta.length();

        const Vec3D newT =
            newLast.T.normalized();

        const Vec3D oldT =
            oldFirst.T.normalized();

        const double tangentDot =
            dot(
                newT,
                oldT
            );




        std::cout
            << "[MH1.17 JUNCTION]"
            << " positionGap="
            << positionGap
            << " tangentDot="
            << tangentDot
            << " newLastP=("
            << newLast.pos.x << ", "
            << newLast.pos.y << ", "
            << newLast.pos.z << ")"
            << " oldFirstP=("
            << oldFirst.pos.x << ", "
            << oldFirst.pos.y << ", "
            << oldFirst.pos.z << ")"
            << std::endl;
    }

    if (!newIncrementNodes.empty()
        && !formedHistoryNodes.empty())
    {
        const PipeNode& correctedLast =
            newIncrementNodes.back();

        const PipeNode& oldFirst =
            formedHistoryNodes.front();

        const double correctedGap =
            (
                oldFirst.pos
                - correctedLast.pos
                ).length();

        const double correctedTangentDot =
            dot(
                correctedLast.T.normalized(),
                oldFirst.T.normalized()
            );

        std::cout
            << "[MH1.17 JUNCTION CORRECTED]"
            << " positionGap="
            << correctedGap
            << " tangentDot="
            << correctedTangentDot
            << std::endl;
    }

    formedHistoryNodes.insert(
        formedHistoryNodes.begin(),
        newIncrementNodes.begin(),
        newIncrementNodes.end()
    );

    previousWrappedLength =
        currentWrappedLength;

    previousSupportRotationAngle =
        state.supportRotationAngle;

    std::cout
        << "[MH1.17 HISTORY UPDATE]"
        << " deltaLength="
        << deltaLength
        << " deltaAngle="
        << deltaAngle
        << " newNodes="
        << newIncrementNodes.size()
        << " historyNodes="
        << formedHistoryNodes.size()
        << std::endl;

      

    return true;
}


bool StretchHelixFormingProcess::
appendFormedHistory(
    std::vector<PipeNode>& nodes) const
{
    if (formedHistoryNodes.empty())
        return true;

    nodes.insert(
        nodes.end(),
        formedHistoryNodes.begin(),
        formedHistoryNodes.end()
    );

    return true;
}