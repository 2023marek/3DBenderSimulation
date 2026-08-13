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

    input =
        newInput;

    startFrame =
        newStartFrame;

    referenceResult.clear();
    currentNodes.clear();

    if (!input.isValid())
        return false;

    if (!rebuildKinematics())
        return false;

    mechanicsValid =
        rebuildStretchEvaluation();

    if (!rebuildReferenceGeometry())
        return false;

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

    if (!rebuildCurrentGeometry())
        return false;

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

    if (!referenceResult.valid)
        return false;

    if (!referenceResult.isComplete())
        return false;

    const std::vector<PipeNode>& referenceNodes =
        referenceResult.nodes;

    if (referenceNodes.size() < 2)
        return false;

    if (!state.isValidForLength(
        input.pipeArcLength
    ))
    {
        return false;
    }

    const double totalLength =
        input.pipeArcLength;

    const double frontS =
        state.contactFrontS;

    const double normalizedFront =
        std::clamp(
            frontS / totalLength,
            0.0,
            1.0
        );

    const std::size_t lastIndex =
        referenceNodes.size() - 1;

    const std::size_t frontIndex =
        static_cast<std::size_t>(
            std::llround(
                normalizedFront
                * static_cast<double>(
                    lastIndex
                    )
            )
            );

    currentNodes.reserve(
        referenceNodes.size()
    );

    // Copy wrapped region.
    for (std::size_t i = 0;
        i <= frontIndex;
        ++i)
    {
        currentNodes.push_back(
            referenceNodes[i]
        );
    }

    // Find tangent.
    Vec3D tangent;

    if (frontIndex == 0)
    {
        tangent =
            referenceNodes[1].pos
            - referenceNodes[0].pos;
    }
    else if (frontIndex >= lastIndex)
    {
        tangent =
            referenceNodes[lastIndex].pos
            - referenceNodes[lastIndex - 1].pos;
    }
    else
    {
        tangent =
            referenceNodes[frontIndex + 1].pos
            - referenceNodes[frontIndex - 1].pos;
    }

    tangent =
        tangent.normalized();
    const Vec3D frontPosition =
        referenceNodes[frontIndex].pos;

    const double remainingLength =
        totalLength - frontS;

    const std::size_t remainingNodeCount =
        lastIndex - frontIndex;

    if (remainingNodeCount > 0)
    {
        for (std::size_t j = 1;
            j <= remainingNodeCount;
            ++j)
        {
            const double fraction =
                static_cast<double>(j)
                / static_cast<double>(
                    remainingNodeCount
                    );

            PipeNode node =
                referenceNodes[frontIndex];

            node.pos =
                frontPosition
                + tangent
                * (
                    remainingLength
                    * fraction
                    );

            currentNodes.push_back(
                node
            );
        }
    }

    return
        currentNodes.size()
        == referenceNodes.size();
}

void StretchHelixFormingProcess::
advanceTime(
    double dt)
{
    if (!valid)
        return;

    if (state.complete)
        return;

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
        && state.complete;
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
        0.0;

    mechanicalInput.compensateSpringback =
        false;

    mechanicalInput.enabled =
        true;

    if (!mechanicalInput.isValid())
        return false;

    StretchBendingEvaluator evaluator;

    stretchEvaluation =
        evaluator.evaluate(
            mechanicalInput
        );

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