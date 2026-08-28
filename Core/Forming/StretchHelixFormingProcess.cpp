#include "Core/Forming/StretchHelixFormingProcess.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <limits>
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


    double distancePointToAxis(
        const Vec3D& point,
        const Vec3D& axisPoint,
        const Vec3D& axisDirection)
    {
        Vec3D axis =
            axisDirection.normalized();

        if (axis.lengthSquared() < 1e-12)
            return 0.0;

        const Vec3D relative =
            point - axisPoint;

        const double axialProjection =
            dot(
                relative,
                axis
            );

        const Vec3D radialVector =
            relative
            - axis * axialProjection;

        return
            radialVector.length();
    }



    struct RadiusStats
    {
        double minimum = 0.0;
        double maximum = 0.0;
        double average = 0.0;

        bool valid = false;
    };

    RadiusStats calculateRadiusStats(
        const std::vector<PipeNode>& nodes,
        const Vec3D& axisPoint,
        const Vec3D& axisDirection)
    {
        RadiusStats result;

        if (nodes.empty())
            return result;

        double sum =
            0.0;

        double minimum =
            std::numeric_limits<double>::max();

        double maximum =
            0.0;

        for (const PipeNode& node : nodes)
        {
            const double radius =
                distancePointToAxis(
                    node.pos,
                    axisPoint,
                    axisDirection
                );

            minimum =
                std::min(
                    minimum,
                    radius
                );

            maximum =
                std::max(
                    maximum,
                    radius
                );

            sum +=
                radius;
        }

        result.minimum =
            minimum;

        result.maximum =
            maximum;

        result.average =
            sum
            / static_cast<double>(
                nodes.size()
                );

        result.valid =
            true;

        return result;
    }

    double helixRadiusFromCurvatureTorsion(
        double curvature,
        double torsion)
    {
        const double denominator =
            curvature * curvature
            + torsion * torsion;

        if (!std::isfinite(denominator)
            || denominator <= 1e-18)
        {
            return 0.0;
        }

        return
            curvature
            / denominator;
    }


    double helixRisePerRadianFromCurvatureTorsion(
        double curvature,
        double torsion)
    {
        const double denominator =
            curvature * curvature
            + torsion * torsion;

        if (!std::isfinite(denominator)
            || denominator <= 1e-18)
        {
            return 0.0;
        }

        return
            torsion
            / denominator;
    }


    Frame buildHelixStartFrameForGlobalZAxis(
        const Vec3D& startPosition,
        double curvature,
        double torsion)
    {
        Frame frame;

        if (!std::isfinite(curvature)
            || !std::isfinite(torsion)
            || curvature <= 0.0)
        {
            return frame;
        }

        const double alpha =
            std::atan2(
                torsion,
                curvature
            );

        const double c =
            std::cos(alpha);

        const double s =
            std::sin(alpha);

        frame.P =
            startPosition;

        frame.T =
        {
            0.0,
            c,
            s
        };

        frame.N =
        {
            -1.0,
            0.0,
            0.0
        };

        frame.B =
        {
            0.0,
            -s,
            c
        };

        return frame;
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
 input.torsionSpringbackRatio = 0.10;
    input =
        newInput;
    input.torsionSpringbackRatio = 0.10;
    previousFormedReferenceIndex =
        0;
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

        if (!rebuildRequiredSupportGeometry())
        {
            std::cout
                << "[STRETCH HELIX PROCESS INIT FAIL]"
                << " reason=RequiredSupportGeometry"
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

   // referenceResult =
   //     integrator.integrate(
   //         startFrame,
   //         profile,
   //         input.sampleStep
   //     );


    referenceResult =
        integrator.integrate(
            finalHelixStartFrame,
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

    std::cout
        << "[MH1.MAREK CURRENT BUILD BEGIN]"
        << " nodes="
        << currentNodes.size()
        << std::endl;
    currentNodes.clear();
    std::cout
        << "[MH1.MAREK CURRENT BUILD BEGIN CLEAR]"
        << " nodes="
        << currentNodes.size()
        << std::endl;

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

    std::cout
        << "[MH1 CURRENT AFTER HISTORY]"
        << " nodes="
        << currentNodes.size()
        << std::endl;
    // =====================================================
    // BUILD CURRENT DISPLAY GEOMETRY
    // =====================================================

    if (!appendIncomingGeometry(
        currentNodes
    ))
    {
        return false;
    }

    std::cout
        << "[MH1 CURRENT AFTER INCOMING A]"
        << " nodes="
        << currentNodes.size()
        << std::endl;

    if (!appendFormedHistory(
        currentNodes
    ))
    {
        return false;
    }
    std::cout
        << "[MH1 CURRENT AFTER INCOMING B]"
        << " nodes="
        << currentNodes.size()
        << std::endl;
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
// temporary for debugging
    bool framesValid =
        true;

    for (std::size_t i = 0;
        i < currentNodes.size();
        ++i)
    {
        const PipeNode& node =
            currentNodes[i];

        const double t2 =
            node.T.lengthSquared();

        const double n2 =
            node.N.lengthSquared();

        const double b2 =
            node.B.lengthSquared();

        if (t2 < 1e-12
            || n2 < 1e-12
            || b2 < 1e-12)
        {
            framesValid =
                false;

            std::cout
                << "[MH1.18 INVALID FRAME]"
                << " index="
                << i
                << " T2="
                << t2
                << " N2="
                << n2
                << " B2="
                << b2
                << " P=("
                << node.pos.x << ", "
                << node.pos.y << ", "
                << node.pos.z
                << ")"
                << std::endl;

            break;
        }
    }std::cout
        << "[MH1.18 MESH FRAME CHECK]"
        << " nodes="
        << currentNodes.size()
        << " valid="
        << framesValid
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

    previousSupportAxialPosition =
        0.0;
    previousFormedReferenceIndex =
        0;
    
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

    finalHelixCurvature =
        kinematics.curvature;

    finalHelixTorsion =
        kinematics.torsion;
    finalHelixStartFrame =
        buildHelixStartFrameForGlobalZAxis(
            startFrame.P,
            finalHelixCurvature,
            finalHelixTorsion
        );

    const Vec3D finalLancret =
        finalHelixStartFrame.T
        * finalHelixTorsion
        + finalHelixStartFrame.B
        * finalHelixCurvature;

    const Vec3D finalAxisDirection =
        finalLancret.normalized();

    std::cout
        << "[MH1.21 FINAL FRAME]"
        << " T=("
        << finalHelixStartFrame.T.x << ", "
        << finalHelixStartFrame.T.y << ", "
        << finalHelixStartFrame.T.z
        << ")"
        << " B=("
        << finalHelixStartFrame.B.x << ", "
        << finalHelixStartFrame.B.y << ", "
        << finalHelixStartFrame.B.z
        << ")"
        << " axis=("
        << finalAxisDirection.x << ", "
        << finalAxisDirection.y << ", "
        << finalAxisDirection.z
        << ")"
        << std::endl;
    finalHelixRadius =
        helixRadiusFromCurvatureTorsion(
            finalHelixCurvature,
            finalHelixTorsion
        );

    finalHelixRisePerRadian =
        helixRisePerRadianFromCurvatureTorsion(
            finalHelixCurvature,
            finalHelixTorsion
        );

    finalHelixPitch =
        2.0
        * 3.14159265358979323846
        * finalHelixRisePerRadian;


    StretchBendingEvaluator evaluator;

    stretchEvaluation =
        evaluator.evaluate(
            mechanicalInput
        );
    if (!stretchEvaluation.valid)
    {
        return false;
    }

    // =================================================
   // H9 — COPY SPRINGBACK RESULT INTO PROCESS STATE
   // =================================================

    targetFinalCurvature =
        stretchEvaluation.finalTargetCurvature;

    loadedCurvature =
        stretchEvaluation.loadedCurvatureCommand;

    loadedHelixCurvature =
        loadedCurvature;



    // Temporary until torsion springback is implemented.
   // loadedHelixTorsion =
    //    finalHelixTorsion;

    const double torsionRecoveryFactor =
        1.0 - input.torsionSpringbackRatio;

   

    if (!std::isfinite(torsionRecoveryFactor)
        || torsionRecoveryFactor <= 1e-12)
    {
        return false;
    }

    loadedHelixTorsion =
        finalHelixTorsion
        / torsionRecoveryFactor;

    loadedHelixRadius =
        helixRadiusFromCurvatureTorsion(
            loadedHelixCurvature,
            loadedHelixTorsion
        );

    loadedHelixRisePerRadian =
        helixRisePerRadianFromCurvatureTorsion(
            loadedHelixCurvature,
            loadedHelixTorsion
        );

    loadedHelixPitch =
        2.0
        * 3.14159265358979323846
        * loadedHelixRisePerRadian;

    loadedHelixStartFrame =
        buildHelixStartFrameForGlobalZAxis(
            startFrame.P,
            loadedHelixCurvature,
            loadedHelixTorsion
        );
    const Vec3D loadedLancret =
        loadedHelixStartFrame.T
        * loadedHelixTorsion
        + loadedHelixStartFrame.B
        * loadedHelixCurvature;

    const Vec3D loadedAxisDirection =
        loadedLancret.normalized();

    std::cout
        << "[MH1.21 LOADED FRAME]"
        << " T=("
        << loadedHelixStartFrame.T.x << ", "
        << loadedHelixStartFrame.T.y << ", "
        << loadedHelixStartFrame.T.z
        << ")"
        << " B=("
        << loadedHelixStartFrame.B.x << ", "
        << loadedHelixStartFrame.B.y << ", "
        << loadedHelixStartFrame.B.z
        << ")"
        << " axis=("
        << loadedAxisDirection.x << ", "
        << loadedAxisDirection.y << ", "
        << loadedAxisDirection.z
        << ")"
        << std::endl;


    const Vec3D globalZ =
    {
        0.0,
        0.0,
        1.0
    };

    const double finalAxisDot =
        dot(
            finalAxisDirection,
            globalZ
        );

    const double loadedAxisDot =
        dot(
            loadedAxisDirection,
            globalZ
        );

    const bool finalAccepted =
        finalAxisDot >= 1.0 - 1e-12;

    const bool loadedAccepted =
        loadedAxisDot >= 1.0 - 1e-12;

    std::cout
        << "[MH1.21 AXIS ACCEPTANCE]"
        << " finalDot="
        << finalAxisDot
        << " loadedDot="
        << loadedAxisDot
        << " accepted="
        << (
            finalAccepted
            && loadedAccepted
            )
        << std::endl;

    loadedHelixRadius =
        helixRadiusFromCurvatureTorsion(
            loadedHelixCurvature,
            loadedHelixTorsion
        );

  
    loadedHelixRisePerRadian =
        helixRisePerRadianFromCurvatureTorsion(
            loadedHelixCurvature,
            loadedHelixTorsion
        );

    loadedHelixPitch =
        2.0
        * 3.14159265358979323846
        * loadedHelixRisePerRadian;
   
    std::cout
        << "[MH1.19A HELIX PARAMETERS]"
        << " finalKappa="
        << finalHelixCurvature
        << " finalTau="
        << finalHelixTorsion
        << " finalRadius="
        << finalHelixRadius
        << " finalPitch="
        << finalHelixPitch
        << " loadedKappa="
        << loadedHelixCurvature
        << " loadedTau="
        << loadedHelixTorsion
        << " loadedRadius="
        << loadedHelixRadius
        << " loadedPitch="
        << loadedHelixPitch
        << std::endl;
    const bool loadedTighter =
        loadedHelixRadius
        < finalHelixRadius;

    const bool curvatureCompensated =
        std::abs(
            loadedHelixCurvature
        )
        >
        std::abs(
            finalHelixCurvature
        );

    std::cout
        << "[MH1.19A HELIX ACCEPTANCE]"
        << " loadedTighter="
        << loadedTighter
        << " curvatureCompensated="
        << curvatureCompensated
        << std::endl;

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

    const bool torsionCompensated =
        std::abs(loadedHelixTorsion)
    >
        std::abs(finalHelixTorsion);
    std::cout
        << "[MH1.20 TORSION SPRINGBACK]"
        << " finalTau="
        << finalHelixTorsion
        << " ratio="
        << input.torsionSpringbackRatio
        << " loadedTau="
        << loadedHelixTorsion
        << " finalPitch="
        << finalHelixPitch
        << " loadedPitch="
        << loadedHelixPitch
        << " compensated="
        << torsionCompensated
        << std::endl;


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





 return
        stretchEvaluation.valid;
    
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
        << loadedHelixCurvature
        << " predictedFinalKappa="
        << predictedFinalCurvature
        << " torsion="
        << loadedHelixTorsion
        << " theoreticalRadius="
        << loadedHelixRadius
        << " theoreticalPitch="
        << loadedHelixPitch
        << " length="
        << input.pipeArcLength
        << " sampleStep="
        << input.sampleStep
        << std::endl;

    // =====================================================
    // VALIDATION
    // =====================================================

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

    if (!std::isfinite(
        loadedHelixCurvature
    ))
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=LoadedCurvatureNotFinite"
            << std::endl;

        return false;
    }

    if (loadedHelixCurvature <= 0.0)
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=LoadedCurvatureNotPositive"
            << " loadedKappa="
            << loadedHelixCurvature
            << std::endl;

        return false;
    }

    if (!std::isfinite(
        loadedHelixTorsion
    ))
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=LoadedTorsionNotFinite"
            << std::endl;

        return false;
    }

    if (!std::isfinite(
        loadedHelixRadius
    )
        || loadedHelixRadius <= 0.0)
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=LoadedRadiusInvalid"
            << " radius="
            << loadedHelixRadius
            << std::endl;

        return false;
    }

    // =====================================================
    // BUILD LOADED CURVATURE / TORSION PROFILE
    //
    // Use explicit MH1.19A loaded helix parameters.
    // =====================================================

    const CurvatureTorsionProfile loadedProfile =
        ConstantCurvatureTorsionProfileBuilder::build(
            input.pipeArcLength,
            loadedHelixCurvature,
            loadedHelixTorsion
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

    // =====================================================
    // INTEGRATE LOADED HELIX
    // =====================================================

    SpatialCurveIntegrator integrator;

   // loadedReferenceResult =
   //     integrator.integrate(
   //         startFrame,
   //         loadedProfile,
   //         input.sampleStep
   //     );

    loadedReferenceResult =
        integrator.integrate(
            loadedHelixStartFrame,
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

    // =====================================================
    // MH1.19A.7 — THEORETICAL LOADED HELIX AXIS
    //
    // For constant kappa / tau:
    //
    // axisDirection ? tau*T0 + kappa*B0
    //
    // Frenet N0 points from the helix centerline toward
    // the helix axis.
    //
    // axisPoint = P0 + N0 * R_loaded
    // =====================================================

   // Vec3D loadedAxisDirection =
   //     startFrame.T
   //     * loadedHelixTorsion
   //     + startFrame.B
   //     * loadedHelixCurvature;

    Vec3D loadedAxisDirection =
        loadedHelixStartFrame.T
        * loadedHelixTorsion
        + loadedHelixStartFrame.B
        * loadedHelixCurvature;

    loadedAxisDirection =
        loadedAxisDirection.normalized();

    if (loadedAxisDirection.lengthSquared() < 1e-12)
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=LoadedAxisDirectionInvalid"
            << std::endl;

        return false;
    }

    loadedAxisDirection =
        loadedAxisDirection.normalized();

    const Vec3D loadedNormal =
        startFrame.N.normalized();

    if (loadedNormal.lengthSquared() < 1e-12)
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=LoadedStartNormalInvalid"
            << std::endl;

        return false;
    }

    //const Vec3D loadedAxisPoint =
    //    startFrame.P
    //    + loadedNormal
    //    * loadedHelixRadius;

    const Vec3D loadedAxisPoint =
        loadedHelixStartFrame.P
        + loadedHelixStartFrame.N.normalized()
        * loadedHelixRadius;

    // =====================================================
    // MH1.19A.7 — MEASURE LOADED REFERENCE AROUND ITS
    // OWN THEORETICAL AXIS
    // =====================================================

    const RadiusStats loadedStats =
        calculateRadiusStats(
            loadedReferenceResult.nodes,
            loadedAxisPoint,
            loadedAxisDirection
        );

    if (!loadedStats.valid)
    {
        std::cout
            << "[STRETCH HELIX LOADED BUILD FAIL]"
            << " reason=LoadedRadiusStatsInvalid"
            << std::endl;

        return false;
    }

    std::cout
        << "[MH1.19A LOADED AXIS]"
        << " point=("
        << loadedAxisPoint.x
        << ", "
        << loadedAxisPoint.y
        << ", "
        << loadedAxisPoint.z
        << ")"
        << " direction=("
        << loadedAxisDirection.x
        << ", "
        << loadedAxisDirection.y
        << ", "
        << loadedAxisDirection.z
        << ")"
        << std::endl;

    std::cout
        << "[MH1.19A LOADED RADIUS CHECK]"
        << " theoreticalRadius="
        << loadedHelixRadius
        << " measuredAvg="
        << loadedStats.average
        << " measuredMin="
        << loadedStats.minimum
        << " measuredMax="
        << loadedStats.maximum
        << std::endl;

    // =====================================================
    // ACCEPTANCE
    // =====================================================

    const double averageError =
        std::abs(
            loadedStats.average
            - loadedHelixRadius
        );

    const double radialSpread =
        loadedStats.maximum
        - loadedStats.minimum;

    constexpr double radiusTolerance =
        0.5;

    const bool averageAccepted =
        averageError <= radiusTolerance;

    const bool spreadAccepted =
        radialSpread <= radiusTolerance;

    std::cout
        << "[MH1.19A LOADED RADIUS ACCEPTANCE]"
        << " averageError="
        << averageError
        << " radialSpread="
        << radialSpread
        << " averageAccepted="
        << averageAccepted
        << " spreadAccepted="
        << spreadAccepted
        << " accepted="
        << (
            averageAccepted
            && spreadAccepted
            )
        << std::endl;

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


// LEGACY MH1 PREFIX REBUILD
// No longer used by Workshop incremental-history workflow.


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
            / static_cast<double>(segmentCount);

        const double distanceFromActive =
            incomingLength
            * (1.0 - fraction);

        Vec3D T =
            activeFormingFrame.T.normalized();

        Vec3D N =
            activeFormingFrame.N
            - T * dot(
                activeFormingFrame.N,
                T
            );

        if (N.lengthSquared() < 1e-12)
        {
            return false;
        }

        N =
            N.normalized();

        Vec3D B =
            cross(
                T,
                N
            ).normalized();

        N =
            cross(
                B,
                T
            ).normalized();

        PipeNode node;

        node.pos =
            activeFormingFrame.P
            - T
            * distanceFromActive;

        node.T = T;
        node.N = N;
        node.B = B;

        nodes.push_back(node);
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
        requiredSupportAxisFrame.P;

    Vec3D supportAxisDirection =
        requiredSupportAxisFrame.T;

    const double deltaAxialPosition =
        state.supportAxialPosition
        - previousSupportAxialPosition;

    if (!std::isfinite(deltaAxialPosition))
    {
        return false;
    }

    if (supportAxisDirection.lengthSquared() < 1e-12)
        return false;

    supportAxisDirection =
        supportAxisDirection.normalized();


    if (std::abs(deltaAngle) > 1e-12)
        for (PipeNode& node :
            formedHistoryNodes)
        {
            // Rotate old formed material with support.
            RigidTransformUtils::
                rotateNodeAroundAxis(
                    node,
                    supportAxisPoint,
                    supportAxisDirection,
                    deltaAngle
                );

            // Move old formed material axially with support.
            node.pos +=
                supportAxisDirection
                * deltaAxialPosition;
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

    const std::vector<PipeNode>& referenceNodes =
        formingReference->nodes;

    const std::size_t lastReferenceIndex =
        referenceNodes.size() - 1;

    // =====================================================
    // MH1.17B — CUMULATIVE MATERIAL INDEX
    // =====================================================

    const double formedFraction =
        std::clamp(
            currentWrappedLength
            / input.pipeArcLength,
            0.0,
            1.0
        );

    const std::size_t targetFormedReferenceIndex =
        std::min(
            lastReferenceIndex,
            static_cast<std::size_t>(
                std::llround(
                    formedFraction
                    * static_cast<double>(
                        lastReferenceIndex
                        )
                )
                )
        );

    if (
        targetFormedReferenceIndex
        < previousFormedReferenceIndex
        )
    {
        return false;
    }

    const std::size_t oldFormedReferenceIndex =
        previousFormedReferenceIndex;

    const std::size_t newSegmentCount =
        targetFormedReferenceIndex
        - previousFormedReferenceIndex;

    // =====================================================
    // NO NEW MATERIAL SAMPLES THIS STEP
    // =====================================================

    if (newSegmentCount == 0)
    {
        previousWrappedLength =
            currentWrappedLength;

        previousSupportRotationAngle =
            state.supportRotationAngle;

        previousSupportAxialPosition =
            state.supportAxialPosition;

        return true;
    }

    // =====================================================
    // BUILD NEW INCREMENT
    // =====================================================

    const Vec3D referenceOrigin =
        referenceNodes.front().pos;

    const Vec3D formingPoint =
        activeFormingFrame.P;

    std::vector<PipeNode>
        newIncrementNodes;

    newIncrementNodes.reserve(
        newSegmentCount
    );

    for (std::size_t i = 1;
        i <= newSegmentCount;
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

    previousSupportAxialPosition =
        state.supportAxialPosition;

    previousFormedReferenceIndex =
        targetFormedReferenceIndex;
    std::cout
       
     
    << "[MH1.17 HISTORY UPDATE]"
    << " deltaLength="
    << deltaLength
    << " deltaAngle="
    << deltaAngle
    << " deltaAxial="
    << deltaAxialPosition
    << " newNodes="
    << newIncrementNodes.size()
    << " historyNodes="
    << formedHistoryNodes.size()
    << std::endl;

    std::cout
        << "[MH1.17B SAMPLING]"
        << " wrappedLength="
        << currentWrappedLength
        << " previousIndex="
        << oldFormedReferenceIndex
        << " targetIndex="
        << targetFormedReferenceIndex
        << " newSegments="
        << newSegmentCount
        << " historyNodes="
        << formedHistoryNodes.size()
        << std::endl;

    if (!formedHistoryNodes.empty())
    {
        const Vec3D axisPoint =
            requiredSupportAxisFrame.P;

        const Vec3D axisDirection =
            requiredSupportAxisFrame.T.normalized();

        const PipeNode& historyNode =
            formedHistoryNodes[
                formedHistoryNodes.size() / 2
            ];

        const double historyRadius =
            distancePointToAxis(
                historyNode.pos,
                axisPoint,
                axisDirection
            );

        std::cout
            << "[MH1.17C HISTORY RADIUS]"
            << " radius="
            << historyRadius
            << " nodes="
            << formedHistoryNodes.size()
            << std::endl;
    }

    const Vec3D loadedAxisPoint =
    requiredSupportAxisFrame.P;

const Vec3D loadedAxisDirection =
    requiredSupportAxisFrame.T.normalized();

const RadiusStats historyStats =
    calculateRadiusStats(
        formedHistoryNodes,
        loadedAxisPoint,
        loadedAxisDirection
    );

const RadiusStats loadedStats =
    calculateRadiusStats(
        loadedReferenceResult.nodes,
        loadedAxisPoint,
        loadedAxisDirection
    );

   // const RadiusStats finalStats =
   //     calculateRadiusStats(
   //         finalResult.nodes,
   //         axisPoint,
   //         axisDirection
   //     );

    std::cout
        << "[MH1.19C LOADED TOOL COMPARISON]"
        << " historyAvg="
        << historyStats.average
        << " historyMin="
        << historyStats.minimum
        << " historyMax="
        << historyStats.maximum

        << " loadedAvg="
        << loadedStats.average
        << " loadedMin="
        << loadedStats.minimum
        << " loadedMax="
        << loadedStats.maximum

        << " expectedLoadedRadius="
        << loadedHelixRadius

       // << " finalAvg="
       // << finalStats.average
       // << " finalMin="
       // << finalStats.minimum
       // << " finalMax="
      //  << finalStats.maximum
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

bool StretchHelixFormingProcess::
rebuildRequiredSupportGeometry()
{
    requiredSupportOuterRadius =
        0.0;

    requiredSupportAxisFrame =
        Frame{};

    if (!std::isfinite(loadedHelixRadius)
        || loadedHelixRadius <= 0.0)
    {
        return false;
    }

    const double pipeOuterRadius =
        input.pipeSection.outerDiameter
        * 0.5;

    if (!std::isfinite(pipeOuterRadius)
        || pipeOuterRadius <= 0.0)
    {
        return false;
    }

    requiredSupportOuterRadius =
        loadedHelixRadius
        - pipeOuterRadius;

    if (!std::isfinite(requiredSupportOuterRadius)
        || requiredSupportOuterRadius <= 0.0)
    {
        return false;
    }

   // Vec3D axisDirection =
   //     startFrame.T
   //     * loadedHelixTorsion
   //     + startFrame.B
   //     * loadedHelixCurvature;
    Vec3D axisDirection =
        loadedHelixStartFrame.T
        * loadedHelixTorsion
        + loadedHelixStartFrame.B
        * loadedHelixCurvature;


    if (axisDirection.lengthSquared() < 1e-12)
        return false;

    axisDirection =
        axisDirection.normalized();

    const Vec3D loadedNormal =
        startFrame.N.normalized();

    if (loadedNormal.lengthSquared() < 1e-12)
        return false;

   // const Vec3D axisPoint =
   //     startFrame.P
   //     + loadedNormal
   //     * loadedHelixRadius;

    const Vec3D axisPoint =
        loadedHelixStartFrame.P
        + loadedHelixStartFrame.N.normalized()
        * loadedHelixRadius;

    requiredSupportAxisFrame.P =
        axisPoint;

    requiredSupportAxisFrame.T =
        axisDirection;

    requiredSupportAxisFrame.N =
        (
            startFrame.P
            - axisPoint
            ).normalized();

    requiredSupportAxisFrame.B =
        cross(
            requiredSupportAxisFrame.T,
            requiredSupportAxisFrame.N
        ).normalized();

    requiredSupportAxisFrame.N =
        cross(
            requiredSupportAxisFrame.B,
            requiredSupportAxisFrame.T
        ).normalized();
    std::cout
        << "[MH1.19B REQUIRED SUPPORT]"
        << " loadedCenterlineRadius="
        << loadedHelixRadius
        << " pipeOuterRadius="
        << pipeOuterRadius
        << " supportOuterRadius="
        << requiredSupportOuterRadius
        << " axisPoint=("
        << requiredSupportAxisFrame.P.x
        << ", "
        << requiredSupportAxisFrame.P.y
        << ", "
        << requiredSupportAxisFrame.P.z
        << ")"
        << " axisDir=("
        << requiredSupportAxisFrame.T.x
        << ", "
        << requiredSupportAxisFrame.T.y
        << ", "
        << requiredSupportAxisFrame.T.z
        << ")"
        << std::endl;

    const double reconstructedCenterlineRadius =
        requiredSupportOuterRadius
        + pipeOuterRadius;

    const double supportRadiusError =
        std::abs(
            reconstructedCenterlineRadius
            - loadedHelixRadius
        );

    constexpr double tolerance =
        1e-9;

    const bool accepted =
        supportRadiusError <= tolerance;

    std::cout
        << "[MH1.19B SUPPORT ACCEPTANCE]"
        << " reconstructedCenterlineRadius="
        << reconstructedCenterlineRadius
        << " expectedLoadedRadius="
        << loadedHelixRadius
        << " error="
        << supportRadiusError
        << " accepted="
        << accepted
        << std::endl;


    return true;
}

double StretchHelixFormingProcess::
getRequiredSupportOuterRadius() const
{
    return
        requiredSupportOuterRadius;
}

const Frame&
StretchHelixFormingProcess::
getRequiredSupportAxisFrame() const
{
    return
        requiredSupportAxisFrame;
}