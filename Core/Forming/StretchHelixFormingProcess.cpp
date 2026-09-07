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
    bool mh12010LoadedHoldChecked = false;
    bool mh12010LoadedHoldAccepted = false;


    
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


    formedHistoryNodes.clear();
    referenceResult.clear();
    loadedReferenceResult.clear();
    finalResult.clear();
    currentNodes.clear();
    mh12010LastWrappingNodes.clear();

    previousWrappedLength = 0.0;
    previousSupportRotationAngle = 0.0;
    previousSupportAxialPosition = 0.0;
    previousFormedReferenceIndex = 0;
    
    mh12010WrappingSnapshotValid = false;
    mh12010LoadedHoldChecked = false;
    mh12010LoadedHoldAccepted = false;

   

    mh12010C18PreviousHistoryFrontPosition =
        Vec3D{};

    mh12010C18PreviousHistoryFrontValid =
        false;

    mh12010C19FrontLocalSourceIndex = 0;

    mh12010C19FrontRepresentedLocalLength = 0.0;

    mh12010C19FrontActualDeltaLength = 0.0;

    mh12010C19FrontOriginValid = false;

    mh12010C21EStoredFrontRadiusError = 0.0;
    mh12010C21EStoredFrontRadiusValid = false;

    mh12010C21HRigidHistoryNodes.clear();
    mh12010C21HRigidHistoryValid = false;

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

    currentNodes.clear();

    std::cout
        << "[MH1 CURRENT AFTER CLEAR]"
        << " nodes="
        << currentNodes.size()
        << std::endl;
    // ============================================================
      // BASIC INPUT / STATE VALIDATION
      // ============================================================

    if (!input.isValid())
        return false;

    if (!state.isValidForLength(
        input.pipeArcLength
    ))
    {
        return false;
    }

    // ============================================================
    // UPDATE PERSISTENT FORMED MATERIAL
    //
    // This updates formedHistoryNodes.
    //
    // IMPORTANT:
    // currentNodes is still empty after this call.
    // We have updated the SOURCE history, but have not yet built
    // the display geometry.
    // ===============================================================================================================


    if (!updateFormedHistory())
    {
        return false;
    }

    std::cout
        << "[MH1 CURRENT AFTER HISTORY UPDATE]"
        << " persistentHistoryNodes="
        << formedHistoryNodes.size()
        << " displayNodes="
        << currentNodes.size()
        << std::endl;

    std::cout
        << "[MH1.20.10A AFTER HISTORY UPDATE]"
        << " formedHistoryNodes="
        << formedHistoryNodes.size()
        << " currentNodes="
        << currentNodes.size()
        << std::endl;

    // ============================================================
    // APPEND INCOMING / UNFORMED GEOMETRY
    //
    // After this call currentNodes contains the incoming section,
    // but NOT YET the complete displayed pipe.
    //
    // Therefore:
    //     DO NOT perform endpoint identity checks here.
    // ============================================================


    if (!appendIncomingGeometry(currentNodes))
    {
        return false;
    }



    std::cout
        << "[MH1 CURRENT AFTER INCOMING]"
        << " nodes="
        << currentNodes.size()
        << std::endl;

    std::cout
        << "[MH1.20.10A AFTER INCOMING]"
        << " incomingDisplayNodes="
        << currentNodes.size()
        << " formedHistoryNodes="
        << formedHistoryNodes.size()
        << std::endl;

    // ============================================================
   // APPEND FORMED HISTORY
   //
   // After this call currentNodes represents the COMPLETE
   // displayed pipe:
   //
   //     incoming geometry
   //          +
   //     formed history
   //
   // Whole-geometry diagnostics belong AFTER this point.
   // ============================================================


    if (!appendFormedHistory(currentNodes))
    {
        return false;
    }

    std::cout
        << "[MH1 CURRENT AFTER FORMED HISTORY]"
        << " nodes="
        << currentNodes.size()
        << std::endl;

       // ============================================================
       // CURRENT LENGTH DIAGNOSTIC
       // ============================================================

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

    // ============================================================
// MH1.20.10A.2
//
// ENDPOINT IDENTITY DIAGNOSTIC
//
// We use exactly the same reference-selection rule as
// appendFormedGeometry().
// ============================================================

    const SpatialCurveIntegrationResult* diagnosticReference =
        &referenceResult;

    if (
        mechanicsValid
        && loadedReferenceResult.valid
        && loadedReferenceResult.isComplete()
        )
    {
        diagnosticReference =
            &loadedReferenceResult;
    }

    if (!currentNodes.empty()
        && diagnosticReference->valid
        && diagnosticReference->isComplete()
        && !diagnosticReference->nodes.empty())
    {
        const std::vector<PipeNode>& referenceNodes =
            diagnosticReference->nodes;

        const Vec3D& currentFirstP =
            currentNodes.front().pos;

        const Vec3D& currentLastP =
            currentNodes.back().pos;

        const Vec3D& referenceFirstP =
            referenceNodes.front().pos;

        const Vec3D& referenceLastP =
            referenceNodes.back().pos;

        const double firstGap =
            (
                currentFirstP
                - referenceFirstP
                ).length();

        const double lastGap =
            (
                currentLastP
                - referenceLastP
                ).length();

        const double firstToReferenceLastGap =
            (
                currentFirstP
                - referenceLastP
                ).length();

        const double lastToReferenceFirstGap =
            (
                currentLastP
                - referenceFirstP
                ).length();

     
    }

    // MH1.18
        // MESH FRAME VALIDATION
        //
        // Every displayed node must contain a usable orthonormal-ish
        // material frame before the mesh renderer consumes it.
        // ============================================================




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

    
        // ============================================================
        // MH1.20.10B
        //
        // Verify continuity between:
        //
        //     final Wrapping geometry
        //
        // and
        //
        //     geometry present when LoadedHold is entered.
        //
        // IMPORTANT:
        //
        // This check must happen BEFORE:
        //
        //     stage = Unloading;
        //     advanceUnloading(dt);
        //
        // because advanceUnloading() rebuilds analytical unloading
        // geometry and would no longer represent the pure stage
        // boundary.
        // ============================================================

        if (!mh12010LoadedHoldChecked)
        {
            mh12010LoadedHoldChecked =
                true;

            const std::size_t wrappingCount =
                mh12010LastWrappingNodes.size();

            const std::size_t loadedHoldCount =
                currentNodes.size();

            const bool countMatch =
                mh12010WrappingSnapshotValid
                && wrappingCount == loadedHoldCount;

            double maxPositionGap =
                0.0;

            double averagePositionGap =
                0.0;

            double minimumTangentDot =
                1.0;

            std::size_t comparedNodes =
                0;

            if (countMatch)
            {
                double positionGapSum =
                    0.0;

                for (std::size_t i = 0;
                    i < wrappingCount;
                    ++i)
                {
                    const PipeNode& wrappingNode =
                        mh12010LastWrappingNodes[i];

                    const PipeNode& loadedHoldNode =
                        currentNodes[i];

                    const double positionGap =
                        (
                            loadedHoldNode.pos
                            - wrappingNode.pos
                            ).length();

                    positionGapSum +=
                        positionGap;

                    maxPositionGap =
                        std::max(
                            maxPositionGap,
                            positionGap
                        );

                    const double tangentDot =
                        dot(
                            wrappingNode.T,
                            loadedHoldNode.T
                        );

                    minimumTangentDot =
                        std::min(
                            minimumTangentDot,
                            tangentDot
                        );

                    ++comparedNodes;
                }

                if (comparedNodes > 0)
                {
                    averagePositionGap =
                        positionGapSum
                        / static_cast<double>(
                            comparedNodes
                            );
                }
            }

            const double positionTolerance =
                1e-9;

            const double tangentTolerance =
                1e-9;

            mh12010LoadedHoldAccepted =
                countMatch
                && comparedNodes == wrappingCount
                && maxPositionGap
                <= positionTolerance
                && minimumTangentDot
                >= 1.0 - tangentTolerance;

            std::cout
                << "[MH1.20.10B LOADED HOLD CONTINUITY]"
                << " wrappingNodes="
                << wrappingCount
                << " loadedHoldNodes="
                << loadedHoldCount
                << " comparedNodes="
                << comparedNodes
                << " averagePositionGap="
                << averagePositionGap
                << " maxPositionGap="
                << maxPositionGap
                << " minTangentDot="
                << minimumTangentDot
                << " accepted="
                << mh12010LoadedHoldAccepted
                << std::endl;
        }


        const double testFraction = 0.0;

        const double testCurvature =
            loadedHelixCurvature;

        const double testTorsion =
            loadedHelixTorsion;

        const Frame testStartFrame =
            buildHelixStartFrameForGlobalZAxis(
                startFrame.P,
                testCurvature,
                testTorsion
            );

       


        const CurvatureTorsionProfile testProfile =
            ConstantCurvatureTorsionProfileBuilder::build(
                input.pipeArcLength,
                testCurvature,
                testTorsion
            );

        SpatialCurveIntegrator integrator;

        const SpatialCurveIntegrationResult testResult =
            integrator.integrate(
                testStartFrame,
                testProfile,
                input.sampleStep
            );

        std::cout
            << "[MH1.20.10C FRACTION ZERO BUILD]"
            << " valid="
            << testResult.valid
            << " complete="
            << testResult.isComplete()
            << " loadedHoldNodes="
            << currentNodes.size()
            << " analyticalNodes="
            << testResult.nodes.size()
            << std::endl;

        if (
            testResult.valid
            && testResult.isComplete()
            && !testResult.nodes.empty()
            && !currentNodes.empty()
            )
        {
            const Vec3D& holdFirst =
                currentNodes.front().pos;

            const Vec3D& holdLast =
                currentNodes.back().pos;

            const Vec3D& analyticalFirst =
                testResult.nodes.front().pos;

            const Vec3D& analyticalLast =
                testResult.nodes.back().pos;

            const double firstToFirst =
                (holdFirst - analyticalFirst).length();

            const double firstToLast =
                (holdFirst - analyticalLast).length();

            const double lastToFirst =
                (holdLast - analyticalFirst).length();

            const double lastToLast =
                (holdLast - analyticalLast).length();

            std::cout
                << "[MH1.20.10C ENDPOINT RELATION]"
                << " firstToFirst="
                << firstToFirst
                << " firstToLast="
                << firstToLast
                << " lastToFirst="
                << lastToFirst
                << " lastToLast="
                << lastToLast
                << std::endl;
        }


        // ============================================================
// MH1.20.10C.2
//
// Compare SHAPE INVARIANTS of the LoadedHold manufacturing
// history against the theoretical loaded helix.
//
// We do NOT compare raw node positions here.
//
// Why?
//
// The manufacturing history is accumulated and transformed
// during wrapping, while the analytical helix is freshly
// integrated from a start frame.
//
// Therefore raw endpoint identity is not a reliable test.
//
// Instead we compare:
//
//     1. helix radius
//     2. helix pitch / rise per radian
//     3. helix axis direction
//
// If these match, then the LoadedHold shape is mechanically
// consistent with the required loaded helix.
// ============================================================

        if (
            mh12010WrappingSnapshotValid
            && mh12010LastWrappingNodes.size() >= 3
            )
        {
            const std::vector<PipeNode>& holdNodes =
                mh12010LastWrappingNodes;

            // --------------------------------------------------------
            // Theoretical loaded helix values
            // --------------------------------------------------------

            const double theoreticalRadius =
                loadedHelixRadius;

            const double theoreticalRisePerRadian =
                loadedHelixRisePerRadian;

            const double theoreticalPitch =
                loadedHelixPitch;

            const Vec3D theoreticalAxisDirection =
            {
                0.0,
                0.0,
                1.0
            };


            // --------------------------------------------------------
            // Reconstruct the theoretical axis position.
            //
            // For the loaded helix:
            //
            //     start point
            //         +
            //     start normal * loaded radius
            //
            // gives a point on the helix axis.
            // --------------------------------------------------------

            const Frame loadedStartFrame =
                buildHelixStartFrameForGlobalZAxis(
                    startFrame.P,
                    loadedHelixCurvature,
                    loadedHelixTorsion
                );

            const Vec3D theoreticalAxisPoint =
                loadedStartFrame.P
                + loadedStartFrame.N
                * theoreticalRadius;


            // ========================================================
            // PART 1
            // Measure radius of LoadedHold history
            // ========================================================

            double radiusSum =
                0.0;

            double measuredMinRadius =
                std::numeric_limits<double>::max();

            double measuredMaxRadius =
                0.0;

            std::size_t radiusSampleCount =
                0;

            for (const PipeNode& node : holdNodes)
            {
                const Vec3D relative =
                    node.pos
                    - theoreticalAxisPoint;

                const double axialProjection =
                    dot(
                        relative,
                        theoreticalAxisDirection
                    );

                const Vec3D radialVector =
                    relative
                    - theoreticalAxisDirection
                    * axialProjection;

                const double measuredRadius =
                    radialVector.length();

                if (!std::isfinite(measuredRadius))
                {
                    continue;
                }

                radiusSum +=
                    measuredRadius;

                measuredMinRadius =
                    std::min(
                        measuredMinRadius,
                        measuredRadius
                    );

                measuredMaxRadius =
                    std::max(
                        measuredMaxRadius,
                        measuredRadius
                    );

                ++radiusSampleCount;
            }

            double measuredAverageRadius =
                0.0;

            if (radiusSampleCount > 0)
            {
                measuredAverageRadius =
                    radiusSum
                    / static_cast<double>(
                        radiusSampleCount
                        );
            }

            const double radiusAverageError =
                std::abs(
                    measuredAverageRadius
                    - theoreticalRadius
                );

            const double radiusSpread =
                measuredMaxRadius
                - measuredMinRadius;
// Is here correct place to insert MH1.20.10C.3  ?
//
// Inspect radial error along the LoadedHold manufacturing
// history.

// ============================================================
// MH1.20.10C.3
//
// Inspect radial error along the LoadedHold manufacturing
// history.
//
// C.2 showed:
//
//     correct pitch
//     correct axis
//     radius slowly varies
//
// Now determine whether that radius error is:
//
//     constant,
//     random,
//     or accumulated along the wrapped history.
// ============================================================

            const std::size_t lastHoldIndex =
                holdNodes.size() - 1;

            const std::size_t diagnosticIndices[] =
            {
                0,
                lastHoldIndex / 4,
                lastHoldIndex / 2,
                (3 * lastHoldIndex) / 4,
                lastHoldIndex
            };

            for (const std::size_t index : diagnosticIndices)
            {
                const PipeNode& node =
                    holdNodes[index];

                const Vec3D relative =
                    node.pos
                    - theoreticalAxisPoint;

                const double axialProjection =
                    dot(
                        relative,
                        theoreticalAxisDirection
                    );

                const Vec3D radialVector =
                    relative
                    - theoreticalAxisDirection
                    * axialProjection;

                const double measuredRadius =
                    radialVector.length();

                const double radialError =
                    measuredRadius
                    - theoreticalRadius;

                const double normalizedPosition =
                    static_cast<double>(index)
                    / static_cast<double>(lastHoldIndex);

                std::cout
                    << "[MH1.20.10C.3 RADIAL DRIFT]"
                    << " fraction="
                    << normalizedPosition
                    << " index="
                    << index
                    << " radius="
                    << measuredRadius
                    << " error="
                    << radialError
                    << std::endl;
            }


            // ========================================================
            // PART 2
            // Measure pitch from approximately one full revolution
            // ========================================================

            bool pitchMeasurementValid =
                false;

            double measuredRisePerRadian =
                0.0;

            double measuredPitch =
                0.0;

            double measuredAngle =
                0.0;

            std::size_t pitchEndIndex =
                0;

            if (holdNodes.size() >= 2)
            {
                const double twoPi =
                    2.0 * 3.14159265358979323846;

                const Vec3D firstRelative =
                    holdNodes.front().pos
                    - theoreticalAxisPoint;

                const double firstAxialProjection =
                    dot(
                        firstRelative,
                        theoreticalAxisDirection
                    );

                Vec3D previousRadialDirection =
                    firstRelative
                    - theoreticalAxisDirection
                    * firstAxialProjection;

                const double previousRadius =
                    previousRadialDirection.length();

                if (previousRadius > 1e-12)
                {
                    previousRadialDirection =
                        previousRadialDirection
                        / previousRadius;

                    const double startAxialPosition =
                        dot(
                            firstRelative,
                            theoreticalAxisDirection
                        );

                    double accumulatedAngle =
                        0.0;

                    for (std::size_t i = 1;
                        i < holdNodes.size();
                        ++i)
                    {
                        const Vec3D relative =
                            holdNodes[i].pos
                            - theoreticalAxisPoint;

                        const double axialProjection =
                            dot(
                                relative,
                                theoreticalAxisDirection
                            );

                        Vec3D radialDirection =
                            relative
                            - theoreticalAxisDirection
                            * axialProjection;

                        const double radialLength =
                            radialDirection.length();

                        if (radialLength <= 1e-12)
                        {
                            continue;
                        }

                        radialDirection =
                            radialDirection
                            / radialLength;

                        const Vec3D radialCross =
                            cross(
                                previousRadialDirection,
                                radialDirection
                            );

                        const double sinAngle =
                            dot(
                                radialCross,
                                theoreticalAxisDirection
                            );

                        const double cosAngle =
                            dot(
                                previousRadialDirection,
                                radialDirection
                            );

                        const double deltaAngle =
                            std::atan2(
                                sinAngle,
                                cosAngle
                            );

                        accumulatedAngle +=
                            deltaAngle;

                        previousRadialDirection =
                            radialDirection;

                        if (std::abs(accumulatedAngle)
                            >= twoPi)
                        {
                            const double endAxialPosition =
                                axialProjection;

                            const double axialAdvance =
                                endAxialPosition
                                - startAxialPosition;

                            measuredAngle =
                                accumulatedAngle;

                            measuredRisePerRadian =
                                axialAdvance
                                / measuredAngle;

                            measuredPitch =
                                measuredRisePerRadian
                                * twoPi;

                            pitchEndIndex =
                                i;

                            pitchMeasurementValid =
                                std::isfinite(
                                    measuredPitch
                                );

                            break;
                        }
                    }
                }
            }

            // ============================================================
            // MH1.20.10C.4
            //
            // Estimate the actual transverse axis center of the
            // LoadedHold manufacturing helix.
            //
            // We already know from C.2 that the helix axis direction is
            // intended to be global Z.
            //
            // Therefore the unknown part of the axis is only:
            //
            //     center X
            //     center Y
            //
            // We use approximately one complete revolution, already found
            // by the pitch measurement.
            //
            // For uniformly sampled points around a complete circle:
            //
            //     average X ? circle center X
            //     average Y ? circle center Y
            //
            // This is a diagnostic estimate only.
            // It does NOT modify production geometry.
            // ============================================================

            bool estimatedCenterValid =
                false;

            Vec3D estimatedAxisPoint =
                theoreticalAxisPoint;

            double estimatedCenterOffset =
                0.0;

            std::size_t centerSampleCount =
                0;

            if (
                pitchMeasurementValid
                && pitchEndIndex > 0
                && pitchEndIndex < holdNodes.size()
                )
            {
                double xSum =
                    0.0;

                double ySum =
                    0.0;

                // --------------------------------------------------------
                // Use the same approximately-one-turn interval that was
                // identified by the pitch diagnostic.
                // --------------------------------------------------------

                for (std::size_t i = 0;
                    i <= pitchEndIndex;
                    ++i)
                {
                    xSum +=
                        holdNodes[i].pos.x;

                    ySum +=
                        holdNodes[i].pos.y;

                    ++centerSampleCount;
                }

                if (centerSampleCount > 0)
                {
                    const double estimatedCenterX =
                        xSum
                        / static_cast<double>(
                            centerSampleCount
                            );

                    const double estimatedCenterY =
                        ySum
                        / static_cast<double>(
                            centerSampleCount
                            );

                    // ----------------------------------------------------
                    // Z does not determine the transverse center because
                    // the helix axis extends along global Z.
                    //
                    // Keep the theoretical axis point Z only for forming
                    // a convenient Vec3D representation.
                    // ----------------------------------------------------

                    estimatedAxisPoint =
                    {
                        estimatedCenterX,
                        estimatedCenterY,
                        theoreticalAxisPoint.z
                    };

                    const double centerDx =
                        estimatedCenterX
                        - theoreticalAxisPoint.x;

                    const double centerDy =
                        estimatedCenterY
                        - theoreticalAxisPoint.y;

                    estimatedCenterOffset =
                        std::sqrt(
                            centerDx * centerDx
                            + centerDy * centerDy
                        );

                    estimatedCenterValid =
                        std::isfinite(
                            estimatedCenterOffset
                        );
                }
            }


            // ============================================================
// Re-measure LoadedHold radius using the estimated axis center.
//
// If the previous radius variation was mainly caused by a
// displaced measurement axis, this spread should collapse.
// ============================================================

            double estimatedCenterRadiusSum =
                0.0;

            double estimatedCenterMinRadius =
                std::numeric_limits<double>::max();

            double estimatedCenterMaxRadius =
                0.0;

            std::size_t estimatedCenterRadiusSamples =
                0;

            if (estimatedCenterValid)
            {
                for (const PipeNode& node : holdNodes)
                {
                    const Vec3D relative =
                        node.pos
                        - estimatedAxisPoint;

                    const double axialProjection =
                        dot(
                            relative,
                            theoreticalAxisDirection
                        );

                    const Vec3D radialVector =
                        relative
                        - theoreticalAxisDirection
                        * axialProjection;

                    const double radius =
                        radialVector.length();

                    if (!std::isfinite(radius))
                    {
                        continue;
                    }

                    estimatedCenterRadiusSum +=
                        radius;

                    estimatedCenterMinRadius =
                        std::min(
                            estimatedCenterMinRadius,
                            radius
                        );

                    estimatedCenterMaxRadius =
                        std::max(
                            estimatedCenterMaxRadius,
                            radius
                        );

                    ++estimatedCenterRadiusSamples;
                }
            }

            double estimatedCenterAverageRadius =
                0.0;

            if (estimatedCenterRadiusSamples > 0)
            {
                estimatedCenterAverageRadius =
                    estimatedCenterRadiusSum
                    / static_cast<double>(
                        estimatedCenterRadiusSamples
                        );
            }

            double estimatedCenterRadiusSpread =
                0.0;

            double estimatedCenterRadiusError =
                0.0;

            if (estimatedCenterRadiusSamples > 0)
            {
                estimatedCenterRadiusSpread =
                    estimatedCenterMaxRadius
                    - estimatedCenterMinRadius;

                estimatedCenterRadiusError =
                    std::abs(
                        estimatedCenterAverageRadius
                        - theoreticalRadius
                    );
            }


            std::cout
                << "[MH1.20.10C.4 AXIS CENTER]"
                << " theoretical=("
                << theoreticalAxisPoint.x
                << ", "
                << theoreticalAxisPoint.y
                << ")"
                << " estimated=("
                << estimatedAxisPoint.x
                << ", "
                << estimatedAxisPoint.y
                << ")"
                << " offset="
                << estimatedCenterOffset
                << " samples="
                << centerSampleCount
                << " valid="
                << estimatedCenterValid
                << std::endl;

            std::cout
                << "[MH1.20.10C.4 RECENTERED RADIUS]"
                << " theoretical="
                << theoreticalRadius
                << " measuredAvg="
                << estimatedCenterAverageRadius
                << " measuredMin="
                << estimatedCenterMinRadius
                << " measuredMax="
                << estimatedCenterMaxRadius
                << " averageError="
                << estimatedCenterRadiusError
                << " spread="
                << estimatedCenterRadiusSpread
                << " samples="
                << estimatedCenterRadiusSamples
                << std::endl;


            // ============================================================
// MH1.20.10C.5
//
// Estimate a LOCAL transverse helix-axis center at several
// locations along the LoadedHold manufacturing history.
//
// C.4 showed that one constant correction to the theoretical
// axis center does NOT remove the radius spread.
//
// Now test whether the apparent helix center moves along the
// accumulated manufacturing history.
//
// We inspect three approximately-one-turn windows:
//
//     START
//     MIDDLE
//     END
//
// Diagnostic only.
// No production geometry is modified.
// ============================================================

            if (
                pitchMeasurementValid
                && pitchEndIndex > 10
                && holdNodes.size() > pitchEndIndex
                )
            {
                const std::size_t oneTurnNodeCount =
                    pitchEndIndex + 1;

                // --------------------------------------------------------
                // Helper lambda:
                //
                // Estimate X/Y center by averaging the points over one
                // approximately complete revolution.
                //
                // This is not yet a precision circle fit.
                // It is sufficient for detecting center drift.
                // --------------------------------------------------------

                auto estimateWindowCenter =
                    [&holdNodes](
                        std::size_t beginIndex,
                        std::size_t endIndex
                        ) -> Vec3D
                    {
                        double xSum = 0.0;
                        double ySum = 0.0;

                        std::size_t count = 0;

                        for (std::size_t i = beginIndex;
                            i <= endIndex;
                            ++i)
                        {
                            xSum += holdNodes[i].pos.x;
                            ySum += holdNodes[i].pos.y;

                            ++count;
                        }

                        if (count == 0)
                        {
                            return {};
                        }

                        return
                        {
                            xSum / static_cast<double>(count),
                            ySum / static_cast<double>(count),
                            0.0
                        };
                    };


                // --------------------------------------------------------
                // START window
                // --------------------------------------------------------

                const std::size_t startBegin =
                    0;

                const std::size_t startEnd =
                    std::min(
                        holdNodes.size() - 1,
                        oneTurnNodeCount - 1
                    );


                // --------------------------------------------------------
                // MIDDLE window
                // --------------------------------------------------------

                const std::size_t halfTurn =
                    oneTurnNodeCount / 2;

                const std::size_t middleIndex =
                    holdNodes.size() / 2;

                std::size_t middleBegin =
                    0;

                if (middleIndex > halfTurn)
                {
                    middleBegin =
                        middleIndex - halfTurn;
                }

                std::size_t middleEnd =
                    middleBegin
                    + oneTurnNodeCount
                    - 1;

                if (middleEnd >= holdNodes.size())
                {
                    middleEnd =
                        holdNodes.size() - 1;

                    middleBegin =
                        middleEnd
                        - oneTurnNodeCount
                        + 1;
                }


                // --------------------------------------------------------
                // END window
                // --------------------------------------------------------

                const std::size_t endEnd =
                    holdNodes.size() - 1;

                const std::size_t endBegin =
                    endEnd
                    - oneTurnNodeCount
                    + 1;


                // --------------------------------------------------------
                // Estimate local centers
                // --------------------------------------------------------

                const Vec3D startCenter =
                    estimateWindowCenter(
                        startBegin,
                        startEnd
                    );

                const Vec3D middleCenter =
                    estimateWindowCenter(
                        middleBegin,
                        middleEnd
                    );

                const Vec3D endCenter =
                    estimateWindowCenter(
                        endBegin,
                        endEnd
                    );

//


// ============================================================
// MH1.20.10C.6
//
// Measure LOCAL radius statistics inside each approximately
// one-turn window, using that window's own estimated center.
//
// This separates:
//
//     center drift
//
// from:
//
//     true local radial distortion.
// ============================================================

                auto measureLocalRadius =
                    [&holdNodes](
                        std::size_t beginIndex,
                        std::size_t endIndex,
                        const Vec3D& localCenter,
                        double& averageRadius,
                        double& minRadius,
                        double& maxRadius,
                        double& spread,
                        std::size_t& sampleCount
                        )
                    {
                        double radiusSum =
                            0.0;

                        minRadius =
                            std::numeric_limits<double>::max();

                        maxRadius =
                            0.0;

                        sampleCount =
                            0;

                        for (std::size_t i = beginIndex;
                            i <= endIndex;
                            ++i)
                        {
                            const double dx =
                                holdNodes[i].pos.x
                                - localCenter.x;

                            const double dy =
                                holdNodes[i].pos.y
                                - localCenter.y;

                            const double radius =
                                std::sqrt(
                                    dx * dx
                                    + dy * dy
                                );

                            if (!std::isfinite(radius))
                            {
                                continue;
                            }

                            radiusSum +=
                                radius;

                            minRadius =
                                std::min(
                                    minRadius,
                                    radius
                                );

                            maxRadius =
                                std::max(
                                    maxRadius,
                                    radius
                                );

                            ++sampleCount;
                        }

                        averageRadius =
                            0.0;

                        spread =
                            0.0;

                        if (sampleCount > 0)
                        {
                            averageRadius =
                                radiusSum
                                / static_cast<double>(
                                    sampleCount
                                    );

                            spread =
                                maxRadius
                                - minRadius;
                        }
                    };


                double startAverageRadius = 0.0;
                double startMinRadius = 0.0;
                double startMaxRadius = 0.0;
                double startRadiusSpread = 0.0;
                std::size_t startRadiusSamples = 0;

                double middleAverageRadius = 0.0;
                double middleMinRadius = 0.0;
                double middleMaxRadius = 0.0;
                double middleRadiusSpread = 0.0;
                std::size_t middleRadiusSamples = 0;

                double endAverageRadius = 0.0;
                double endMinRadius = 0.0;
                double endMaxRadius = 0.0;
                double endRadiusSpread = 0.0;
                std::size_t endRadiusSamples = 0;

                measureLocalRadius(
                    startBegin,
                    startEnd,
                    startCenter,
                    startAverageRadius,
                    startMinRadius,
                    startMaxRadius,
                    startRadiusSpread,
                    startRadiusSamples
                );

                measureLocalRadius(
                    middleBegin,
                    middleEnd,
                    middleCenter,
                    middleAverageRadius,
                    middleMinRadius,
                    middleMaxRadius,
                    middleRadiusSpread,
                    middleRadiusSamples
                );

                measureLocalRadius(
                    endBegin,
                    endEnd,
                    endCenter,
                    endAverageRadius,
                    endMinRadius,
                    endMaxRadius,
                    endRadiusSpread,
                    endRadiusSamples
                );

                std::cout
                    << "[MH1.20.10C.6 LOCAL RADIUS START]"
                    << " theoretical="
                    << theoreticalRadius
                    << " average="
                    << startAverageRadius
                    << " min="
                    << startMinRadius
                    << " max="
                    << startMaxRadius
                    << " spread="
                    << startRadiusSpread
                    << " averageError="
                    << std::abs(
                        startAverageRadius
                        - theoreticalRadius
                    )
                    << " samples="
                    << startRadiusSamples
                    << std::endl;


                std::cout
                    << "[MH1.20.10C.6 LOCAL RADIUS MIDDLE]"
                    << " theoretical="
                    << theoreticalRadius
                    << " average="
                    << middleAverageRadius
                    << " min="
                    << middleMinRadius
                    << " max="
                    << middleMaxRadius
                    << " spread="
                    << middleRadiusSpread
                    << " averageError="
                    << std::abs(
                        middleAverageRadius
                        - theoreticalRadius
                    )
                    << " samples="
                    << middleRadiusSamples
                    << std::endl;


                std::cout
                    << "[MH1.20.10C.6 LOCAL RADIUS END]"
                    << " theoretical="
                    << theoreticalRadius
                    << " average="
                    << endAverageRadius
                    << " min="
                    << endMinRadius
                    << " max="
                    << endMaxRadius
                    << " spread="
                    << endRadiusSpread
                    << " averageError="
                    << std::abs(
                        endAverageRadius
                        - theoreticalRadius
                    )
                    << " samples="
                    << endRadiusSamples
                    << std::endl;




                // --------------------------------------------------------
                // Compare each local center against the theoretical center.
                // --------------------------------------------------------

                auto centerOffsetFromTheory =
                    [&theoreticalAxisPoint](
                        const Vec3D& center
                        ) -> double
                    {
                        const double dx =
                            center.x
                            - theoreticalAxisPoint.x;

                        const double dy =
                            center.y
                            - theoreticalAxisPoint.y;

                        return std::sqrt(
                            dx * dx
                            + dy * dy
                        );
                    };


                const double startOffset =
                    centerOffsetFromTheory(
                        startCenter
                    );

                const double middleOffset =
                    centerOffsetFromTheory(
                        middleCenter
                    );

                const double endOffset =
                    centerOffsetFromTheory(
                        endCenter
                    );


                const double startToMiddle =
                    std::sqrt(
                        (middleCenter.x - startCenter.x)
                        * (middleCenter.x - startCenter.x)
                        +
                        (middleCenter.y - startCenter.y)
                        * (middleCenter.y - startCenter.y)
                    );

                const double middleToEnd =
                    std::sqrt(
                        (endCenter.x - middleCenter.x)
                        * (endCenter.x - middleCenter.x)
                        +
                        (endCenter.y - middleCenter.y)
                        * (endCenter.y - middleCenter.y)
                    );

                const double startToEnd =
                    std::sqrt(
                        (endCenter.x - startCenter.x)
                        * (endCenter.x - startCenter.x)
                        +
                        (endCenter.y - startCenter.y)
                        * (endCenter.y - startCenter.y)
                    );


                std::cout
                    << "[MH1.20.10C.5 LOCAL CENTER START]"
                    << " begin="
                    << startBegin
                    << " end="
                    << startEnd
                    << " center=("
                    << startCenter.x
                    << ", "
                    << startCenter.y
                    << ")"
                    << " theoryOffset="
                    << startOffset
                    << std::endl;


                std::cout
                    << "[MH1.20.10C.5 LOCAL CENTER MIDDLE]"
                    << " begin="
                    << middleBegin
                    << " end="
                    << middleEnd
                    << " center=("
                    << middleCenter.x
                    << ", "
                    << middleCenter.y
                    << ")"
                    << " theoryOffset="
                    << middleOffset
                    << std::endl;


                std::cout
                    << "[MH1.20.10C.5 LOCAL CENTER END]"
                    << " begin="
                    << endBegin
                    << " end="
                    << endEnd
                    << " center=("
                    << endCenter.x
                    << ", "
                    << endCenter.y
                    << ")"
                    << " theoryOffset="
                    << endOffset
                    << std::endl;


                std::cout
                    << "[MH1.20.10C.5 CENTER DRIFT]"
                    << " startToMiddle="
                    << startToMiddle
                    << " middleToEnd="
                    << middleToEnd
                    << " startToEnd="
                    << startToEnd
                    << std::endl;
            }






            // ========================================================
            // PART 3
            // Axis direction acceptance
            //
            // The manufacturing helix is intended to wrap around
            // global Z.
            //
            // We already use the same theoretical axis to measure
            // radius and pitch, so this check documents the intended
            // invariant explicitly.
            // ========================================================

            const double axisDirectionDot =
                dot(
                    theoreticalAxisDirection,
                    Vec3D{ 0.0, 0.0, 1.0 }
                );

            const bool axisAccepted =
                axisDirectionDot
                >= 1.0 - 1e-12;


            // ========================================================
            // Acceptance
            // ========================================================

            const double radiusTolerance =
                0.01;

            const double radiusSpreadTolerance =
                0.01;

            const double pitchTolerance =
                0.01;

            const bool radiusAccepted =
                radiusSampleCount > 0
                && radiusAverageError
                <= radiusTolerance
                && radiusSpread
                <= radiusSpreadTolerance;

            const double pitchError =
                std::abs(
                    std::abs(measuredPitch)
                    - std::abs(theoreticalPitch)
                );

            const bool pitchAccepted =
                pitchMeasurementValid
                && pitchError
                <= pitchTolerance;

            const bool accepted =
                radiusAccepted
                && pitchAccepted
                && axisAccepted;


            // ========================================================
            // Diagnostics
            // ========================================================

            std::cout
                << "[MH1.20.10C.2 LOADED HOLD RADIUS]"
                << " theoretical="
                << theoreticalRadius
                << " measuredAvg="
                << measuredAverageRadius
                << " measuredMin="
                << measuredMinRadius
                << " measuredMax="
                << measuredMaxRadius
                << " averageError="
                << radiusAverageError
                << " spread="
                << radiusSpread
                << " samples="
                << radiusSampleCount
                << " accepted="
                << radiusAccepted
                << std::endl;


            std::cout
                << "[MH1.20.10C.2 LOADED HOLD PITCH]"
                << " theoreticalPitch="
                << theoreticalPitch
                << " measuredPitch="
                << measuredPitch
                << " theoreticalRisePerRadian="
                << theoreticalRisePerRadian
                << " measuredRisePerRadian="
                << measuredRisePerRadian
                << " measuredAngle="
                << measuredAngle
                << " endIndex="
                << pitchEndIndex
                << " pitchError="
                << pitchError
                << " accepted="
                << pitchAccepted
                << std::endl;


            std::cout
                << "[MH1.20.10C.2 LOADED HOLD AXIS]"
                << " axis=("
                << theoreticalAxisDirection.x
                << ", "
                << theoreticalAxisDirection.y
                << ", "
                << theoreticalAxisDirection.z
                << ")"
                << " dotZ="
                << axisDirectionDot
                << " accepted="
                << axisAccepted
                << std::endl;


            std::cout
                << "[MH1.20.10C.2 ACCEPTANCE]"
                << " radiusAccepted="
                << radiusAccepted
                << " pitchAccepted="
                << pitchAccepted
                << " axisAccepted="
                << axisAccepted
                << " accepted="
                << accepted
                << std::endl;
        }






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


    // ============================================================
// MH1.20.10C.21H
//
// Reset diagnostic rigid shadow history together with the
// production formed history.
//
// The shadow history belongs to one wrapping run only.
// ============================================================

    mh12010C21HRigidHistoryNodes.clear();
    mh12010C21HRigidHistoryValid = false;

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
        kinematics,
        loadedHelixRadius,
        loadedHelixRisePerRadian
    );

    std::cout
        << "[MH1.20.10A PRE-SNAPSHOT]"
        << " currentNodes="
        << currentNodes.size()
       // << " referenceNodes="
       // << formingReference->nodes.size()
        << " wrappedLength="
        << state.wrappedLength
        << " pipeArcLength="
        << input.pipeArcLength
        << std::endl;
    if (!rebuildCurrentGeometry())
    {
        valid =
            false;

        return;
    }

    if (state.complete)
    {

        mh12010LastWrappingNodes =
            currentNodes;

        mh12010WrappingSnapshotValid =
            !mh12010LastWrappingNodes.empty();
        // ========================================================
           // MH1.20.10C.21H.1
           //
           // Final node-count acceptance for the diagnostic rigid
           // shadow history.
           //
           // We are still in Wrapping here, but state.complete tells
           // us that the complete pipe length has been wrapped.
           // ========================================================

        const long long rigidHistoryCountDifference =
            static_cast<long long>(
                mh12010C21HRigidHistoryNodes.size()
                )
            - static_cast<long long>(
                formedHistoryNodes.size()
                );

        const bool rigidHistoryCountAccepted =
            rigidHistoryCountDifference == 0;

        std::cout
            << "[MH1.20.10C.21H.1 FINAL COUNT]"
            << " productionNodes="
            << formedHistoryNodes.size()
            << " rigidHistoryNodes="
            << mh12010C21HRigidHistoryNodes.size()
            << " difference="
            << rigidHistoryCountDifference
            << " accepted="
            << rigidHistoryCountAccepted
            << std::endl;
        // ============================================================
// MH1.20.10C.21H.2
//
// LoadedHold rigid-history radius acceptance.
//
// Measure the diagnostic rigid shadow history against the
// theoretical LOADED helix centerline radius.
//
// IMPORTANT:
//
//     Diagnostic only.
//     Production formedHistoryNodes is NOT modified.
//
// ============================================================

        if (
            mh12010C21HRigidHistoryValid
            && !mh12010C21HRigidHistoryNodes.empty()
            )
        {
            Vec3D rigidAxisDirection =
                requiredSupportAxisFrame.T;

            bool rigidRadiusMeasurementValid =
                false;

            double rigidRadiusSum =
                0.0;

            double rigidMinimumRadius =
                std::numeric_limits<double>::max();

            double rigidMaximumRadius =
                0.0;

            std::size_t rigidRadiusSampleCount =
                0;


            // --------------------------------------------------------
            // Validate support axis.
            // --------------------------------------------------------

            if (rigidAxisDirection.lengthSquared() > 1e-12)
            {
                rigidAxisDirection =
                    rigidAxisDirection.normalized();


                // ----------------------------------------------------
                // Measure every rigid-history node from the SAME
                // support axis used during rigid construction.
                // ----------------------------------------------------

                for (const PipeNode& node :
                    mh12010C21HRigidHistoryNodes)
                {
                    const Vec3D relative =
                        node.pos
                        - requiredSupportAxisFrame.P;

                    const double axialCoordinate =
                        dot(
                            relative,
                            rigidAxisDirection
                        );

                    const Vec3D radialVector =
                        relative
                        - rigidAxisDirection
                        * axialCoordinate;

                    const double radius =
                        radialVector.length();

                    if (!std::isfinite(radius))
                    {
                        continue;
                    }

                    rigidRadiusSum +=
                        radius;

                    rigidMinimumRadius =
                        std::min(
                            rigidMinimumRadius,
                            radius
                        );

                    rigidMaximumRadius =
                        std::max(
                            rigidMaximumRadius,
                            radius
                        );

                    ++rigidRadiusSampleCount;
                }


                if (rigidRadiusSampleCount > 0)
                {
                    rigidRadiusMeasurementValid =
                        true;
                }
            }


            // --------------------------------------------------------
            // Derived statistics.
            // --------------------------------------------------------

            double rigidAverageRadius =
                0.0;

            double rigidRadiusSpread =
                0.0;

            double rigidAverageRadiusError =
                0.0;


            if (rigidRadiusMeasurementValid)
            {
                rigidAverageRadius =
                    rigidRadiusSum
                    / static_cast<double>(
                        rigidRadiusSampleCount
                        );

                rigidRadiusSpread =
                    rigidMaximumRadius
                    - rigidMinimumRadius;

                rigidAverageRadiusError =
                    std::abs(
                        rigidAverageRadius
                        - loadedHelixRadius
                    );
            }


            // --------------------------------------------------------
            // Acceptance.
            //
            // Keep the same practical scale used earlier for loaded
            // radius acceptance.
            // --------------------------------------------------------

            const double rigidRadiusTolerance =
                0.01;

            const double rigidRadiusSpreadTolerance =
                0.01;

            const bool rigidRadiusAccepted =
                rigidRadiusMeasurementValid
                && rigidAverageRadiusError
                <= rigidRadiusTolerance
                && rigidRadiusSpread
                <= rigidRadiusSpreadTolerance;


            std::cout
                << "[MH1.20.10C.21H.2 RIGID HISTORY RADIUS]"
                << " theoretical="
                << loadedHelixRadius
                << " measuredAvg="
                << rigidAverageRadius
                << " measuredMin="
                << rigidMinimumRadius
                << " measuredMax="
                << rigidMaximumRadius
                << " averageError="
                << rigidAverageRadiusError
                << " spread="
                << rigidRadiusSpread
                << " samples="
                << rigidRadiusSampleCount
                << " valid="
                << rigidRadiusMeasurementValid
                << " accepted="
                << rigidRadiusAccepted
                << std::endl;
        }
        // ============================================================
        // MH1.20.10C.21H.3
        //
        // LoadedHold rigid-history pitch acceptance.
        //
        // Measure approximately one complete revolution of the
        // diagnostic rigid shadow history and compare its axial rise
        // against the theoretical LOADED helix pitch.
        //
        // Diagnostic only.
        // Production geometry is NOT modified.
        // ============================================================

        if (
            mh12010C21HRigidHistoryValid
            && mh12010C21HRigidHistoryNodes.size() >= 2
            )
        {
            Vec3D rigidAxisDirection =
                requiredSupportAxisFrame.T;

            bool rigidPitchMeasurementValid =
                false;

            double rigidMeasuredAngle =
                0.0;

            double rigidMeasuredRisePerRadian =
                0.0;

            double rigidMeasuredPitch =
                0.0;

            double rigidAxialAdvance =
                0.0;

            std::size_t rigidPitchEndIndex =
                0;


            if (rigidAxisDirection.lengthSquared() > 1e-12)
            {
                rigidAxisDirection =
                    rigidAxisDirection.normalized();

                const double twoPi =
                    2.0 * 3.14159265358979323846;


                // --------------------------------------------------------
                // Establish radial direction of the first shadow node.
                // --------------------------------------------------------

                const Vec3D firstRelative =
                    mh12010C21HRigidHistoryNodes.front().pos
                    - requiredSupportAxisFrame.P;

                const double firstAxialCoordinate =
                    dot(
                        firstRelative,
                        rigidAxisDirection
                    );

                Vec3D previousRadialDirection =
                    firstRelative
                    - rigidAxisDirection
                    * firstAxialCoordinate;

                const double firstRadius =
                    previousRadialDirection.length();


                if (firstRadius > 1e-12)
                {
                    previousRadialDirection =
                        previousRadialDirection
                        / firstRadius;

                    double accumulatedAngle =
                        0.0;


                    // ----------------------------------------------------
                    // Walk along history until approximately one complete
                    // revolution has accumulated.
                    // ----------------------------------------------------

                    for (
                        std::size_t i = 1;
                        i < mh12010C21HRigidHistoryNodes.size();
                        ++i
                        )
                    {
                        const Vec3D relative =
                            mh12010C21HRigidHistoryNodes[i].pos
                            - requiredSupportAxisFrame.P;

                        const double axialCoordinate =
                            dot(
                                relative,
                                rigidAxisDirection
                            );

                        Vec3D radialDirection =
                            relative
                            - rigidAxisDirection
                            * axialCoordinate;

                        const double radialLength =
                            radialDirection.length();

                        if (radialLength <= 1e-12)
                        {
                            continue;
                        }

                        radialDirection =
                            radialDirection
                            / radialLength;


                        // ------------------------------------------------
                        // Signed angular step about the support axis.
                        // ------------------------------------------------

                        const Vec3D radialCross =
                            cross(
                                previousRadialDirection,
                                radialDirection
                            );

                        const double sinAngle =
                            dot(
                                radialCross,
                                rigidAxisDirection
                            );

                        const double cosAngle =
                            std::clamp(
                                dot(
                                    previousRadialDirection,
                                    radialDirection
                                ),
                                -1.0,
                                1.0
                            );

                        const double deltaMeasuredAngle =
                            std::atan2(
                                sinAngle,
                                cosAngle
                            );

                        accumulatedAngle +=
                            deltaMeasuredAngle;

                        previousRadialDirection =
                            radialDirection;


                        // ------------------------------------------------
                        // One complete revolution reached.
                        // ------------------------------------------------

                        if (std::abs(accumulatedAngle) >= twoPi)
                        {
                            rigidAxialAdvance =
                                axialCoordinate
                                - firstAxialCoordinate;

                            rigidMeasuredAngle =
                                accumulatedAngle;

                            if (std::abs(rigidMeasuredAngle) > 1e-12)
                            {
                                rigidMeasuredRisePerRadian =
                                    rigidAxialAdvance
                                    / rigidMeasuredAngle;

                                rigidMeasuredPitch =
                                    rigidMeasuredRisePerRadian
                                    * twoPi;

                                rigidPitchEndIndex =
                                    i;

                                rigidPitchMeasurementValid =
                                    std::isfinite(
                                        rigidMeasuredPitch
                                    )
                                    && std::isfinite(
                                        rigidMeasuredRisePerRadian
                                    );
                            }

                            break;
                        }
                    }
                }
            }


            // --------------------------------------------------------
            // Compare magnitudes because wrapping direction may reverse
            // the signs of both angle and axial advance.
            // --------------------------------------------------------

            const double rigidPitchError =
                rigidPitchMeasurementValid
                ? std::abs(
                    std::abs(rigidMeasuredPitch)
                    - std::abs(loadedHelixPitch)
                )
                : std::numeric_limits<double>::infinity();


            const double rigidRisePerRadianError =
                rigidPitchMeasurementValid
                ? std::abs(
                    std::abs(rigidMeasuredRisePerRadian)
                    - std::abs(loadedHelixRisePerRadian)
                )
                : std::numeric_limits<double>::infinity();


            const double rigidPitchTolerance =
                0.01;

            const double rigidRisePerRadianTolerance =
                0.01;


            const bool rigidPitchAccepted =
                rigidPitchMeasurementValid
                && rigidPitchError
                <= rigidPitchTolerance
                && rigidRisePerRadianError
                <= rigidRisePerRadianTolerance;


            std::cout
                << "[MH1.20.10C.21H.3 RIGID HISTORY PITCH]"
                << " theoreticalPitch="
                << loadedHelixPitch
                << " measuredPitch="
                << rigidMeasuredPitch
                << " pitchError="
                << rigidPitchError
                << " theoreticalRisePerRadian="
                << loadedHelixRisePerRadian
                << " measuredRisePerRadian="
                << rigidMeasuredRisePerRadian
                << " riseError="
                << rigidRisePerRadianError
                << " measuredAngle="
                << rigidMeasuredAngle
                << " axialAdvance="
                << rigidAxialAdvance
                << " endIndex="
                << rigidPitchEndIndex
                << " samples="
                << mh12010C21HRigidHistoryNodes.size()
                << " valid="
                << rigidPitchMeasurementValid
                << " accepted="
                << rigidPitchAccepted
                << std::endl;
        }


        // ============================================================
// MH1.20.10C.21H.4
//
// LoadedHold rigid-history axis acceptance.
//
// Measure the helix axis from the displacement over
// approximately one complete revolution.
//
// Diagnostic only.
// Production geometry is NOT modified.
// ============================================================

        if (
            mh12010C21HRigidHistoryValid
            && mh12010C21HRigidHistoryNodes.size() >= 2
            )
        {
            Vec3D expectedAxisDirection =
                requiredSupportAxisFrame.T;

            bool rigidAxisMeasurementValid =
                false;

            Vec3D measuredAxisDirection =
            {
                0.0,
                0.0,
                0.0
            };

            double rigidAxisDirectionDot =
                0.0;

            double rigidAxisAngleRadians =
                0.0;

            double rigidOneTurnRadialClosure =
                0.0;

            double rigidOneTurnAxialAdvance =
                0.0;

            std::size_t rigidAxisEndIndex =
                0;


            if (expectedAxisDirection.lengthSquared() > 1e-12)
            {
                expectedAxisDirection =
                    expectedAxisDirection.normalized();

                const double twoPi =
                    2.0 * 3.14159265358979323846;


                // ----------------------------------------------------
                // Build the initial radial direction.
                // ----------------------------------------------------

                const Vec3D firstPosition =
                    mh12010C21HRigidHistoryNodes.front().pos;

                const Vec3D firstRelative =
                    firstPosition
                    - requiredSupportAxisFrame.P;

                const double firstAxialCoordinate =
                    dot(
                        firstRelative,
                        expectedAxisDirection
                    );

                Vec3D previousRadialDirection =
                    firstRelative
                    - expectedAxisDirection
                    * firstAxialCoordinate;

                const double firstRadialLength =
                    previousRadialDirection.length();


                if (firstRadialLength > 1e-12)
                {
                    previousRadialDirection =
                        previousRadialDirection
                        / firstRadialLength;

                    double accumulatedAngle =
                        0.0;


                    // ------------------------------------------------
                    // Find approximately one complete revolution.
                    // ------------------------------------------------

                    for (
                        std::size_t i = 1;
                        i < mh12010C21HRigidHistoryNodes.size();
                        ++i
                        )
                    {
                        const Vec3D currentPosition =
                            mh12010C21HRigidHistoryNodes[i].pos;

                        const Vec3D relative =
                            currentPosition
                            - requiredSupportAxisFrame.P;

                        const double axialCoordinate =
                            dot(
                                relative,
                                expectedAxisDirection
                            );

                        Vec3D radialDirection =
                            relative
                            - expectedAxisDirection
                            * axialCoordinate;

                        const double radialLength =
                            radialDirection.length();

                        if (radialLength <= 1e-12)
                        {
                            continue;
                        }

                        radialDirection =
                            radialDirection
                            / radialLength;


                        const Vec3D radialCross =
                            cross(
                                previousRadialDirection,
                                radialDirection
                            );

                        const double sinAngle =
                            dot(
                                radialCross,
                                expectedAxisDirection
                            );

                        const double cosAngle =
                            std::clamp(
                                dot(
                                    previousRadialDirection,
                                    radialDirection
                                ),
                                -1.0,
                                1.0
                            );

                        const double deltaMeasuredAngle =
                            std::atan2(
                                sinAngle,
                                cosAngle
                            );

                        accumulatedAngle +=
                            deltaMeasuredAngle;

                        previousRadialDirection =
                            radialDirection;


                        // ------------------------------------------------
                        // One revolution reached.
                        // ------------------------------------------------

                        if (std::abs(accumulatedAngle) >= twoPi)
                        {
                            const Vec3D displacement =
                                currentPosition
                                - firstPosition;


                            // --------------------------------------------
                            // Split one-turn displacement into axial
                            // and radial components relative to expected
                            // support axis.
                            // --------------------------------------------

                            rigidOneTurnAxialAdvance =
                                dot(
                                    displacement,
                                    expectedAxisDirection
                                );

                            const Vec3D radialClosureVector =
                                displacement
                                - expectedAxisDirection
                                * rigidOneTurnAxialAdvance;

                            rigidOneTurnRadialClosure =
                                radialClosureVector.length();


                            // --------------------------------------------
                            // The full displacement is almost axial.
                            //
                            // Because our end node is discrete and may
                            // lie slightly past 2*pi, there can be a
                            // small radial closure error.
                            // --------------------------------------------

                            const double displacementLength =
                                displacement.length();

                            if (displacementLength > 1e-12)
                            {
                                measuredAxisDirection =
                                    displacement
                                    / displacementLength;


                                // Axis has no meaningful +/- orientation
                                // for this acceptance test.
                                rigidAxisDirectionDot =
                                    std::abs(
                                        dot(
                                            measuredAxisDirection,
                                            expectedAxisDirection
                                        )
                                    );

                                rigidAxisDirectionDot =
                                    std::clamp(
                                        rigidAxisDirectionDot,
                                        0.0,
                                        1.0
                                    );

                                rigidAxisAngleRadians =
                                    std::acos(
                                        rigidAxisDirectionDot
                                    );

                                rigidAxisEndIndex =
                                    i;

                                rigidAxisMeasurementValid =
                                    std::isfinite(
                                        rigidAxisDirectionDot
                                    )
                                    && std::isfinite(
                                        rigidAxisAngleRadians
                                    )
                                    && std::isfinite(
                                        rigidOneTurnRadialClosure
                                    );
                            }

                            break;
                        }
                    }
                }
            }


            // --------------------------------------------------------
            // Acceptance.
            //
            // IMPORTANT:
            //
            // The one-turn end node is discrete, so displacement is
            // not expected to be mathematically parallel to the axis.
            //
            // Therefore use an angular tolerance appropriate to the
            // sampling test, not floating-point epsilon.
            // --------------------------------------------------------

            const double rigidAxisMinimumDot =
                0.999;

            const bool rigidAxisAccepted =
                rigidAxisMeasurementValid
                && rigidAxisDirectionDot
                >= rigidAxisMinimumDot;


            std::cout
                << "[MH1.20.10C.21H.4 RIGID HISTORY AXIS]"
                << " expectedAxis=("
                << expectedAxisDirection.x
                << ","
                << expectedAxisDirection.y
                << ","
                << expectedAxisDirection.z
                << ")"
                << " measuredAxis=("
                << measuredAxisDirection.x
                << ","
                << measuredAxisDirection.y
                << ","
                << measuredAxisDirection.z
                << ")"
                << " directionDot="
                << rigidAxisDirectionDot
                << " angleRadians="
                << rigidAxisAngleRadians
                << " radialClosure="
                << rigidOneTurnRadialClosure
                << " axialAdvance="
                << rigidOneTurnAxialAdvance
                << " endIndex="
                << rigidAxisEndIndex
                << " valid="
                << rigidAxisMeasurementValid
                << " accepted="
                << rigidAxisAccepted
                << std::endl;
        }


        // ============================================================
// MH1.20.10C.21I.2
//
// PRODUCTION LOADED-HOLD INVARIANT ACCEPTANCE
//
// Validate the REAL production formedHistoryNodes after the
// rigid-junction replacement.
//
// Tests:
//
//     I.2A  loaded radius
//     I.2B  loaded pitch
//     I.2C  loaded axis
//
// IMPORTANT:
//
//     Diagnostic only.
//     Production geometry is NOT modified here.
//
// ============================================================

        if (formedHistoryNodes.size() >= 2)
        {
            Vec3D productionAxisDirection =
                requiredSupportAxisFrame.T;

            bool productionAxisUsable =
                productionAxisDirection.lengthSquared() > 1e-12;

            if (productionAxisUsable)
            {
                productionAxisDirection =
                    productionAxisDirection.normalized();
            }


            // ========================================================
            // MH1.20.10C.21I.2A
            //
            // PRODUCTION RADIUS ACCEPTANCE
            // ========================================================

            bool productionRadiusValid =
                false;

            double productionRadiusSum =
                0.0;

            double productionMinimumRadius =
                std::numeric_limits<double>::max();

            double productionMaximumRadius =
                0.0;

            std::size_t productionRadiusSampleCount =
                0;


            if (productionAxisUsable)
            {
                for (const PipeNode& node : formedHistoryNodes)
                {
                    const Vec3D relative =
                        node.pos
                        - requiredSupportAxisFrame.P;

                    const double axialCoordinate =
                        dot(
                            relative,
                            productionAxisDirection
                        );

                    const Vec3D radialVector =
                        relative
                        - productionAxisDirection
                        * axialCoordinate;

                    const double radius =
                        radialVector.length();

                    if (!std::isfinite(radius))
                    {
                        continue;
                    }

                    productionRadiusSum +=
                        radius;

                    productionMinimumRadius =
                        std::min(
                            productionMinimumRadius,
                            radius
                        );

                    productionMaximumRadius =
                        std::max(
                            productionMaximumRadius,
                            radius
                        );

                    ++productionRadiusSampleCount;
                }


                productionRadiusValid =
                    productionRadiusSampleCount > 0;
            }


            double productionAverageRadius =
                0.0;

            double productionRadiusSpread =
                0.0;

            double productionAverageRadiusError =
                std::numeric_limits<double>::infinity();


            if (productionRadiusValid)
            {
                productionAverageRadius =
                    productionRadiusSum
                    / static_cast<double>(
                        productionRadiusSampleCount
                        );

                productionRadiusSpread =
                    productionMaximumRadius
                    - productionMinimumRadius;

                productionAverageRadiusError =
                    std::abs(
                        productionAverageRadius
                        - loadedHelixRadius
                    );
            }


            const double productionRadiusTolerance =
                0.01;

            const double productionRadiusSpreadTolerance =
                0.01;


            const bool productionRadiusAccepted =
                productionRadiusValid
                && productionAverageRadiusError
                <= productionRadiusTolerance
                && productionRadiusSpread
                <= productionRadiusSpreadTolerance;


            std::cout
                << "[MH1.20.10C.21I.2A PRODUCTION RADIUS]"
                << " theoretical="
                << loadedHelixRadius

                << " measuredAvg="
                << productionAverageRadius

                << " measuredMin="
                << productionMinimumRadius

                << " measuredMax="
                << productionMaximumRadius

                << " averageError="
                << productionAverageRadiusError

                << " spread="
                << productionRadiusSpread

                << " samples="
                << productionRadiusSampleCount

                << " valid="
                << productionRadiusValid

                << " accepted="
                << productionRadiusAccepted

                << std::endl;


            // ========================================================
            // MH1.20.10C.21I.2B
            //
            // PRODUCTION PITCH ACCEPTANCE
            //
            // Measure approximately one complete revolution from the
            // real production history.
            // ========================================================

            bool productionPitchValid =
                false;

            double productionMeasuredAngle =
                0.0;

            double productionAxialAdvance =
                0.0;

            double productionMeasuredRisePerRadian =
                0.0;

            double productionMeasuredPitch =
                0.0;

            std::size_t productionPitchEndIndex =
                0;


            if (
                productionAxisUsable
                && formedHistoryNodes.size() >= 2
                )
            {
                const double twoPi =
                    2.0 * 3.14159265358979323846;


                const Vec3D firstRelative =
                    formedHistoryNodes.front().pos
                    - requiredSupportAxisFrame.P;


                const double firstAxialCoordinate =
                    dot(
                        firstRelative,
                        productionAxisDirection
                    );


                Vec3D previousRadialDirection =
                    firstRelative
                    - productionAxisDirection
                    * firstAxialCoordinate;


                const double firstRadius =
                    previousRadialDirection.length();


                if (firstRadius > 1e-12)
                {
                    previousRadialDirection =
                        previousRadialDirection
                        / firstRadius;


                    double accumulatedAngle =
                        0.0;


                    for (
                        std::size_t i = 1;
                        i < formedHistoryNodes.size();
                        ++i
                        )
                    {
                        const Vec3D relative =
                            formedHistoryNodes[i].pos
                            - requiredSupportAxisFrame.P;


                        const double axialCoordinate =
                            dot(
                                relative,
                                productionAxisDirection
                            );


                        Vec3D radialDirection =
                            relative
                            - productionAxisDirection
                            * axialCoordinate;


                        const double radialLength =
                            radialDirection.length();


                        if (radialLength <= 1e-12)
                        {
                            continue;
                        }


                        radialDirection =
                            radialDirection
                            / radialLength;


                        const double sinAngle =
                            dot(
                                cross(
                                    previousRadialDirection,
                                    radialDirection
                                ),
                                productionAxisDirection
                            );


                        const double cosAngle =
                            std::clamp(
                                dot(
                                    previousRadialDirection,
                                    radialDirection
                                ),
                                -1.0,
                                1.0
                            );


                        const double deltaMeasuredAngle =
                            std::atan2(
                                sinAngle,
                                cosAngle
                            );


                        accumulatedAngle +=
                            deltaMeasuredAngle;


                        previousRadialDirection =
                            radialDirection;


                        if (std::abs(accumulatedAngle) >= twoPi)
                        {
                            productionMeasuredAngle =
                                accumulatedAngle;


                            productionAxialAdvance =
                                axialCoordinate
                                - firstAxialCoordinate;


                            if (
                                std::abs(productionMeasuredAngle)
                        > 1e-12
                                )
                            {
                                productionMeasuredRisePerRadian =
                                    productionAxialAdvance
                                    / productionMeasuredAngle;


                                productionMeasuredPitch =
                                    productionMeasuredRisePerRadian
                                    * twoPi;


                                productionPitchEndIndex =
                                    i;


                                productionPitchValid =
                                    std::isfinite(
                                        productionMeasuredPitch
                                    )
                                    && std::isfinite(
                                        productionMeasuredRisePerRadian
                                    );
                            }

                            break;
                        }
                    }
                }
            }


            const double productionPitchError =
                productionPitchValid
                ? std::abs(
                    std::abs(productionMeasuredPitch)
                    - std::abs(loadedHelixPitch)
                )
                : std::numeric_limits<double>::infinity();


            const double productionRiseError =
                productionPitchValid
                ? std::abs(
                    std::abs(productionMeasuredRisePerRadian)
                    - std::abs(loadedHelixRisePerRadian)
                )
                : std::numeric_limits<double>::infinity();


            const double productionPitchTolerance =
                0.01;

            const double productionRiseTolerance =
                0.01;


            const bool productionPitchAccepted =
                productionPitchValid
                && productionPitchError
                <= productionPitchTolerance
                && productionRiseError
                <= productionRiseTolerance;


            std::cout
                << "[MH1.20.10C.21I.2B PRODUCTION PITCH]"
                << " theoreticalPitch="
                << loadedHelixPitch

                << " measuredPitch="
                << productionMeasuredPitch

                << " pitchError="
                << productionPitchError

                << " theoreticalRisePerRadian="
                << loadedHelixRisePerRadian

                << " measuredRisePerRadian="
                << productionMeasuredRisePerRadian

                << " riseError="
                << productionRiseError

                << " measuredAngle="
                << productionMeasuredAngle

                << " axialAdvance="
                << productionAxialAdvance

                << " endIndex="
                << productionPitchEndIndex

                << " valid="
                << productionPitchValid

                << " accepted="
                << productionPitchAccepted

                << std::endl;


            // ========================================================
            // MH1.20.10C.21I.2C
            //
            // PRODUCTION AXIS ACCEPTANCE
            //
            // Use the same approximately-one-turn endpoint found by
            // the pitch measurement.
            //
            // The discrete endpoint may lie slightly past 2*pi, so a
            // small radial closure error is expected.
            // ========================================================

            bool productionAxisMeasurementValid =
                false;

            Vec3D productionMeasuredAxis =
            {
                0.0,
                0.0,
                0.0
            };

            double productionAxisDot =
                0.0;

            double productionAxisAngleRadians =
                0.0;

            double productionRadialClosure =
                0.0;

            double productionAxisAxialAdvance =
                0.0;


            if (
                productionPitchValid
                && productionPitchEndIndex > 0
                && productionPitchEndIndex
                < formedHistoryNodes.size()
                )
            {
                const Vec3D firstPosition =
                    formedHistoryNodes.front().pos;


                const Vec3D endPosition =
                    formedHistoryNodes[
                        productionPitchEndIndex
                    ].pos;


                const Vec3D displacement =
                    endPosition
                    - firstPosition;


                productionAxisAxialAdvance =
                    dot(
                        displacement,
                        productionAxisDirection
                    );


                const Vec3D radialClosureVector =
                    displacement
                    - productionAxisDirection
                    * productionAxisAxialAdvance;


                productionRadialClosure =
                    radialClosureVector.length();


                const double displacementLength =
                    displacement.length();


                if (displacementLength > 1e-12)
                {
                    productionMeasuredAxis =
                        displacement
                        / displacementLength;


                    productionAxisDot =
                        std::abs(
                            dot(
                                productionMeasuredAxis,
                                productionAxisDirection
                            )
                        );


                    productionAxisDot =
                        std::clamp(
                            productionAxisDot,
                            0.0,
                            1.0
                        );


                    productionAxisAngleRadians =
                        std::acos(
                            productionAxisDot
                        );


                    productionAxisMeasurementValid =
                        std::isfinite(
                            productionAxisDot
                        )
                        && std::isfinite(
                            productionAxisAngleRadians
                        )
                        && std::isfinite(
                            productionRadialClosure
                        );
                }
            }


            const double productionAxisMinimumDot =
                0.999;


            const bool productionAxisAccepted =
                productionAxisMeasurementValid
                && productionAxisDot
                >= productionAxisMinimumDot;


            std::cout
                << "[MH1.20.10C.21I.2C PRODUCTION AXIS]"

                << " expectedAxis=("
                << productionAxisDirection.x
                << ","
                << productionAxisDirection.y
                << ","
                << productionAxisDirection.z
                << ")"

                << " measuredAxis=("
                << productionMeasuredAxis.x
                << ","
                << productionMeasuredAxis.y
                << ","
                << productionMeasuredAxis.z
                << ")"

                << " directionDot="
                << productionAxisDot

                << " angleRadians="
                << productionAxisAngleRadians

                << " radialClosure="
                << productionRadialClosure

                << " axialAdvance="
                << productionAxisAxialAdvance

                << " endIndex="
                << productionPitchEndIndex

                << " valid="
                << productionAxisMeasurementValid

                << " accepted="
                << productionAxisAccepted

                << std::endl;


            // ========================================================
            // MH1.20.10C.21I.2 FINAL ACCEPTANCE
            // ========================================================

            const bool productionLoadedHoldAccepted =
                productionRadiusAccepted
                && productionPitchAccepted
                && productionAxisAccepted;


            std::cout
                << "[MH1.20.10C.21I.2 FINAL ACCEPTANCE]"

                << " radiusAccepted="
                << productionRadiusAccepted

                << " pitchAccepted="
                << productionPitchAccepted

                << " axisAccepted="
                << productionAxisAccepted

                << " accepted="
                << productionLoadedHoldAccepted

                << std::endl;
        }
        // ========================================================
        // Existing transition

        std::cout
            << "[MH1.20.10A WRAPPING SNAPSHOT]"
            << " valid="
            << mh12010WrappingSnapshotValid
            << " nodes="
            << mh12010LastWrappingNodes.size()
            << std::endl;


       

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
    // ============================================================
    // MH1.20.9
    //
    // Rebuild the COMPLETE instantaneous helix during unloading.
    //
    // IMPORTANT:
    //
    // The geometry generated in this function must be controlled
    // entirely by the instantaneous unloading state:
    //
    //     currentCurvature
    //     currentTorsion
    //     currentHelixStartFrame
    //
    // Do NOT accidentally use the original final-helix torsion,
    // loaded torsion, or original start frame here.
    //
    // This function represents:
    //
    // loaded helix
    //      ?
    // instantaneous unloading helix
    //      ?
    // final helix
    //
    // ============================================================

    currentNodes.clear();

    if (!mechanicsValid)
    {
        return false;
    }


    // ============================================================
    // Clamp the unloading fraction once.
    //
    // From this point onward use ONLY "fraction".
    //
    // fraction = 0.0  -> fully loaded geometry
    // fraction = 1.0  -> fully unloaded / final geometry
    //
    // ============================================================

    const double fraction =
        std::clamp(
            unloadingFraction,
            0.0,
            1.0
        );


    // ============================================================
    // Instantaneous curvature.
    //
    // Interpolate from:
    //
    // loadedHelixCurvature
    //          ?
    // finalHelixCurvature
    //
    // ============================================================

    const double currentCurvature =
        loadedHelixCurvature
        + (
            finalHelixCurvature
            - loadedHelixCurvature
            )
        * fraction;


    // ============================================================
    // Instantaneous torsion.
    //
    // IMPORTANT:
    //
    // This value is the authoritative torsion for the current
    // unloading geometry.
    //
    // Do NOT use kinematics.torsion below when building the
    // instantaneous unloading profile.
    //
    // ============================================================

    const double currentTorsion =
        loadedHelixTorsion
        + (
            finalHelixTorsion
            - loadedHelixTorsion
            )
        * fraction;


    // ============================================================
    // Basic validity check for the instantaneous state.
    // ============================================================

    if (!std::isfinite(currentCurvature)
        || currentCurvature <= 0.0
        || !std::isfinite(currentTorsion))
    {
        return false;
    }


    // ============================================================
    // Build the start frame that belongs specifically to the
    // CURRENT (kappa, tau) pair.
    //
    // IMPORTANT:
    //
    // This frame preserves the desired global-Z helix axis while
    // curvature and torsion change during unloading.
    //
    // This frame must later be passed to the integrator.
    //
    // ============================================================

    const Frame currentHelixStartFrame =
        buildHelixStartFrameForGlobalZAxis(
            startFrame.P,
            currentCurvature,
            currentTorsion
        );


    // ============================================================
    // MH1.20.9A
    // Instantaneous Lancret-axis validation.
    // ============================================================

    const Vec3D currentLancret =
        currentHelixStartFrame.T
        * currentTorsion
        + currentHelixStartFrame.B
        * currentCurvature;

    const Vec3D currentAxisDirection =
        currentLancret.normalized();

    const Vec3D globalZ =
    {
        0.0,
        0.0,
        1.0
    };

    const double currentAxisDot =
        dot(
            currentAxisDirection,
            globalZ
        );

    const bool axisAccepted =
        currentAxisDot >= 1.0 - 1e-12;

    std::cout
        << "[MH1.20.9 AXIS]"
        << " fraction="
        << fraction
        << " axis=("
        << currentAxisDirection.x << ", "
        << currentAxisDirection.y << ", "
        << currentAxisDirection.z
        << ")"
        << " dotZ="
        << currentAxisDot
        << " accepted="
        << axisAccepted
        << std::endl;





    // ============================================================
      // Build the INSTANTANEOUS unloading profile.
      //
      // CRITICAL:
      //
      // OLD / WRONG:
      //
      //     currentCurvature,
      //     kinematics.torsion
      //
      // NEW / CORRECT:
      //
      //     currentCurvature,
      //     currentTorsion
      //
      // The profile and diagnostics must describe the same helix.
      //
      // ============================================================

    const CurvatureTorsionProfile profile =
        ConstantCurvatureTorsionProfileBuilder::build(
            input.pipeArcLength,
            currentCurvature,
            currentTorsion
        );

    if (!profile.valid)
    {
        return false;
    }


    SpatialCurveIntegrator integrator;


    // ============================================================
    // Integrate using the CURRENT helix start frame.
    //
    // CRITICAL:
    //
    // OLD / WRONG:
    //
    //     integrator.integrate(
    //         startFrame,
    //         ...
    //     );
    //
    // NEW / CORRECT:
    //
    //     integrator.integrate(
    //         currentHelixStartFrame,
    //         ...
    //     );
    //
    // Otherwise we calculate a pitch-aware current frame but never
    // actually use it to generate the geometry.
    //
    // ============================================================

    const SpatialCurveIntegrationResult result =
        integrator.integrate(
            currentHelixStartFrame,
            profile,
            input.sampleStep
        );

    if (!result.valid
        || !result.isComplete()
        || result.nodes.size() < 2)
    {
        return false;
    }


    // ============================================================
    // The generated node set is now the actual instantaneous
    // unloading geometry.
    // ============================================================

    currentNodes =
        result.nodes;


    // ============================================================
    // Existing MH1.20.8 interpolation diagnostic.
    // ============================================================

    std::cout
        << "[MH1.20.8 UNLOADING]"
        << " fraction="
        << fraction
        << " currentKappa="
        << currentCurvature
        << " loadedKappa="
        << loadedHelixCurvature
        << " finalKappa="
        << finalHelixCurvature
        << " currentTau="
        << currentTorsion
        << " loadedTau="
        << loadedHelixTorsion
        << " finalTau="
        << finalHelixTorsion
        << std::endl;






    // ============================================================
    // MH1.20.9A
    // Theoretical instantaneous helix geometry.
    // ============================================================

    const double currentRadius =
        helixRadiusFromCurvatureTorsion(
            currentCurvature,
            currentTorsion
        );

    const double currentRisePerRadian =
        helixRisePerRadianFromCurvatureTorsion(
            currentCurvature,
            currentTorsion
        );

    const double currentPitch =
        2.0
        * 3.14159265358979323846
        * currentRisePerRadian;

    std::cout
        << "[MH1.20.9 THEORETICAL HELIX]"
        << " fraction="
        << fraction
        << " radius="
        << currentRadius
        << " risePerRadian="
        << currentRisePerRadian
        << " pitch="
        << currentPitch
        << std::endl;

    // ============================================================
      // MH1.20.9B
      // Measure the radius of the ACTUAL integrated currentNodes.
      //
      // Helix convention used by buildHelixStartFrameForGlobalZAxis:
      //
      //     N points from the pipe start toward the helix axis.
      //
      // Therefore:
      //
      //     axisPoint =
      //         startPosition
      //         + N * radius
      //
      // Example:
      //
      //     start P = (0, -500, 0)
      //     N       = (-1, 0, 0)
      //     R       = 459
      //
      //     axis P  = (-459, -500, 0)
      //
      // ============================================================

    const Vec3D currentAxisPoint =
        currentHelixStartFrame.P
        + currentHelixStartFrame.N
        * currentRadius;


    double radiusSum = 0.0;

    double radiusMin =
        std::numeric_limits<double>::max();

    double radiusMax = 0.0;

    std::size_t radiusSampleCount = 0;


    // ============================================================
    // For every integrated pipe node:
    //
    //     1. form vector from helix axis to node
    //     2. remove component parallel to helix axis
    //     3. remaining vector is radial
    //     4. its length is measured helix radius
    //
    // ============================================================

    for (const PipeNode& node : currentNodes)
    {
        const Vec3D relative = node.pos
            - currentAxisPoint;

        const double axialProjection =
            dot(
                relative,
                currentAxisDirection
            );

        const Vec3D radialVector =
            relative
            - currentAxisDirection
            * axialProjection;

        const double measuredRadius =
            radialVector.length();

        if (!std::isfinite(measuredRadius))
        {
            continue;
        }

        radiusSum += measuredRadius;

        radiusMin =
            std::min(
                radiusMin,
                measuredRadius
            );

        radiusMax =
            std::max(
                radiusMax,
                measuredRadius
            );

        ++radiusSampleCount;
    }


    // ============================================================
    // Compare actual integrated geometry with theoretical radius.
    // ============================================================

    double radiusAverage = 0.0;
    double radiusAverageError = 0.0;
    double radiusSpread = 0.0;

    bool radiusAccepted = false;

    if (radiusSampleCount > 0)
    {
        radiusAverage =
            radiusSum
            / static_cast<double>(
                radiusSampleCount
                );

        radiusAverageError =
            std::abs(
                radiusAverage
                - currentRadius
            );

        radiusSpread =
            radiusMax
            - radiusMin;


        // --------------------------------------------------------
        // These are diagnostic tolerances.
        //
        // They are intentionally much larger than the numerical
        // errors previously observed (~0.001 mm radial spread),
        // while still being small geometrically.
        // --------------------------------------------------------

        const double averageTolerance = 0.01;
        const double spreadTolerance = 0.01;

        radiusAccepted =
            radiusAverageError
            <= averageTolerance
            &&
            radiusSpread
            <= spreadTolerance;
    }


    std::cout
        << "[MH1.20.9B CURRENT RADIUS]"
        << " fraction="
        << fraction
        << " theoretical="
        << currentRadius
        << " measuredAvg="
        << radiusAverage
        << " measuredMin="
        << radiusMin
        << " measuredMax="
        << radiusMax
        << " averageError="
        << radiusAverageError
        << " spread="
        << radiusSpread
        << " samples="
        << radiusSampleCount
        << " accepted="
        << radiusAccepted
        << std::endl;
    // ============================================================
// MH1.20.9C
// Measure actual pitch from the integrated current geometry.
//
// Strategy:
//
//     1. Use the first node as angular reference.
//     2. Walk through currentNodes.
//     3. Measure accumulated angular rotation around helix axis.
//     4. Find the node closest to one complete revolution (2*pi).
//     5. Measure axial displacement between the two nodes.
//
// For an ideal helix:
//
//     axial displacement over 2*pi radians = pitch
//
// ============================================================

    bool pitchAccepted = false;

    double measuredPitch = 0.0;
    double measuredRisePerRadian = 0.0;
    double pitchError = 0.0;

    double measuredAngle = 0.0;

    std::size_t pitchEndIndex = 0;


    // ============================================================
    // Need enough geometry to measure one complete revolution.
    // ============================================================

    if (currentNodes.size() >= 2)
    {
        const Vec3D firstRelative =
            currentNodes.front().pos
            - currentAxisPoint;

        const double firstAxialProjection =
            dot(
                firstRelative,
                currentAxisDirection
            );

        const Vec3D firstRadial =
            firstRelative
            - currentAxisDirection
            * firstAxialProjection;

        const Vec3D firstRadialDirection =
            firstRadial.normalized();


        // --------------------------------------------------------
        // Previous radial direction is used to accumulate signed
        // angular increments between consecutive nodes.
        // --------------------------------------------------------

        Vec3D previousRadialDirection =
            firstRadialDirection;

        double accumulatedAngle = 0.0;

        const double twoPi =
            2.0 * 3.14159265358979323846;


        // ========================================================
        // Walk along actual geometry until approximately one full
        // revolution has been accumulated.
        // ========================================================

        for (std::size_t i = 1;
            i < currentNodes.size();
            ++i)
        {
            const Vec3D relative =
                currentNodes[i].pos
                - currentAxisPoint;

            const double axialProjection =
                dot(
                    relative,
                    currentAxisDirection
                );

            const Vec3D radial =
                relative
                - currentAxisDirection
                * axialProjection;

            const Vec3D radialDirection =
                radial.normalized();


            // ----------------------------------------------------
            // Signed angle from previous radial direction to the
            // current radial direction around the helix axis.
            //
            // atan2 gives a robust signed angular increment.
            // ----------------------------------------------------

            const Vec3D radialCross =
                cross(
                    previousRadialDirection,
                    radialDirection
                );

            const double sinAngle =
                dot(
                    radialCross,
                    currentAxisDirection
                );

            const double cosAngle =
                dot(
                    previousRadialDirection,
                    radialDirection
                );

            const double deltaAngle =
                std::atan2(
                    sinAngle,
                    cosAngle
                );

            accumulatedAngle += deltaAngle;

            previousRadialDirection =
                radialDirection;


            // ----------------------------------------------------
            // We only care about magnitude of one revolution here.
            //
            // This keeps the measurement independent of handedness.
            // ----------------------------------------------------

            if (std::abs(accumulatedAngle) >= twoPi)
            {
                pitchEndIndex = i;
                measuredAngle =
                    accumulatedAngle;

                break;
            }
        }


        // ========================================================
        // If one complete revolution was found, measure axial
        // distance between first node and selected end node.
        // ========================================================

        if (pitchEndIndex > 0
            && std::abs(measuredAngle) > 1e-12)
        {
            const Vec3D startRelative =
                currentNodes.front().pos
                - currentAxisPoint;

            const Vec3D endRelative =
                currentNodes[pitchEndIndex].pos
                - currentAxisPoint;

            const double startAxial =
                dot(
                    startRelative,
                    currentAxisDirection
                );

            const double endAxial =
                dot(
                    endRelative,
                    currentAxisDirection
                );

            const double measuredAxialAdvance =
                endAxial
                - startAxial;


            // ----------------------------------------------------
            // Because our selected node may be slightly beyond 2*pi,
            // scale the measured axial movement back to exactly
            // one revolution.
            //
            // This avoids sample-step quantization becoming the
            // dominant error.
            // ----------------------------------------------------

            measuredRisePerRadian =
                measuredAxialAdvance
                / measuredAngle;

            measuredPitch =
                measuredRisePerRadian
                * twoPi;


            pitchError =
                std::abs(
                    std::abs(measuredPitch)
                    - std::abs(currentPitch)
                );


            // ----------------------------------------------------
            // Start with a diagnostic tolerance of 0.01 length units.
            //
            // We can tighten this after seeing real numerical error.
            // ----------------------------------------------------

            const double pitchTolerance =
                0.01;

            pitchAccepted =
                pitchError
                <= pitchTolerance;
        }

        std::cout
            << "[MH1.20.9C CURRENT PITCH]"
            << " fraction="
            << fraction
            << " theoreticalPitch="
            << currentPitch
            << " measuredPitch="
            << measuredPitch
            << " theoreticalRisePerRadian="
            << currentRisePerRadian
            << " measuredRisePerRadian="
            << measuredRisePerRadian
            << " measuredAngle="
            << measuredAngle
            << " endIndex="
            << pitchEndIndex
            << " pitchError="
            << pitchError
            << " accepted="
            << pitchAccepted
            << std::endl;
    }

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

        previousSupportAxialPosition =
            state.supportAxialPosition;

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

    // ============================================================
    // MH1.20.10C.10
    //
    // Verify that material-length advancement is consistent with
    // the LOADED manufacturing helix.
    //
    // For a helix:
    //
    //     ds / dTheta = sqrt(R^2 + b^2)
    //
    // During forming we must use:
    //
    //     R = loadedHelixRadius
    //     b = loadedHelixRisePerRadian
    //
    // Diagnostic only.
    // ============================================================

    if (std::abs(deltaAngle) > 1e-12)
    {
        const double actualLengthPerRadian =
            deltaLength
            / std::abs(deltaAngle);


        const double expectedLoadedLengthPerRadian =
            std::sqrt(
                loadedHelixRadius
                * loadedHelixRadius
                +
                loadedHelixRisePerRadian
                * loadedHelixRisePerRadian
            );


        const double expectedFinalLengthPerRadian =
            std::sqrt(
                finalHelixRadius
                * finalHelixRadius
                +
                finalHelixRisePerRadian
                * finalHelixRisePerRadian
            );


        const double loadedError =
            actualLengthPerRadian
            - expectedLoadedLengthPerRadian;


        const double finalError =
            actualLengthPerRadian
            - expectedFinalLengthPerRadian;


        std::cout
            << "[MH1.20.10C.10 LENGTH PER RADIAN]"
            << " actual="
            << actualLengthPerRadian
            << " loadedExpected="
            << expectedLoadedLengthPerRadian
            << " finalExpected="
            << expectedFinalLengthPerRadian
            << " loadedError="
            << loadedError
            << " finalError="
            << finalError
            << std::endl;
    }

    // ============================================================
// MH1.20.10C.10
//
// Verify that the amount of pipe declared as "newly formed"
// is consistent with the LOADED helix machine rotation.
//
// For a helix:
//
//     ds / dTheta = sqrt(R^2 + b^2)
//
// where:
//
//     R = helix centerline radius
//     b = rise per radian
//
// During wrapping we expect:
//
//     R = loadedHelixRadius
//     b = loadedHelixRisePerRadian
//
// Therefore:
//
//     expectedDeltaLength
//         = loadedLengthPerRadian
//         * abs(deltaAngle)
//
// Diagnostic only.
// No production geometry is changed.
// ============================================================

    if (
        std::abs(deltaAngle) > 1e-12
        && std::isfinite(deltaLength)
        && std::isfinite(loadedHelixRadius)
        && std::isfinite(loadedHelixRisePerRadian)
        )
    {
        const double actualLengthPerRadian =
            deltaLength
            / std::abs(deltaAngle);


        const double loadedLengthPerRadian =
            std::sqrt(
                loadedHelixRadius
                * loadedHelixRadius
                +
                loadedHelixRisePerRadian
                * loadedHelixRisePerRadian
            );


        const double expectedLoadedDeltaLength =
            loadedLengthPerRadian
            * std::abs(deltaAngle);


        const double lengthPerRadianError =
            actualLengthPerRadian
            - loadedLengthPerRadian;


        const double deltaLengthError =
            deltaLength
            - expectedLoadedDeltaLength;


        std::cout
            << "[MH1.20.10C.10 LENGTH PER RADIAN]"

            << " deltaLength="
            << deltaLength

            << " deltaAngle="
            << deltaAngle

            << " actualLengthPerRadian="
            << actualLengthPerRadian

            << " loadedLengthPerRadian="
            << loadedLengthPerRadian

            << " lengthPerRadianError="
            << lengthPerRadianError

            << " expectedLoadedDeltaLength="
            << expectedLoadedDeltaLength

            << " deltaLengthError="
            << deltaLengthError

            << " loadedRadius="
            << loadedHelixRadius

            << " loadedRisePerRadian="
            << loadedHelixRisePerRadian

            << std::endl;

    }

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


    // ============================================================
    // VALIDATE MOTION INPUTS FIRST
    // ============================================================

    if (!std::isfinite(deltaAxialPosition))
    {
        return false;
    }

    if (supportAxisDirection.lengthSquared() < 1e-12)
    {
        return false;
    }

    supportAxisDirection =
        supportAxisDirection.normalized();


    // ============================================================
    // MH1.20.10C.18A
    // ============================================================

    Vec3D mh12010C18FrontBeforeMotion;

    bool mh12010C18FrontBeforeMotionValid = false;

    if (!formedHistoryNodes.empty())
    {
        mh12010C18FrontBeforeMotion =
            formedHistoryNodes.front().pos;

        mh12010C18FrontBeforeMotionValid =
            true;

        if (mh12010C18PreviousHistoryFrontValid)
        {
            const double persistenceGap =
                (
                    mh12010C18FrontBeforeMotion
                    - mh12010C18PreviousHistoryFrontPosition
                    ).length();

            std::cout
                << "[MH1.20.10C.18A FRONT PERSISTENCE]"
                << " gap="
                << persistenceGap
                << " historyNodes="
                << formedHistoryNodes.size()
                << std::endl;
        }
    }


    // ============================================================
// MH1.20.10C.19D
//
// Report the semantic origin carried by the history front
// entering this timestep.
//
// Diagnostic only.
// ============================================================

    if (
        mh12010C19FrontOriginValid
        && !formedHistoryNodes.empty()
        )
    {
        std::cout
            << "[MH1.20.10C.19D INCOMING HISTORY FRONT SEMANTICS]"

            << " storedSourceIndex="
            << mh12010C19FrontLocalSourceIndex

            << " storedRepresentedLocalLength="
            << mh12010C19FrontRepresentedLocalLength

            << " previousDeltaLength="
            << mh12010C19FrontActualDeltaLength

            << std::endl;
    }



    // ============================================================
    // MH1.20.10C.18B
    //
    // Predict where the old history front SHOULD move under the
    // exact same rigid support motion used by production.
    //
    // We calculate this prediction BEFORE production modifies the
    // stored nodes.
    //
    // Diagnostic only.
    // ============================================================

    Vec3D mh12010C18PredictedFrontAfterMotion;

    bool mh12010C18PredictionValid = false;

    if (
        mh12010C18FrontBeforeMotionValid
        && !formedHistoryNodes.empty()
        )
    {
        // Exact copy of the real front node.
        PipeNode diagnosticNode =
            formedHistoryNodes.front();


        if (std::abs(deltaAngle) > 1e-12)
        {
            RigidTransformUtils::
                rotateNodeAroundAxis(
                    diagnosticNode,
                    supportAxisPoint,
                    supportAxisDirection,
                    deltaAngle
                );

            diagnosticNode.pos +=
                supportAxisDirection
                * deltaAxialPosition;
        }


        mh12010C18PredictedFrontAfterMotion =
            diagnosticNode.pos;

        mh12010C18PredictionValid = true;
    }









    if (!std::isfinite(deltaAxialPosition))
    {
        return false;
    }

    if (supportAxisDirection.lengthSquared() < 1e-12)
        return false;

    supportAxisDirection =
        supportAxisDirection.normalized();

    // ============================================================
    // MH1.20.10C.21E.2
    //
    // Compare the radial error stored at the end of the PREVIOUS
    // update with the same persistent history front at the
    // beginning of THIS update, before any new rigid motion.
    //
    // Diagnostic only.
    // ============================================================

    if (
        mh12010C21EStoredFrontRadiusValid
        && !formedHistoryNodes.empty()
        )
    {
        Vec3D axisDirection =
            requiredSupportAxisFrame.T;

        if (axisDirection.lengthSquared() > 1e-12)
        {
            axisDirection =
                axisDirection.normalized();

            const Vec3D relative =
                formedHistoryNodes.front().pos
                - requiredSupportAxisFrame.P;

            const double axialCoordinate =
                dot(
                    relative,
                    axisDirection
                );

            const Vec3D radialVector =
                relative
                - axisDirection * axialCoordinate;

            const double radiusBeforeMotion =
                radialVector.length();

            const double radiusErrorBeforeMotion =
                radiusBeforeMotion
                - loadedHelixRadius;

            const double persistenceError =
                radiusErrorBeforeMotion
                - mh12010C21EStoredFrontRadiusError;

            std::cout
                << "[MH1.20.10C.21E.2 FRONT BEFORE MOTION]"
                << " storedRadiusError="
                << mh12010C21EStoredFrontRadiusError
                << " currentRadiusError="
                << radiusErrorBeforeMotion
                << " persistenceError="
                << persistenceError
                << std::endl;
        }
    }



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


    // ============================================================
 // MH1.20.10C.21H.1A
 //
 // Transport the diagnostic rigid-history geometry with the
 // same support screw motion used by production history.
 //
 // Diagnostic only.
 // ============================================================

    if (!mh12010C21HRigidHistoryNodes.empty())
    {
        if (std::abs(deltaAngle) > 1e-12)
        {
            for (PipeNode& node :
                mh12010C21HRigidHistoryNodes)
            {
                RigidTransformUtils::rotateNodeAroundAxis(
                    node,
                    supportAxisPoint,
                    supportAxisDirection,
                    deltaAngle
                );

                node.pos +=
                    supportAxisDirection
                    * deltaAxialPosition;
            }
        }
    }
    // 
    // 
    // ============================================================
    // MH1.20.10C.21E.3
    //
    // Measure whether the rigid history transport changes radius.
    //
    // Rotation about the support axis plus translation along that
    // axis should preserve radius exactly.
    //
    // Diagnostic only.
    // ============================================================

    if (!formedHistoryNodes.empty())
    {
        Vec3D axisDirection =
            requiredSupportAxisFrame.T;

        if (axisDirection.lengthSquared() > 1e-12)
        {
            axisDirection =
                axisDirection.normalized();

            const Vec3D relative =
                formedHistoryNodes.front().pos
                - requiredSupportAxisFrame.P;

            const double axialCoordinate =
                dot(
                    relative,
                    axisDirection
                );

            const Vec3D radialVector =
                relative
                - axisDirection * axialCoordinate;

            const double radiusAfterMotion =
                radialVector.length();

            const double radiusErrorAfterMotion =
                radiusAfterMotion
                - loadedHelixRadius;

            std::cout
                << "[MH1.20.10C.21E.3 FRONT AFTER MOTION]"
                << " radius="
                << radiusAfterMotion
                << " radiusError="
                << radiusErrorAfterMotion
                << std::endl;
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

    // ============================================================
    // MH1.20.10C.20A — per-update diagnostic state
    //
    // These values belong only to THIS updateFormedHistory() call.
    //
    // They are deliberately local rather than namespace/member
    // state so that a later timestep cannot accidentally reuse
    // an older prediction.
    //
    // Diagnostic only.
    // ============================================================

    double mh12010C20SemanticPhase = 0.0;

    double mh12010C20QuantizationError = 0.0;

    double mh12010C20PredictedGap = 0.0;

    bool mh12010C20PredictionValid = false;
    // ============================================================
// MH1.20.10C.12
//
// Diagnose residual junction error caused by converting the
// continuous wrapped-length advance into an integer number of
// reference-curve segments.
//
// Continuous manufacturing motion gives:
//
//     deltaLength
//
// But history growth uses:
//
//     newSegmentCount
//
// which is derived from rounded cumulative reference indices.
//
// Therefore the actual reference arc length represented by the
// new discrete increment may differ slightly from deltaLength.
//
// If this difference tracks the remaining C.8 tangential gap,
// then the residual junction error is primarily sampling /
// index quantization rather than a mechanics error.
//
// Diagnostic only.
// ============================================================

    if (
        newSegmentCount > 0
        && referenceNodes.size() >= 2
        )
    {
        // --------------------------------------------------------
        // Measure the actual arc length represented by the local
        // reference-template nodes that will be used below:
        //
        //     referenceNodes[1]
        //     ...
        //     referenceNodes[newSegmentCount]
        //
        // We deliberately measure the real sampled geometry rather
        // than assuming input.sampleStep is exact.
        // --------------------------------------------------------

        double discreteReferenceLength = 0.0;

        for (std::size_t i = 1;
            i <= newSegmentCount;
            ++i)
        {
            const Vec3D segment =
                referenceNodes[i].pos
                - referenceNodes[i - 1].pos;

            discreteReferenceLength +=
                segment.length();
        }


        const double quantizationError =
            discreteReferenceLength
            - deltaLength;
        // C.20A needs the same value later, outside this C.12 scope.
        mh12010C20QuantizationError =
            quantizationError;

        const double absoluteQuantizationError =
            std::abs(quantizationError);


        // --------------------------------------------------------
        // Also report the ideal continuous reference-index advance.
        //
        // This tells us where the continuous material front lies
        // between two discrete sampled indices.
        // --------------------------------------------------------

        const double continuousIndex =
            formedFraction
            * static_cast<double>(
                lastReferenceIndex
                );


        const double roundedIndex =
            static_cast<double>(
                targetFormedReferenceIndex
                );


        const double indexRoundingError =
            roundedIndex
            - continuousIndex;


        std::cout
            << "[MH1.20.10C.12 SAMPLE QUANTIZATION]"

            << " deltaLength="
            << deltaLength

            << " oldIndex="
            << oldFormedReferenceIndex

            << " targetIndex="
            << targetFormedReferenceIndex

            << " newSegments="
            << newSegmentCount

            << " continuousIndex="
            << continuousIndex

            << " roundedIndex="
            << roundedIndex

            << " indexRoundingError="
            << indexRoundingError

            << " discreteReferenceLength="
            << discreteReferenceLength

            << " quantizationError="
            << quantizationError

            << " absQuantizationError="
            << absoluteQuantizationError

            << std::endl;



        // ============================================================
        // MH1.20.10C.12B
        //
        // Test whether the remaining junction gap is explained by:
        //
        //     one reference segment
        //     minus
        //     current increment quantization error.
        //
        // The history representation intentionally does not store
        // referenceNodes[0] in the new increment. We want to determine
        // whether that one-sample convention explains the remaining
        // pre-correction junction offset.
        //
        // Diagnostic only.
        // ============================================================

        const double firstReferenceSegmentLength =
            (
                referenceNodes[1].pos
                - referenceNodes[0].pos
                ).length();


        const double predictedJunctionGap =
            std::abs(
                firstReferenceSegmentLength
                - quantizationError
            );


        std::cout
            << "[MH1.20.10C.12B ONE-SAMPLE PREDICTION]"

            << " firstSegmentLength="
            << firstReferenceSegmentLength

            << " quantizationError="
            << quantizationError

            << " predictedGap="
            << predictedJunctionGap

            << std::endl;


        const double representedCumulativeLength =
            (
                static_cast<double>(
                    targetFormedReferenceIndex
                    )
                /
                static_cast<double>(
                    lastReferenceIndex
                    )
                )
            * input.pipeArcLength;

        const double cumulativeLengthError =
            representedCumulativeLength
            - currentWrappedLength;

        std::cout
            << "[MH1.20.10C.12 CUMULATIVE QUANTIZATION]"
            << " wrappedLength="
            << currentWrappedLength
            << " representedLength="
            << representedCumulativeLength
            << " cumulativeError="
            << cumulativeLengthError
            << std::endl;

     


        // ============================================================
    // MH1.20.10C.20A — prediction
    // ============================================================

        if (referenceNodes.size() >= 2)
        {
            mh12010C20SemanticPhase =
                (
                    referenceNodes[1].pos
                    - referenceNodes[0].pos
                    ).length();

            mh12010C20PredictedGap =
                std::abs(
                    mh12010C20SemanticPhase
                    - mh12010C20QuantizationError
                );

            mh12010C20PredictionValid = true;
        }


    }

    // ============================================================
// MH1.20.10C.18C
//
// Compare the ACTUAL production-moved history front against
// the independently predicted rigidly transformed front.
//
// If this gap is approximately zero, the history transport is
// correct and the remaining ~0.25 mm phase originates later,
// in endpoint/topology representation.
//
// Diagnostic only.
// ============================================================

    if (
        mh12010C18PredictionValid
        && !formedHistoryNodes.empty()
        )
    {
        const Vec3D actualFrontAfterMotion =
            formedHistoryNodes.front().pos;


        const double rigidMotionGap =
            (
                actualFrontAfterMotion
                - mh12010C18PredictedFrontAfterMotion
                ).length();


        const double actualMotionDistance =
            (
                actualFrontAfterMotion
                - mh12010C18FrontBeforeMotion
                ).length();


        const double predictedMotionDistance =
            (
                mh12010C18PredictedFrontAfterMotion
                - mh12010C18FrontBeforeMotion
                ).length();


        std::cout
            << "[MH1.20.10C.18C RIGID FRONT TRANSPORT]"

            << " rigidMotionGap="
            << rigidMotionGap

            << " actualMotionDistance="
            << actualMotionDistance

            << " predictedMotionDistance="
            << predictedMotionDistance

            << " deltaAngle="
            << deltaAngle

            << " deltaAxial="
            << deltaAxialPosition

            << std::endl;
    }

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

// ============================================================
// MH1.20.10C.7
//
// Verify which analytical reference sample is being used
// when a new piece of material is added to the persistent
// forming history.
//
// Cumulative sampling says the expected reference index is:
//
//     oldFormedReferenceIndex + i
//
// The existing implementation currently uses:
//
//     i
//
// These are identical only for the first history increment.
// ============================================================

const std::size_t actualReferenceIndex =
    i;

const std::size_t expectedReferenceIndex =
    oldFormedReferenceIndex
    + i;


// Print only the first and last sample of each increment.
// Otherwise the console would receive thousands of lines.
if (
    i == 1
    || i == newSegmentCount
    )
{
    std::cout
        << "[MH1.20.10C.7 SOURCE INDEX]"
        << " oldIndex="
        << oldFormedReferenceIndex
        << " targetIndex="
        << targetFormedReferenceIndex
        << " localI="
        << i
        << " actualIndex="
        << actualReferenceIndex
        << " expectedIndex="
        << expectedReferenceIndex
        << " mismatch="
        << (
            actualReferenceIndex
            != expectedReferenceIndex
        )
        << std::endl;
}



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



    // ============================================================
    // MH1.20.10C.21H.1B
    //
    // Build a diagnostic rigidly-positioned version of the fresh
    // increment and append it into an independent shadow history.
    //
    // Production geometry is NOT modified.
    // ============================================================

    if (!newIncrementNodes.empty())
    {
        std::vector<PipeNode> rigidHistoryIncrement =
            newIncrementNodes;

        bool rigidPlacementValid = true;

        // --------------------------------------------------------
        // If diagnostic history already exists, rigidly align the
        // new increment BACK endpoint to the existing history FRONT.
        // --------------------------------------------------------

        if (!mh12010C21HRigidHistoryNodes.empty())
        {
            const PipeNode& sourceLast =
                rigidHistoryIncrement.back();

            const PipeNode& targetFirst =
                mh12010C21HRigidHistoryNodes.front();

            const Vec3D axisPoint =
                requiredSupportAxisFrame.P;

            Vec3D axisDirection =
                requiredSupportAxisFrame.T;

            if (axisDirection.lengthSquared() < 1e-12)
            {
                rigidPlacementValid = false;
            }
            else
            {
                axisDirection =
                    axisDirection.normalized();

                const Vec3D sourceRelative =
                    sourceLast.pos
                    - axisPoint;

                const Vec3D targetRelative =
                    targetFirst.pos
                    - axisPoint;

                const double sourceAxial =
                    dot(
                        sourceRelative,
                        axisDirection
                    );

                const double targetAxial =
                    dot(
                        targetRelative,
                        axisDirection
                    );

                const Vec3D sourceRadial =
                    sourceRelative
                    - axisDirection * sourceAxial;

                const Vec3D targetRadial =
                    targetRelative
                    - axisDirection * targetAxial;

                const double sourceRadius =
                    sourceRadial.length();

                const double targetRadius =
                    targetRadial.length();

                if (sourceRadius <= 1e-12
                    || targetRadius <= 1e-12)
                {
                    rigidPlacementValid = false;
                }
                else
                {
                    const Vec3D sourceRadialUnit =
                        sourceRadial / sourceRadius;

                    const Vec3D targetRadialUnit =
                        targetRadial / targetRadius;

                    const double rotationSin =
                        dot(
                            axisDirection,
                            cross(
                                sourceRadialUnit,
                                targetRadialUnit
                            )
                        );

                    const double rotationCos =
                        std::clamp(
                            dot(
                                sourceRadialUnit,
                                targetRadialUnit
                            ),
                            -1.0,
                            1.0
                        );

                    const double rotationAngle =
                        std::atan2(
                            rotationSin,
                            rotationCos
                        );

                    const double axialTranslation =
                        targetAxial
                        - sourceAxial;

                    for (PipeNode& node :
                        rigidHistoryIncrement)
                    {
                        RigidTransformUtils::rotateNodeAroundAxis(
                            node,
                            axisPoint,
                            axisDirection,
                            rotationAngle
                        );

                        node.pos +=
                            axisDirection
                            * axialTranslation;
                    }
                }
            }
        }

        // --------------------------------------------------------
        // Insert diagnostic increment using the SAME history
        // topology as production:
        //
        //     [new increment] + [older history]
        //
        // --------------------------------------------------------

        if (rigidPlacementValid)
        {
            mh12010C21HRigidHistoryNodes.insert(
                mh12010C21HRigidHistoryNodes.begin(),
                rigidHistoryIncrement.begin(),
                rigidHistoryIncrement.end()
            );

            mh12010C21HRigidHistoryValid =
                !mh12010C21HRigidHistoryNodes.empty();

            std::cout
                << "[MH1.20.10C.21H.1 RIGID HISTORY]"
                << " newIncrementNodes="
                << rigidHistoryIncrement.size()
                << " historyNodes="
                << mh12010C21HRigidHistoryNodes.size()
                << " valid="
                << mh12010C21HRigidHistoryValid
                << std::endl;
        }
    }



    // ============================================================
// MH1.20.10C.21G
//
// Compare two diagnostic-only junction constructions:
//
//   A. Existing distributed correction
//   B. Rigid screw alignment
//
// Both candidates start from the SAME fresh increment.
//
// Production newIncrementNodes is NOT modified here.
// ============================================================

    if (!newIncrementNodes.empty()
        && !formedHistoryNodes.empty())
    {
        const PipeNode& originalLast =
            newIncrementNodes.back();

        const PipeNode& targetOldFirst =
            formedHistoryNodes.front();

        // --------------------------------------------------------
        // Build independent diagnostic copies.
        // --------------------------------------------------------

        std::vector<PipeNode> distributedCandidate =
            newIncrementNodes;

        std::vector<PipeNode> rigidCandidate =
            newIncrementNodes;


        // ========================================================
        // A. DISTRIBUTED CORRECTION CANDIDATE
        // ========================================================

        const Vec3D distributedCorrection =
            targetOldFirst.pos
            - originalLast.pos;

        const std::size_t distributedCount =
            distributedCandidate.size();

        if (distributedCount > 1)
        {
            for (std::size_t i = 0;
                i < distributedCount;
                ++i)
            {
                const double fraction =
                    static_cast<double>(i + 1)
                    / static_cast<double>(distributedCount);

                distributedCandidate[i].pos +=
                    distributedCorrection
                    * fraction;
            }
        }


        // ========================================================
        // B. RIGID SCREW CANDIDATE
        //
        // Rotate about the support axis until the LAST node has
        // the same angular coordinate as oldFirst, then translate
        // axially until the axial coordinates match.
        //
        // Radius is intentionally NOT changed.
        // ========================================================

        Vec3D axisDirection =
            requiredSupportAxisFrame.T;

        const Vec3D axisPoint =
            requiredSupportAxisFrame.P;

        bool rigidValid = false;

        double rigidRotationAngle = 0.0;
        double rigidAxialTranslation = 0.0;

        if (axisDirection.lengthSquared() > 1e-12)
        {
            axisDirection =
                axisDirection.normalized();

            const Vec3D sourceRelative =
                originalLast.pos
                - axisPoint;

            const Vec3D targetRelative =
                targetOldFirst.pos
                - axisPoint;

            const double sourceAxial =
                dot(
                    sourceRelative,
                    axisDirection
                );

            const double targetAxial =
                dot(
                    targetRelative,
                    axisDirection
                );

            const Vec3D sourceRadial =
                sourceRelative
                - axisDirection * sourceAxial;

            const Vec3D targetRadial =
                targetRelative
                - axisDirection * targetAxial;

            const double sourceRadius =
                sourceRadial.length();

            const double targetRadius =
                targetRadial.length();

            if (sourceRadius > 1e-12
                && targetRadius > 1e-12)
            {
                const Vec3D sourceRadialUnit =
                    sourceRadial / sourceRadius;

                const Vec3D targetRadialUnit =
                    targetRadial / targetRadius;

                const double rotationSin =
                    dot(
                        axisDirection,
                        cross(
                            sourceRadialUnit,
                            targetRadialUnit
                        )
                    );

                const double rotationCos =
                    std::clamp(
                        dot(
                            sourceRadialUnit,
                            targetRadialUnit
                        ),
                        -1.0,
                        1.0
                    );

                rigidRotationAngle =
                    std::atan2(
                        rotationSin,
                        rotationCos
                    );

                rigidAxialTranslation =
                    targetAxial
                    - sourceAxial;

                for (PipeNode& node :
                    rigidCandidate)
                {
                    RigidTransformUtils::
                        rotateNodeAroundAxis(
                            node,
                            axisPoint,
                            axisDirection,
                            rigidRotationAngle
                        );

                    node.pos +=
                        axisDirection
                        * rigidAxialTranslation;
                }

                rigidValid = true;
            }
        }


        // ========================================================
        // COMMON MEASUREMENT HELPERS
        // ========================================================

        const auto measurePolylineLength =
            [](
                const std::vector<PipeNode>& nodes
                )
            {
                double length = 0.0;

                for (std::size_t i = 1;
                    i < nodes.size();
                    ++i)
                {
                    length +=
                        (
                            nodes[i].pos
                            - nodes[i - 1].pos
                            ).length();
                }

                return length;
            };


        const auto measureAverageRadius =
            [&](
                const std::vector<PipeNode>& nodes
                )
            {
                double sum = 0.0;

                for (const PipeNode& node :
                    nodes)
                {
                    const Vec3D relative =
                        node.pos
                        - axisPoint;

                    const double axial =
                        dot(
                            relative,
                            axisDirection
                        );

                    const Vec3D radial =
                        relative
                        - axisDirection * axial;

                    sum +=
                        radial.length();
                }

                if (nodes.empty())
                    return 0.0;

                return
                    sum
                    / static_cast<double>(
                        nodes.size()
                        );
            };


        const auto measureMaxSegmentLengthError =
            [](
                const std::vector<PipeNode>& original,
                const std::vector<PipeNode>& candidate
                )
            {
                double maxError = 0.0;

                const std::size_t count =
                    std::min(
                        original.size(),
                        candidate.size()
                    );

                for (std::size_t i = 1;
                    i < count;
                    ++i)
                {
                    const double originalLength =
                        (
                            original[i].pos
                            - original[i - 1].pos
                            ).length();

                    const double candidateLength =
                        (
                            candidate[i].pos
                            - candidate[i - 1].pos
                            ).length();

                    maxError =
                        std::max(
                            maxError,
                            std::abs(
                                candidateLength
                                - originalLength
                            )
                        );
                }

                return maxError;
            };


        // ========================================================
        // MEASURE ORIGINAL
        // ========================================================

        const double originalLength =
            measurePolylineLength(
                newIncrementNodes
            );

        const double originalAverageRadius =
            measureAverageRadius(
                newIncrementNodes
            );


        // ========================================================
        // MEASURE DISTRIBUTED CANDIDATE
        // ========================================================

        const double distributedLength =
            measurePolylineLength(
                distributedCandidate
            );

        const double distributedAverageRadius =
            measureAverageRadius(
                distributedCandidate
            );

        const double distributedLengthError =
            distributedLength
            - originalLength;

        const double distributedAverageRadiusError =
            distributedAverageRadius
            - originalAverageRadius;

        const double distributedMaxSegmentError =
            measureMaxSegmentLengthError(
                newIncrementNodes,
                distributedCandidate
            );

        const double distributedJunctionGap =
            (
                targetOldFirst.pos
                - distributedCandidate.back().pos
                ).length();

        const double distributedTangentDot =
            dot(
                distributedCandidate.back().T.normalized(),
                targetOldFirst.T.normalized()
            );


        // ========================================================
        // MEASURE RIGID CANDIDATE
        // ========================================================

        double rigidLength = 0.0;
        double rigidAverageRadius = 0.0;
        double rigidLengthError = 0.0;
        double rigidAverageRadiusError = 0.0;
        double rigidMaxSegmentError = 0.0;
        double rigidJunctionGap = 0.0;
        double rigidTangentDot = 0.0;

        if (rigidValid)
        {
            rigidLength =
                measurePolylineLength(
                    rigidCandidate
                );

            rigidAverageRadius =
                measureAverageRadius(
                    rigidCandidate
                );

            rigidLengthError =
                rigidLength
                - originalLength;

            rigidAverageRadiusError =
                rigidAverageRadius
                - originalAverageRadius;

            rigidMaxSegmentError =
                measureMaxSegmentLengthError(
                    newIncrementNodes,
                    rigidCandidate
                );

            rigidJunctionGap =
                (
                    targetOldFirst.pos
                    - rigidCandidate.back().pos
                    ).length();

            rigidTangentDot =
                dot(
                    rigidCandidate.back().T.normalized(),
                    targetOldFirst.T.normalized()
                );
        }


        // ========================================================
        // DIAGNOSTIC OUTPUT
        // ========================================================

        std::cout
            << "[MH1.20.10C.21G DISTRIBUTED VS RIGID]"

            << " originalLength="
            << originalLength

            << " distributedLengthError="
            << distributedLengthError

            << " distributedMaxSegmentError="
            << distributedMaxSegmentError

            << " distributedAverageRadiusError="
            << distributedAverageRadiusError

            << " distributedJunctionGap="
            << distributedJunctionGap

            << " distributedTangentDot="
            << distributedTangentDot

            << " rigidValid="
            << rigidValid

            << " rigidLengthError="
            << rigidLengthError

            << " rigidMaxSegmentError="
            << rigidMaxSegmentError

            << " rigidAverageRadiusError="
            << rigidAverageRadiusError

            << " rigidJunctionGap="
            << rigidJunctionGap

            << " rigidTangentDot="
            << rigidTangentDot

            << " rigidRotationAngle="
            << rigidRotationAngle

            << " rigidAxialTranslation="
            << rigidAxialTranslation

            << std::endl;
    }






    // ============================================================
    // MH1.20.10C.19A
    //
    // Inspect semantic meaning of the freshly built increment.
    //
    // Important ordering:
    //
    //     newIncrementNodes.front() -> referenceNodes[1]
    //     newIncrementNodes.back()  -> referenceNodes[newSegmentCount]
    //
    // Because the vector is inserted at formedHistoryNodes.begin(),
    // the NEW history front becomes newIncrementNodes.front(),
    // not newIncrementNodes.back().
    //
    // Diagnostic only.
    // ============================================================

    if (!newIncrementNodes.empty())
    {
        const std::size_t frontLocalSourceIndex = 1;

        const std::size_t backLocalSourceIndex =
            newSegmentCount;

        double frontRepresentedLocalLength = 0.0;

        double backRepresentedLocalLength = 0.0;


        // --------------------------------------------------------
        // Measure local arc length from referenceNodes[0]
        // to referenceNodes[1].
        // --------------------------------------------------------

        if (referenceNodes.size() >= 2)
        {
            frontRepresentedLocalLength =
                (
                    referenceNodes[1].pos
                    - referenceNodes[0].pos
                    ).length();
        }


        // --------------------------------------------------------
        // Measure local represented arc length from node 0
        // through node newSegmentCount.
        // --------------------------------------------------------

        for (
            std::size_t i = 1;
            i <= newSegmentCount;
            ++i
            )
        {
            const Vec3D segment =
                referenceNodes[i].pos
                - referenceNodes[i - 1].pos;

            backRepresentedLocalLength +=
                segment.length();
        }


        std::cout
            << "[MH1.20.10C.19A NEW INCREMENT SEMANTICS]"

            << " newSegmentCount="
            << newSegmentCount

            << " frontSourceIndex="
            << frontLocalSourceIndex

            << " backSourceIndex="
            << backLocalSourceIndex

            << " frontRepresentedLength="
            << frontRepresentedLocalLength

            << " backRepresentedLength="
            << backRepresentedLocalLength

            << " actualDeltaLength="
            << deltaLength

            << std::endl;
    }

    // ============================================================
// MH1.20.10C.19B
//
// Explicitly identify which endpoint participates in the
// history junction.
//
// The junction is NOT the new history-front node.
//
// It is:
//
//     newIncrementNodes.back()
//               ?
//     old formedHistoryNodes.front()
//
// Diagnostic only.
// ============================================================

    if (
        !newIncrementNodes.empty()
        && !formedHistoryNodes.empty()
        )
    {
        const PipeNode& newIncrementFront =
            newIncrementNodes.front();

        const PipeNode& newIncrementBack =
            newIncrementNodes.back();

        const PipeNode& oldHistoryFront =
            formedHistoryNodes.front();


        const double frontToOldHistoryGap =
            (
                oldHistoryFront.pos
                - newIncrementFront.pos
                ).length();


        const double backToOldHistoryGap =
            (
                oldHistoryFront.pos
                - newIncrementBack.pos
                ).length();


        std::cout
            << "[MH1.20.10C.19B JUNCTION SEMANTICS]"

            << " frontSourceIndex=1"

            << " backSourceIndex="
            << newSegmentCount

            << " frontToOldHistoryGap="
            << frontToOldHistoryGap

            << " backToOldHistoryGap="
            << backToOldHistoryGap

            << " junctionUsesBack=1"

            << std::endl;
    }


    // ============================================================
// MH1.20.10C.13
//
// Inspect the one-node endpoint convention.
//
// The local template is conceptually rooted at:
//
//     referenceNodes[0]
//
// but the new increment actually stores:
//
//     referenceNodes[1]
//     ...
//     referenceNodes[newSegmentCount]
//
// We want to determine which local-template point corresponds
// geometrically to the front of the previously formed history.
//
// Diagnostic only.
// No geometry is modified.
// ============================================================

    if (
        !formedHistoryNodes.empty()
        && !newIncrementNodes.empty()
        && referenceNodes.size() >= 2
        )
    {
        const PipeNode& oldHistoryFront =
            formedHistoryNodes.front();


        // --------------------------------------------------------
        // Reconstruct the LOCAL template origin in exactly the same
        // coordinate system used to build newIncrementNodes.
        //
        // Production increment construction uses:
        //
        //     formingPoint
        //       + (referenceNodes[i].pos - referenceOrigin)
        //
        // where:
        //
        //     referenceOrigin = referenceNodes.front().pos
        //
        // Therefore i == 0 maps exactly to formingPoint.
        // --------------------------------------------------------

        const Vec3D templateNode0Position =
            formingPoint
            + (
                referenceNodes[0].pos
                - referenceOrigin
                );


        const Vec3D templateNode1Position =
            formingPoint
            + (
                referenceNodes[1].pos
                - referenceOrigin
                );


        const Vec3D templateLastPosition =
            formingPoint
            + (
                referenceNodes[newSegmentCount].pos
                - referenceOrigin
                );


        // --------------------------------------------------------
        // Compare the old-history front against:
        //
        //     local template node 0
        //     local template node 1
        //     local template last node
        //
        // The third quantity should correspond closely to the
        // existing C.8 junction comparison.
        // --------------------------------------------------------

        const double gapToTemplateNode0 =
            (
                oldHistoryFront.pos
                - templateNode0Position
                ).length();


        const double gapToTemplateNode1 =
            (
                oldHistoryFront.pos
                - templateNode1Position
                ).length();


        const double gapToTemplateLast =
            (
                oldHistoryFront.pos
                - templateLastPosition
                ).length();


        // --------------------------------------------------------
        // Also measure the first template segment itself.
        // --------------------------------------------------------

        const double firstTemplateSegmentLength =
            (
                templateNode1Position
                - templateNode0Position
                ).length();


        std::cout
            << "[MH1.20.10C.13 ENDPOINT CONVENTION]"

            << " gapToNode0="
            << gapToTemplateNode0

            << " gapToNode1="
            << gapToTemplateNode1

            << " gapToLast="
            << gapToTemplateLast

            << " firstSegment="
            << firstTemplateSegmentLength

            << " newSegments="
            << newSegmentCount

            << std::endl;
    }

    // ============================================================
// MH1.20.10C.13B
//
// Inspect the sampled endpoint neighborhood at the actual
// history junction.
//
// The junction is created at:
//
//     referenceNodes[newSegmentCount]
//
// We now compare the old-history front with:
//
//     N - 1
//     N
//     N + 1
//
// This tells us whether the remaining gap is simply the result
// of choosing one discrete sample around a continuous endpoint.
//
// Diagnostic only.
// ============================================================

    if (
        !formedHistoryNodes.empty()
        && newSegmentCount > 0
        && newSegmentCount + 1 < referenceNodes.size()
        )
    {
        const PipeNode& oldHistoryFront =
            formedHistoryNodes.front();

        const std::size_t endpointIndex =
            newSegmentCount;


        const Vec3D endpointMinusOne =
            formingPoint
            + (
                referenceNodes[endpointIndex - 1].pos
                - referenceOrigin
                );

        const Vec3D endpointCurrent =
            formingPoint
            + (
                referenceNodes[endpointIndex].pos
                - referenceOrigin
                );

        const Vec3D endpointPlusOne =
            formingPoint
            + (
                referenceNodes[endpointIndex + 1].pos
                - referenceOrigin
                );


        const double gapMinusOne =
            (
                oldHistoryFront.pos
                - endpointMinusOne
                ).length();

        const double gapCurrent =
            (
                oldHistoryFront.pos
                - endpointCurrent
                ).length();

        const double gapPlusOne =
            (
                oldHistoryFront.pos
                - endpointPlusOne
                ).length();


        std::cout
            << "[MH1.20.10C.13B ENDPOINT NEIGHBORS]"

            << " endpointIndex="
            << endpointIndex

            << " gapNMinus1="
            << gapMinusOne

            << " gapN="
            << gapCurrent

            << " gapNPlus1="
            << gapPlusOne

            << std::endl;
    }
    // ============================================================
// MH1.20.10C.14
//
// Test the N+1 endpoint hypothesis WITHOUT changing production
// history topology.
//
// Current production increment:
//
//     referenceNodes[1]
//         ...
//     referenceNodes[N]
//
// where:
//
//     N = newSegmentCount
//
// C.13B showed that the previous history front can sometimes
// lie much closer to:
//
//     referenceNodes[N + 1]
//
// than to:
//
//     referenceNodes[N]
//
// We now test both candidates:
//
//     Candidate A:
//         current production endpoint N
//
//     Candidate B:
//         hypothetical endpoint N + 1
//
// For each candidate we compare:
//
//     1. junction position gap
//     2. represented reference arc length
//     3. error versus continuous deltaLength
//
// Diagnostic only.
// No production nodes are modified.
// ============================================================

    if (
        !formedHistoryNodes.empty()
        && newSegmentCount > 0
        && newSegmentCount + 1 < referenceNodes.size()
        )
    {
        const PipeNode& oldHistoryFront =
            formedHistoryNodes.front();

        const std::size_t currentEndpointIndex =
            newSegmentCount;

        const std::size_t alternativeEndpointIndex =
            newSegmentCount + 1;


        // --------------------------------------------------------
        // Reconstruct both endpoint candidates in the same local
        // coordinate system used by newIncrementNodes.
        // --------------------------------------------------------

        const Vec3D currentEndpointPosition =
            formingPoint
            + (
                referenceNodes[currentEndpointIndex].pos
                - referenceOrigin
                );

        const Vec3D alternativeEndpointPosition =
            formingPoint
            + (
                referenceNodes[alternativeEndpointIndex].pos
                - referenceOrigin
                );


        // --------------------------------------------------------
        // Junction gaps.
        // --------------------------------------------------------

        const double currentEndpointGap =
            (
                oldHistoryFront.pos
                - currentEndpointPosition
                ).length();

        const double alternativeEndpointGap =
            (
                oldHistoryFront.pos
                - alternativeEndpointPosition
                ).length();


        // --------------------------------------------------------
        // Measure the actual sampled reference length represented
        // by endpoint N.
        //
        // This should agree with the C.12
        // discreteReferenceLength value.
        // --------------------------------------------------------

        double currentRepresentedLength = 0.0;

        for (
            std::size_t i = 1;
            i <= currentEndpointIndex;
            ++i
            )
        {
            currentRepresentedLength +=
                (
                    referenceNodes[i].pos
                    - referenceNodes[i - 1].pos
                    ).length();
        }


        // --------------------------------------------------------
        // Alternative N+1 represented length.
        // --------------------------------------------------------

        double alternativeRepresentedLength =
            currentRepresentedLength;

        alternativeRepresentedLength +=
            (
                referenceNodes[alternativeEndpointIndex].pos
                - referenceNodes[currentEndpointIndex].pos
                ).length();


        // --------------------------------------------------------
        // Compare both represented lengths with the continuous
        // material advance for this timestep.
        // --------------------------------------------------------

        const double currentLengthError =
            currentRepresentedLength
            - deltaLength;

        const double alternativeLengthError =
            alternativeRepresentedLength
            - deltaLength;


        const double currentAbsoluteLengthError =
            std::abs(currentLengthError);

        const double alternativeAbsoluteLengthError =
            std::abs(alternativeLengthError);


        // --------------------------------------------------------
        // Which candidate is geometrically closer?
        // --------------------------------------------------------

        const bool alternativeHasSmallerGap =
            alternativeEndpointGap
            < currentEndpointGap;

        const bool alternativeHasSmallerLengthError =
            alternativeAbsoluteLengthError
            < currentAbsoluteLengthError;


        std::cout
            << "[MH1.20.10C.14 N+1 HYPOTHESIS]"

            << " N="
            << currentEndpointIndex

            << " gapN="
            << currentEndpointGap

            << " gapNPlus1="
            << alternativeEndpointGap

            << " lengthN="
            << currentRepresentedLength

            << " lengthNPlus1="
            << alternativeRepresentedLength

            << " deltaLength="
            << deltaLength

            << " lengthErrorN="
            << currentLengthError

            << " lengthErrorNPlus1="
            << alternativeLengthError

            << " absLengthErrorN="
            << currentAbsoluteLengthError

            << " absLengthErrorNPlus1="
            << alternativeAbsoluteLengthError

            << " nPlus1SmallerGap="
            << alternativeHasSmallerGap

            << " nPlus1SmallerLengthError="
            << alternativeHasSmallerLengthError

            << std::endl;
    }



    // ============================================================
// MH1.20.10C.15
//
// Inspect whether the OLD formed-history front is already
// carrying a one-sample endpoint offset relative to the
// cumulative bookkeeping index.
//
// Important:
// previousFormedReferenceIndex is cumulative MATERIAL/SAMPLE
// bookkeeping.
//
// formedHistoryNodes.front() is a SPATIAL history node.
//
// These are not automatically guaranteed to represent the
// exact same endpoint convention.
//
// We compare the old history front against hypothetical local
// endpoints implied by:
//
//     old index
//     old index + 1
//
// expressed RELATIVE to the current local forming template.
//
// Diagnostic only.
// No production geometry is modified.
// ============================================================

    if (
        !formedHistoryNodes.empty()
        && oldFormedReferenceIndex > 0
        && oldFormedReferenceIndex + 1 < referenceNodes.size()
        )
    {
        const PipeNode& oldHistoryFront =
            formedHistoryNodes.front();


        // --------------------------------------------------------
        // The previous timestep had already formed:
        //
        //     oldFormedReferenceIndex
        //
        // reference segments.
        //
        // The current timestep advances by:
        //
        //     deltaLength
        //
        // while the previously formed history has first been moved
        // by the current machine delta.
        //
        // Therefore, to inspect endpoint CONVENTION rather than
        // absolute cumulative placement, compare the old history
        // front against local template distances around the current
        // increment endpoint.
        //
        // Current production new increment ends at:
        //
        //     N = newSegmentCount
        //
        // If the old history front internally represents one extra
        // sample, it should align more closely with N+1.
        // --------------------------------------------------------

        const std::size_t endpointN =
            newSegmentCount;

        const std::size_t endpointNPlus1 =
            newSegmentCount + 1;


        if (endpointNPlus1 < referenceNodes.size())
        {
            const Vec3D candidateN =
                formingPoint
                + (
                    referenceNodes[endpointN].pos
                    - referenceOrigin
                    );

            const Vec3D candidateNPlus1 =
                formingPoint
                + (
                    referenceNodes[endpointNPlus1].pos
                    - referenceOrigin
                    );


            const double gapToN =
                (
                    oldHistoryFront.pos
                    - candidateN
                    ).length();

            const double gapToNPlus1 =
                (
                    oldHistoryFront.pos
                    - candidateNPlus1
                    ).length();


            // ----------------------------------------------------
            // Now inspect the bookkeeping convention itself.
            //
            // History node count tells us how many explicit nodes
            // have actually been stored.
            //
            // If one explicit endpoint is missing/implicit, we may
            // observe a stable relationship such as:
            //
            //     historyNodes == oldIndex
            //
            // instead of:
            //
            //     historyNodes == oldIndex + 1
            //
            // ----------------------------------------------------

            const std::size_t historyNodeCount =
                formedHistoryNodes.size();

            const long long countMinusOldIndex =
                static_cast<long long>(
                    historyNodeCount
                    )
                -
                static_cast<long long>(
                    oldFormedReferenceIndex
                    );


            // ----------------------------------------------------
            // Sample spacing around the candidate endpoint.
            // ----------------------------------------------------

            const double endpointSampleLength =
                (
                    referenceNodes[endpointNPlus1].pos
                    - referenceNodes[endpointN].pos
                    ).length();


            // ----------------------------------------------------
            // Classification:
            //
            // If N+1 is closer than N AND the node-count convention
            // consistently lacks the explicit +1 endpoint, that is
            // evidence that the spatial old-history front is one
            // sample ahead of the bookkeeping/storage convention.
            // ----------------------------------------------------

            const bool oldFrontPrefersNPlus1 =
                gapToNPlus1 < gapToN;


            std::cout
                << "[MH1.20.10C.15 OLD HISTORY FRONT CONVENTION]"

                << " oldIndex="
                << oldFormedReferenceIndex

                << " historyNodes="
                << historyNodeCount

                << " historyNodesMinusOldIndex="
                << countMinusOldIndex

                << " newSegments="
                << newSegmentCount

                << " gapToN="
                << gapToN

                << " gapToNPlus1="
                << gapToNPlus1

                << " endpointSampleLength="
                << endpointSampleLength

                << " prefersNPlus1="
                << oldFrontPrefersNPlus1

                << std::endl;
        }
    }

   
    // ============================================================
    // MH1.20.10C.16
    //
    // Continuous sampled-endpoint interpolation.
    //
    // Instead of forcing the new increment endpoint onto an
    // integer reference node, reconstruct the continuous endpoint
    // implied by the actual material advance:
    //
    //     deltaLength
    //
    // We walk along the LOCAL reference template until the segment
    // containing deltaLength is found, then linearly interpolate
    // inside that segment.
    //
    // This tests whether the remaining junction gap is caused by
    // discrete sample quantization rather than mechanics.
    //
    // Diagnostic only.
    // No production geometry is modified.
    // ============================================================

    if (
        !formedHistoryNodes.empty()
        && referenceNodes.size() >= 2
        && deltaLength > 0.0
        )
    {
        const PipeNode& oldHistoryFront =
            formedHistoryNodes.front();

        double accumulatedReferenceLength = 0.0;

        bool continuousEndpointFound = false;

        Vec3D continuousEndpointPosition;

        std::size_t continuousSegmentStartIndex = 0;
        std::size_t continuousSegmentEndIndex = 0;

        double interpolationFraction = 0.0;


        // --------------------------------------------------------
        // Find the reference segment that contains deltaLength.
        //
        // Example:
        //
        //     ref[366] ---- ref[367] ---- ref[368]
        //
        // If deltaLength lies between ref[367] and ref[368],
        // interpolate inside that segment instead of choosing
        // either integer endpoint.
        // --------------------------------------------------------

        for (
            std::size_t i = 1;
            i < referenceNodes.size();
            ++i
            )
        {
            const Vec3D segmentVector =
                referenceNodes[i].pos
                - referenceNodes[i - 1].pos;

            const double segmentLength =
                segmentVector.length();

            if (
                !std::isfinite(segmentLength)
                || segmentLength <= 1e-12
                )
            {
                continue;
            }

            const double nextAccumulatedLength =
                accumulatedReferenceLength
                + segmentLength;


            // ----------------------------------------------------
            // deltaLength lies inside this sampled segment.
            // ----------------------------------------------------

            if (
                deltaLength
                <= nextAccumulatedLength
                )
            {
                const double distanceIntoSegment =
                    deltaLength
                    - accumulatedReferenceLength;

                interpolationFraction =
                    distanceIntoSegment
                    / segmentLength;

                interpolationFraction =
                    std::clamp(
                        interpolationFraction,
                        0.0,
                        1.0
                    );


                const Vec3D localStart =
                    referenceNodes[i - 1].pos
                    - referenceOrigin;

                const Vec3D localEnd =
                    referenceNodes[i].pos
                    - referenceOrigin;


                const Vec3D interpolatedLocalPosition =
                    localStart
                    + (
                        localEnd
                        - localStart
                        )
                    * interpolationFraction;


                continuousEndpointPosition =
                    formingPoint
                    + interpolatedLocalPosition;


                continuousSegmentStartIndex =
                    i - 1;

                continuousSegmentEndIndex =
                    i;

                continuousEndpointFound = true;

                break;
            }


            accumulatedReferenceLength =
                nextAccumulatedLength;
        }


        if (continuousEndpointFound)
        {
            const double continuousEndpointGap =
                (
                    oldHistoryFront.pos
                    - continuousEndpointPosition
                    ).length();


            // ----------------------------------------------------
            // Compare with the currently used discrete endpoint.
            // ----------------------------------------------------

            const std::size_t discreteEndpointIndex =
                newSegmentCount;

            double discreteEndpointGap =
                0.0;

            bool discreteEndpointValid =
                discreteEndpointIndex
                < referenceNodes.size();

            if (discreteEndpointValid)
            {
                const Vec3D discreteEndpointPosition =
                    formingPoint
                    + (
                        referenceNodes[discreteEndpointIndex].pos
                        - referenceOrigin
                        );

                discreteEndpointGap =
                    (
                        oldHistoryFront.pos
                        - discreteEndpointPosition
                        ).length();
            }


            // ----------------------------------------------------
            // Diagnostic acceptance:
            //
            // We are NOT requiring zero yet.
            //
            // First we simply ask whether interpolation improves
            // the spatial junction over the integer endpoint.
            // ----------------------------------------------------

            const bool interpolationImprovesGap =
                discreteEndpointValid
                && continuousEndpointGap
                < discreteEndpointGap;


            std::cout
                << "[MH1.20.10C.16 CONTINUOUS ENDPOINT]"

                << " deltaLength="
                << deltaLength

                << " segmentStart="
                << continuousSegmentStartIndex

                << " segmentEnd="
                << continuousSegmentEndIndex

                << " interpolationFraction="
                << interpolationFraction

                << " discreteIndex="
                << discreteEndpointIndex

                << " discreteGap="
                << discreteEndpointGap

                << " continuousGap="
                << continuousEndpointGap

                << " improvesGap="
                << interpolationImprovesGap

                << std::endl;



        }

      
    }




    // ============================================================
// MH1.20.10C.17
//
// Cumulative continuous endpoint phase.
//
// C.16 showed that interpolating ONLY the current deltaLength
// leaves an almost constant ~one-sample spatial gap.
//
// Now compare the cumulative continuous formed boundary at:
//
//     previousWrappedLength
//     currentWrappedLength
//
// against the actual old-history front after machine motion.
//
// The goal is to detect a persistent endpoint phase offset
// between:
//
//     cumulative material bookkeeping
//
// and
//
//     formedHistoryNodes.front()
//
// Diagnostic only.
// No production geometry is modified.
// ============================================================

    if (
        !formedHistoryNodes.empty()
        && referenceNodes.size() >= 2
        && input.pipeArcLength > 1e-12
        )
    {
        const PipeNode& oldHistoryFront =
            formedHistoryNodes.front();

        // --------------------------------------------------------
        // Helper-style local lambda:
        //
        // Given a cumulative arc length from the beginning of the
        // reference curve, find its continuous interpolated
        // position in the reference samples.
        //
        // The returned position is still in REFERENCE coordinates.
        // --------------------------------------------------------

        auto interpolateReferenceAtArcLength =
            [&referenceNodes](
                double requestedArcLength,
                Vec3D& interpolatedPosition,
                std::size_t& segmentStartIndex,
                std::size_t& segmentEndIndex,
                double& segmentFraction
                ) -> bool
            {
                if (
                    referenceNodes.size() < 2
                    || !std::isfinite(requestedArcLength)
                    || requestedArcLength < 0.0
                    )
                {
                    return false;
                }

                double accumulatedLength = 0.0;

                for (
                    std::size_t i = 1;
                    i < referenceNodes.size();
                    ++i
                    )
                {
                    const Vec3D segmentVector =
                        referenceNodes[i].pos
                        - referenceNodes[i - 1].pos;

                    const double segmentLength =
                        segmentVector.length();

                    if (
                        !std::isfinite(segmentLength)
                        || segmentLength <= 1e-12
                        )
                    {
                        continue;
                    }

                    const double nextAccumulatedLength =
                        accumulatedLength
                        + segmentLength;

                    if (
                        requestedArcLength
                        <= nextAccumulatedLength
                        )
                    {
                        const double distanceIntoSegment =
                            requestedArcLength
                            - accumulatedLength;

                        segmentFraction =
                            distanceIntoSegment
                            / segmentLength;

                        segmentFraction =
                            std::clamp(
                                segmentFraction,
                                0.0,
                                1.0
                            );

                        interpolatedPosition =
                            referenceNodes[i - 1].pos
                            + (
                                referenceNodes[i].pos
                                - referenceNodes[i - 1].pos
                                )
                            * segmentFraction;

                        segmentStartIndex =
                            i - 1;

                        segmentEndIndex =
                            i;

                        return true;
                    }

                    accumulatedLength =
                        nextAccumulatedLength;
                }

                return false;
            };


        // --------------------------------------------------------
        // Reconstruct the continuous cumulative boundary at the
        // PREVIOUS and CURRENT wrapped lengths.
        //
        // These are material-space cumulative arc lengths.
        // --------------------------------------------------------

        Vec3D previousContinuousReferencePosition;
        Vec3D currentContinuousReferencePosition;

        std::size_t previousSegmentStart = 0;
        std::size_t previousSegmentEnd = 0;

        std::size_t currentSegmentStart = 0;
        std::size_t currentSegmentEnd = 0;

        double previousSegmentFraction = 0.0;
        double currentSegmentFraction = 0.0;


        const bool previousContinuousValid =
            interpolateReferenceAtArcLength(
                previousWrappedLength,
                previousContinuousReferencePosition,
                previousSegmentStart,
                previousSegmentEnd,
                previousSegmentFraction
            );

        const bool currentContinuousValid =
            interpolateReferenceAtArcLength(
                currentWrappedLength,
                currentContinuousReferencePosition,
                currentSegmentStart,
                currentSegmentEnd,
                currentSegmentFraction
            );


        if (
            previousContinuousValid
            && currentContinuousValid
            )
        {
            // ----------------------------------------------------
            // Continuous material advance measured directly on the
            // sampled reference.
            // ----------------------------------------------------

            const double cumulativeContinuousAdvance =
                (
                    currentContinuousReferencePosition
                    - previousContinuousReferencePosition
                    ).length();


            // ----------------------------------------------------
            // Compare the INDEX phases.
            //
            // These values show where the continuous cumulative
            // boundary lies relative to its surrounding samples.
            // ----------------------------------------------------

            const double previousContinuousIndex =
                static_cast<double>(
                    previousSegmentStart
                    )
                + previousSegmentFraction;

            const double currentContinuousIndex =
                static_cast<double>(
                    currentSegmentStart
                    )
                + currentSegmentFraction;


            const double previousBookkeepingPhase =
                static_cast<double>(
                    oldFormedReferenceIndex
                    )
                - previousContinuousIndex;

            const double currentBookkeepingPhase =
                static_cast<double>(
                    targetFormedReferenceIndex
                    )
                - currentContinuousIndex;


            // ----------------------------------------------------
            // Now inspect the spatial endpoint phase.
            //
            // The old history front has already been moved by this
            // timestep's machine motion.
            //
            // Build the CURRENT local-template continuous endpoint
            // using cumulative phase information.
            //
            // The current increment begins at formingPoint, so the
            // amount of NEW material represented continuously is:
            //
            //     currentWrappedLength
            //       - previousWrappedLength
            //
            // but the cumulative interpolation phases tell us how
            // each boundary sits between integer samples.
            //
            // The phase shift across the timestep is:
            //
            //     current phase - previous phase
            //
            // ----------------------------------------------------

            const double continuousIndexAdvance =
                currentContinuousIndex
                - previousContinuousIndex;

            const double bookkeepingIndexAdvance =
                static_cast<double>(
                    targetFormedReferenceIndex
                    - oldFormedReferenceIndex
                    );

            const double indexAdvanceError =
                bookkeepingIndexAdvance
                - continuousIndexAdvance;


            // ----------------------------------------------------
            // Convert one-sample reference spacing near the current
            // boundary into a physical magnitude for comparison.
            // ----------------------------------------------------

            double localSampleLength = 0.0;

            if (
                currentSegmentEnd
                < referenceNodes.size()
                )
            {
                localSampleLength =
                    (
                        referenceNodes[currentSegmentEnd].pos
                        - referenceNodes[currentSegmentStart].pos
                        ).length();
            }


            // ----------------------------------------------------
            // C.16 already reconstructed the continuous endpoint
            // from deltaLength.
            //
            // Rebuild the same concept here, but predict its phase
            // using the DIFFERENCE between previous and current
            // cumulative interpolation phases.
            //
            // A stable ~1-sample phase relationship should become
            // visible in these values.
            // ----------------------------------------------------

            const double phaseDifference =
                currentSegmentFraction
                - previousSegmentFraction;


            std::cout
                << "[MH1.20.10C.17 CUMULATIVE PHASE]"

                << " previousWrappedLength="
                << previousWrappedLength

                << " currentWrappedLength="
                << currentWrappedLength

                << " oldIndex="
                << oldFormedReferenceIndex

                << " targetIndex="
                << targetFormedReferenceIndex

                << " previousContinuousIndex="
                << previousContinuousIndex

                << " currentContinuousIndex="
                << currentContinuousIndex

                << " previousFraction="
                << previousSegmentFraction

                << " currentFraction="
                << currentSegmentFraction

                << " previousBookkeepingPhase="
                << previousBookkeepingPhase

                << " currentBookkeepingPhase="
                << currentBookkeepingPhase

                << " phaseDifference="
                << phaseDifference

                << " continuousIndexAdvance="
                << continuousIndexAdvance

                << " bookkeepingIndexAdvance="
                << bookkeepingIndexAdvance

                << " indexAdvanceError="
                << indexAdvanceError

                << " localSampleLength="
                << localSampleLength

                << " cumulativeContinuousAdvance="
                << cumulativeContinuousAdvance

                << std::endl;

			    
        }
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

    double mh12010C21FFrontRadiusBeforeCorrection = 0.0;
    double mh12010C21FFrontRadiusAfterCorrection = 0.0;

    bool mh12010C21FBeforeValid = false;
    bool mh12010C21FAfterValid = false;

    // ============================================================
// MH1.20.10C.21F.1
//
// Measure the radius of the fresh increment FRONT immediately
// before the production distributed junction correction.
//
// Diagnostic only.
// ============================================================

    if (!newIncrementNodes.empty())
    {
        Vec3D axisDirection =
            requiredSupportAxisFrame.T;

        if (axisDirection.lengthSquared() > 1e-12)
        {
            axisDirection =
                axisDirection.normalized();

            const Vec3D relative =
                newIncrementNodes.front().pos
                - requiredSupportAxisFrame.P;

            const double axialCoordinate =
                dot(
                    relative,
                    axisDirection
                );

            const Vec3D radialVector =
                relative
                - axisDirection * axialCoordinate;

            mh12010C21FFrontRadiusBeforeCorrection =
                radialVector.length();

            mh12010C21FBeforeValid =
                std::isfinite(
                    mh12010C21FFrontRadiusBeforeCorrection
                );

            std::cout
                << "[MH1.20.10C.21F.1 FRONT BEFORE CORRECTION]"
                << " radius="
                << mh12010C21FFrontRadiusBeforeCorrection
                << " radiusError="
                << (
                    mh12010C21FFrontRadiusBeforeCorrection
                    - loadedHelixRadius
                    )
                << " valid="
                << mh12010C21FBeforeValid
                << std::endl;
        }
    }



    if (!newIncrementNodes.empty()
        && !formedHistoryNodes.empty())
    {
        const PipeNode& newLast =
            newIncrementNodes.back();

        const PipeNode& oldFirst =
            formedHistoryNodes.front();

        // ============================================================
  // MH1.20.10C.8
  //
  // Measure the REAL junction mismatch BEFORE the distributed
  // correction modifies the new increment.
  //
  // Diagnostic only.
  // ============================================================

        const Vec3D preCorrectionDelta =
            oldFirst.pos
            - newLast.pos;

        const double preCorrectionGap =
            preCorrectionDelta.length();

        const double preCorrectionTangentDot =
            dot(
                newLast.T.normalized(),
                oldFirst.T.normalized()
            );

        // ============================================================
        // MH1.20.10C.20A — acceptance
        //
        // Compare prediction against the actual pre-correction
        // junction gap.
        // ============================================================

        if (mh12010C20PredictionValid)
        {
            const double predictionError =
                std::abs(
                    preCorrectionGap
                    - mh12010C20PredictedGap
                );

            std::cout
                << "[MH1.20.10C.20A SEMANTIC PHASE MODEL]"

                << " semanticPhase="
                << mh12010C20SemanticPhase

                << " quantizationError="
                << mh12010C20QuantizationError


                << " predictedGap="
                << mh12010C20PredictedGap

                << " measuredGap="
                << preCorrectionGap

                << " predictionError="
                << predictionError

                << std::endl;
        }
        // ============================================================
// MH1.20.10C.20B.1
//
// VECTOR SEMANTIC-PHASE HYPOTHESIS
//
// C.20A showed that the scalar junction gap is approximately:
//
//     | semanticPhase - quantizationError |
//
// Now reconstruct the corresponding point directly on the
// sampled LOCAL helix template.
//
// C.19 tells us that the persistent history front carries
// approximately one local sample of semantic phase.
//
// Therefore test:
//
//     semanticTargetArcLength
//         = deltaLength + semanticPhase
//
// Then:
//
//     predictedVector
//         = semanticTargetPosition - newLast.pos
//
// Compare that against the REAL C.8 vector:
//
//     actualVector
//         = oldFirst.pos - newLast.pos
//
// Diagnostic only.
// No production geometry is modified.
// ============================================================

        if (
            mh12010C20PredictionValid
            && referenceNodes.size() >= 2
            )
        {
            const double semanticTargetArcLength =
                deltaLength
                + mh12010C20SemanticPhase;


            bool semanticTargetFound = false;

            Vec3D semanticTargetPosition;

            std::size_t semanticSegmentStart = 0;
            std::size_t semanticSegmentEnd = 0;

            double semanticInterpolationFraction = 0.0;

            double accumulatedSemanticLength = 0.0;


            // --------------------------------------------------------
            // Walk along the LOCAL sampled reference until the
            // semantic target arc length is reached.
            //
            // This deliberately uses measured segment lengths rather
            // than assuming input.sampleStep is exactly 0.25.
            // --------------------------------------------------------

            for (
                std::size_t i = 1;
                i < referenceNodes.size();
                ++i
                )
            {
                const Vec3D segmentVector =
                    referenceNodes[i].pos
                    - referenceNodes[i - 1].pos;

                const double segmentLength =
                    segmentVector.length();


                if (
                    !std::isfinite(segmentLength)
                    || segmentLength <= 1e-12
                    )
                {
                    continue;
                }


                const double nextAccumulatedLength =
                    accumulatedSemanticLength
                    + segmentLength;


                if (
                    semanticTargetArcLength
                    <= nextAccumulatedLength
                    )
                {
                    const double distanceIntoSegment =
                        semanticTargetArcLength
                        - accumulatedSemanticLength;


                    semanticInterpolationFraction =
                        distanceIntoSegment
                        / segmentLength;


                    semanticInterpolationFraction =
                        std::clamp(
                            semanticInterpolationFraction,
                            0.0,
                            1.0
                        );


                    const Vec3D localStart =
                        referenceNodes[i - 1].pos
                        - referenceOrigin;


                    const Vec3D localEnd =
                        referenceNodes[i].pos
                        - referenceOrigin;


                    const Vec3D interpolatedLocalPosition =
                        localStart
                        + (
                            localEnd
                            - localStart
                            )
                        * semanticInterpolationFraction;


                    semanticTargetPosition =
                        formingPoint
                        + interpolatedLocalPosition;


                    semanticSegmentStart =
                        i - 1;

                    semanticSegmentEnd =
                        i;


                    semanticTargetFound = true;

                    break;
                }


                accumulatedSemanticLength =
                    nextAccumulatedLength;
            }


            if (semanticTargetFound)
            {
                // ----------------------------------------------------
                // Predicted semantic-phase correction vector.
                //
                // Starts at the SAME discrete endpoint used by
                // production:
                //
                //     newLast.pos
                //
                // and points toward the semantic continuous target.
                // ----------------------------------------------------

                const Vec3D predictedSemanticDelta =
                    semanticTargetPosition
                    - newLast.pos;


                // ----------------------------------------------------
                // REAL production mismatch measured by C.8:
                //
                //     oldFirst - newLast
                // ----------------------------------------------------

                const Vec3D actualSemanticDelta =
                    preCorrectionDelta;


                // ----------------------------------------------------
                // Vector prediction error.
                //
                // Perfect agreement would give:
                //
                //     actualSemanticDelta
                //       - predictedSemanticDelta
                //         = (0,0,0)
                // ----------------------------------------------------

                const Vec3D semanticVectorError =
                    actualSemanticDelta
                    - predictedSemanticDelta;


                const double predictedVectorLength =
                    predictedSemanticDelta.length();


                const double actualVectorLength =
                    actualSemanticDelta.length();


                const double semanticVectorErrorLength =
                    semanticVectorError.length();


                // ============================================================
                // MH1.20.10C.20B.2
                //
                // Decompose the predicted and actual junction vectors into
                // helix-local:
                //
                //     radial
                //     tangential
                //     axial
                //
                // This tests whether the semantic-phase model predicts not only
                // the total 3D vector, but also its physical direction.
                //
                // Diagnostic only.
                // No production geometry is modified.
                // ============================================================


                // ------------------------------------------------------------
                // Tangential direction
                //
                // Interpolate the tangent direction from the same reference
                // segment used for the semantic target.
                // ------------------------------------------------------------

                Vec3D semanticTangent =
                    referenceNodes[semanticSegmentStart].T
                    + (
                        referenceNodes[semanticSegmentEnd].T
                        - referenceNodes[semanticSegmentStart].T
                        )
                    * semanticInterpolationFraction;


                if (semanticTangent.lengthSquared() > 1e-12)
                {
                    semanticTangent =
                        semanticTangent.normalized();
                }


                // ------------------------------------------------------------
                // Axial direction
                //
                // The loaded helix axis is the required support axis.
                // ------------------------------------------------------------

                Vec3D semanticAxial =
                    requiredSupportAxisFrame.T;

                if (semanticAxial.lengthSquared() > 1e-12)
                {
                    semanticAxial =
                        semanticAxial.normalized();
                }


                // ------------------------------------------------------------
                // Radial direction
                //
                // For a helix:
                //
                //     radial ? tangent
                //     radial ? axis
                //
                // We construct it from:
                //
                //     radial = tangent × axis
                //
                // Sign is not important for this diagnostic as long as the same
                // basis is used for predicted and actual vectors.
                // ------------------------------------------------------------

                Vec3D semanticRadial =
                    cross(
                        semanticTangent,
                        semanticAxial
                    );

                if (semanticRadial.lengthSquared() > 1e-12)
                {
                    semanticRadial =
                        semanticRadial.normalized();
                }


                // ------------------------------------------------------------
                // Re-orthogonalize tangential direction.
                //
                // Because the interpolated tangent may contain tiny numerical
                // error, rebuild a clean tangent from the radial/axial pair.
                // ------------------------------------------------------------

                semanticTangent =
                    cross(
                        semanticAxial,
                        semanticRadial
                    );

                if (semanticTangent.lengthSquared() > 1e-12)
                {
                    semanticTangent =
                        semanticTangent.normalized();
                }


                // ------------------------------------------------------------
                // Project PREDICTED vector.
                // ------------------------------------------------------------

                const double predictedRadial =
                    dot(
                        predictedSemanticDelta,
                        semanticRadial
                    );

                const double predictedTangential =
                    dot(
                        predictedSemanticDelta,
                        semanticTangent
                    );

                const double predictedAxial =
                    dot(
                        predictedSemanticDelta,
                        semanticAxial
                    );


                // ------------------------------------------------------------
                // Project ACTUAL vector.
                // ------------------------------------------------------------

                const double actualRadial =
                    dot(
                        actualSemanticDelta,
                        semanticRadial
                    );

                const double actualTangential =
                    dot(
                        actualSemanticDelta,
                        semanticTangent
                    );

                const double actualAxial =
                    dot(
                        actualSemanticDelta,
                        semanticAxial
                    );


                // ------------------------------------------------------------
                // Component prediction errors.
                // ------------------------------------------------------------

                const double radialError =
                    actualRadial
                    - predictedRadial;

                const double tangentialError =
                    actualTangential
                    - predictedTangential;

                const double axialError =
                    actualAxial
                    - predictedAxial;


                // ------------------------------------------------------------
                // Diagnostic output.
                // ------------------------------------------------------------

                std::cout
                    << "[MH1.20.10C.20B.2 VECTOR COMPONENTS]"

                    << " predictedRadial="
                    << predictedRadial

                    << " actualRadial="
                    << actualRadial

                    << " radialError="
                    << radialError

                    << " predictedTangential="
                    << predictedTangential

                    << " actualTangential="
                    << actualTangential

                    << " tangentialError="
                    << tangentialError

                    << " predictedAxial="
                    << predictedAxial

                    << " actualAxial="
                    << actualAxial

                    << " axialError="
                    << axialError

                    << std::endl;





                std::cout
                    << "[MH1.20.10C.20B.1 VECTOR SEMANTIC MODEL]"

                    << " targetArcLength="
                    << semanticTargetArcLength

                    << " segmentStart="
                    << semanticSegmentStart

                    << " segmentEnd="
                    << semanticSegmentEnd

                    << " fraction="
                    << semanticInterpolationFraction

                    << " predictedVector=("
                    << predictedSemanticDelta.x << ", "
                    << predictedSemanticDelta.y << ", "
                    << predictedSemanticDelta.z << ")"

                    << " actualVector=("
                    << actualSemanticDelta.x << ", "
                    << actualSemanticDelta.y << ", "
                    << actualSemanticDelta.z << ")"

                    << " predictedLength="
                    << predictedVectorLength

                    << " actualLength="
                    << actualVectorLength

                    << " vectorError=("
                    << semanticVectorError.x << ", "
                    << semanticVectorError.y << ", "
                    << semanticVectorError.z << ")"

                    << " vectorErrorLength="
                    << semanticVectorErrorLength

                    << std::endl;

                // ============================================================
                // MH1.20.10C.20B.3
                //
                // VECTOR DIRECTION ACCEPTANCE
                //
                // C.20B.1 proved that the predicted semantic vector closely
                // matches the actual junction vector.
                //
                // C.20B.2 proved that both vectors have the same physical
                // radial / tangential / axial character.
                //
                // Now compare only their DIRECTIONS.
                //
                // For normalized vectors:
                //
                //     dot = +1   -> same direction
                //     dot =  0   -> perpendicular
                //     dot = -1   -> opposite direction
                //
                // Diagnostic only.
                // ============================================================

                const double predictedDirectionLength =
                    predictedSemanticDelta.length();

                const double actualDirectionLength =
                    actualSemanticDelta.length();

                bool directionComparisonValid = false;

                double directionDot = 0.0;
                double directionAngleRadians = 0.0;


                if (
                    std::isfinite(predictedDirectionLength)
                    && std::isfinite(actualDirectionLength)
                    && predictedDirectionLength > 1e-12
                    && actualDirectionLength > 1e-12
                    )
                {
                    const Vec3D predictedDirection =
                        predictedSemanticDelta
                        / predictedDirectionLength;

                    const Vec3D actualDirection =
                        actualSemanticDelta
                        / actualDirectionLength;


                    directionDot =
                        dot(
                            predictedDirection,
                            actualDirection
                        );


                    // Numerical protection against values such as:
                    //
                    //     1.0000000000000002
                    //
                    // which would make acos() invalid.
                    directionDot =
                        std::clamp(
                            directionDot,
                            -1.0,
                            1.0
                        );


                    directionAngleRadians =
                        std::acos(
                            directionDot
                        );


                    directionComparisonValid = true;
                }


                // ------------------------------------------------------------
                // For now, keep acceptance simple.
                //
                // We do NOT choose a final production tolerance yet.
                // This threshold is only for diagnostic confirmation that the
                // two vectors are effectively collinear.
                // ------------------------------------------------------------

                const bool directionAccepted =
                    directionComparisonValid
                    && directionDot >= 0.99999;


                std::cout
                    << "[MH1.20.10C.20B.3 VECTOR DIRECTION]"

                    << " valid="
                    << directionComparisonValid

                    << " dot="
                    << directionDot

                    << " angleRadians="
                    << directionAngleRadians

                    << " accepted="
                    << directionAccepted

                    << std::endl;
                // ============================================================
                // MH1.20.10C.21A
                //
                // NON-DEFORMING JUNCTION CONSTRUCTION HYPOTHESIS
                //
                // C.20 proved that the old history junction lies almost exactly
                // at the semantic continuous target on the helix.
                //
                // Instead of distributing a positional correction through all
                // new nodes, test whether the complete fresh increment can be
                // moved by ONE rigid helix screw transform:
                //
                //     1. rotate about the loaded support / helix axis
                //     2. translate along that same axis
                //
                // A rigid screw transform preserves:
                //
                //     - segment lengths
                //     - local curvature
                //     - local torsion
                //     - helix radius
                //     - internal node spacing
                //
                // Diagnostic only.
                // newIncrementNodes itself is NOT modified.
                // ============================================================

                Vec3D c21AxisDirection =
                    requiredSupportAxisFrame.T;

                const Vec3D c21AxisPoint =
                    requiredSupportAxisFrame.P;

                bool c21RigidCandidateValid = false;


                if (c21AxisDirection.lengthSquared() > 1e-12)
                {
                    c21AxisDirection =
                        c21AxisDirection.normalized();


                    // --------------------------------------------------------
                    // Source endpoint:
                    //
                    //     current discrete junction-side endpoint
                    // --------------------------------------------------------

                    const Vec3D sourceRelative =
                        newLast.pos
                        - c21AxisPoint;


                    const double sourceAxialCoordinate =
                        dot(
                            sourceRelative,
                            c21AxisDirection
                        );


                    const Vec3D sourceRadial =
                        sourceRelative
                        - c21AxisDirection
                        * sourceAxialCoordinate;


                    // --------------------------------------------------------
                    // Target endpoint:
                    //
                    //     semantic continuous target found in C.20B
                    // --------------------------------------------------------

                    const Vec3D targetRelative =
                        semanticTargetPosition
                        - c21AxisPoint;


                    const double targetAxialCoordinate =
                        dot(
                            targetRelative,
                            c21AxisDirection
                        );


                    const Vec3D targetRadial =
                        targetRelative
                        - c21AxisDirection
                        * targetAxialCoordinate;


                    if (
                        sourceRadial.lengthSquared() > 1e-12
                        && targetRadial.lengthSquared() > 1e-12
                        )
                    {
                        const Vec3D sourceRadialUnit =
                            sourceRadial.normalized();

                        const Vec3D targetRadialUnit =
                            targetRadial.normalized();


                        // ----------------------------------------------------
                        // Signed rotation required to move the discrete
                        // endpoint around the helix axis toward the semantic
                        // target.
                        //
                        // atan2 gives the sign automatically.
                        // ----------------------------------------------------

                        const double rotationSin =
                            dot(
                                c21AxisDirection,
                                cross(
                                    sourceRadialUnit,
                                    targetRadialUnit
                                )
                            );


                        const double rotationCos =
                            std::clamp(
                                dot(
                                    sourceRadialUnit,
                                    targetRadialUnit
                                ),
                                -1.0,
                                1.0
                            );


                        const double c21RotationAngle =
                            std::atan2(
                                rotationSin,
                                rotationCos
                            );


                        // ----------------------------------------------------
                        // Matching translation along the helix axis.
                        // ----------------------------------------------------

                        const double c21AxialTranslation =
                            targetAxialCoordinate
                            - sourceAxialCoordinate;


                        // ----------------------------------------------------
                        // IMPORTANT:
                        //
                        // Work on a COPY.
                        //
                        // Production newIncrementNodes remains untouched.
                        // ----------------------------------------------------

                        std::vector<PipeNode> c21RigidCandidate =
                            newIncrementNodes;


                        for (PipeNode& node : c21RigidCandidate)
                        {
                            RigidTransformUtils::rotateNodeAroundAxis(
                                node,
                                c21AxisPoint,
                                c21AxisDirection,
                                c21RotationAngle
                            );

                            node.pos +=
                                c21AxisDirection
                                * c21AxialTranslation;
                        }


                        if (!c21RigidCandidate.empty())
                        {
                            const PipeNode& candidateLast =
                                c21RigidCandidate.back();


                            // ------------------------------------------------
                            // Test 1:
                            //
                            // Did the rigid candidate reach our mathematical
                            // semantic target?
                            // ------------------------------------------------

                            const double targetGap =
                                (
                                    semanticTargetPosition
                                    - candidateLast.pos
                                    ).length();


                            // ------------------------------------------------
                            // Test 2:
                            //
                            // How close is that rigid candidate to the REAL
                            // persistent old history front?
                            //
                            // Based on C.20 we expect only the tiny remaining
                            // ~sub-micron semantic-model residual here.
                            // ------------------------------------------------

                            const double historyGap =
                                (
                                    oldFirst.pos
                                    - candidateLast.pos
                                    ).length();


                            // ------------------------------------------------
                            // Tangent continuity against real old history.
                            //
                            // rotateNodeAroundAxis() should rotate the node
                            // frame rigidly as well.
                            // ------------------------------------------------

                            double tangentDot = 0.0;

                            if (
                                candidateLast.T.lengthSquared() > 1e-12
                                && oldFirst.T.lengthSquared() > 1e-12
                                )
                            {
                                tangentDot =
                                    dot(
                                        candidateLast.T.normalized(),
                                        oldFirst.T.normalized()
                                    );
                            }
                            // ============================================================
                            // MH1.20.10C.21B
                            //
                            // RIGID-SHAPE PRESERVATION ACCEPTANCE
                            //
                            // C21.A applies one rigid screw transform to a COPY of the
                            // fresh increment.
                            //
                            // A true rigid transform must preserve:
                            //
                            //     1. total polyline length
                            //     2. every individual segment length
                            //     3. radius of every node from the helix/support axis
                            //
                            // Diagnostic only.
                            // Production geometry is still untouched.
                            // ============================================================

                            if (
                                c21RigidCandidate.size() == newIncrementNodes.size()
                                && c21RigidCandidate.size() >= 2
                                )
                            {
                                double originalTotalLength = 0.0;
                                double candidateTotalLength = 0.0;

                                double maxSegmentLengthError = 0.0;

                                double originalRadiusSum = 0.0;
                                double candidateRadiusSum = 0.0;

                                double maxRadiusError = 0.0;

                                std::size_t radiusSampleCount = 0;


                                for (
                                    std::size_t i = 0;
                                    i < newIncrementNodes.size();
                                    ++i
                                    )
                                {
                                    // ----------------------------------------------------
                                    // Radius from the support axis:
                                    //
                                    // radialVector =
                                    //     point - axisProjection(point)
                                    // ----------------------------------------------------

                                    const Vec3D originalRelative =
                                        newIncrementNodes[i].pos
                                        - c21AxisPoint;

                                    const double originalAxialCoordinate =
                                        dot(
                                            originalRelative,
                                            c21AxisDirection
                                        );

                                    const Vec3D originalRadialVector =
                                        originalRelative
                                        - c21AxisDirection
                                        * originalAxialCoordinate;

                                    const double originalRadius =
                                        originalRadialVector.length();


                                    const Vec3D candidateRelative =
                                        c21RigidCandidate[i].pos
                                        - c21AxisPoint;

                                    const double candidateAxialCoordinate =
                                        dot(
                                            candidateRelative,
                                            c21AxisDirection
                                        );

                                    const Vec3D candidateRadialVector =
                                        candidateRelative
                                        - c21AxisDirection
                                        * candidateAxialCoordinate;

                                    const double candidateRadius =
                                        candidateRadialVector.length();


                                    if (
                                        std::isfinite(originalRadius)
                                        && std::isfinite(candidateRadius)
                                        )
                                    {
                                        originalRadiusSum +=
                                            originalRadius;

                                        candidateRadiusSum +=
                                            candidateRadius;

                                        const double radiusError =
                                            std::abs(
                                                candidateRadius
                                                - originalRadius
                                            );

                                        maxRadiusError =
                                            std::max(
                                                maxRadiusError,
                                                radiusError
                                            );

                                        ++radiusSampleCount;
                                    }


                                    // ----------------------------------------------------
                                    // Segment-length preservation.
                                    // ----------------------------------------------------

                                    if (i > 0)
                                    {
                                        const double originalSegmentLength =
                                            (
                                                newIncrementNodes[i].pos
                                                - newIncrementNodes[i - 1].pos
                                                ).length();

                                        const double candidateSegmentLength =
                                            (
                                                c21RigidCandidate[i].pos
                                                - c21RigidCandidate[i - 1].pos
                                                ).length();


                                        originalTotalLength +=
                                            originalSegmentLength;

                                        candidateTotalLength +=
                                            candidateSegmentLength;


                                        const double segmentLengthError =
                                            std::abs(
                                                candidateSegmentLength
                                                - originalSegmentLength
                                            );


                                        maxSegmentLengthError =
                                            std::max(
                                                maxSegmentLengthError,
                                                segmentLengthError
                                            );
                                    }
                                }


                                const double totalLengthError =
                                    std::abs(
                                        candidateTotalLength
                                        - originalTotalLength
                                    );


                                double originalAverageRadius = 0.0;
                                double candidateAverageRadius = 0.0;
                                double averageRadiusError = 0.0;


                                if (radiusSampleCount > 0)
                                {
                                    originalAverageRadius =
                                        originalRadiusSum
                                        / static_cast<double>(
                                            radiusSampleCount
                                            );

                                    candidateAverageRadius =
                                        candidateRadiusSum
                                        / static_cast<double>(
                                            radiusSampleCount
                                            );

                                    averageRadiusError =
                                        std::abs(
                                            candidateAverageRadius
                                            - originalAverageRadius
                                        );
                                }


                                // --------------------------------------------------------
                                // Diagnostic acceptance.
                                //
                                // These tolerances are deliberately very tight because
                                // this is supposed to be a rigid transformation.
                                //
                                // We can tune them later if needed, but ideally the errors
                                // should be close to machine precision.
                                // --------------------------------------------------------

                                const bool totalLengthAccepted =
                                    totalLengthError <= 1e-9;

                                const bool segmentLengthsAccepted =
                                    maxSegmentLengthError <= 1e-9;

                                const bool radiusAccepted =
                                    maxRadiusError <= 1e-9;


                                const bool rigidShapeAccepted =
                                    totalLengthAccepted
                                    && segmentLengthsAccepted
                                    && radiusAccepted;


                                std::cout
                                    << "[MH1.20.10C.21B RIGID SHAPE]"

                                    << " originalLength="
                                    << originalTotalLength

                                    << " candidateLength="
                                    << candidateTotalLength

                                    << " totalLengthError="
                                    << totalLengthError

                                    << " maxSegmentLengthError="
                                    << maxSegmentLengthError

                                    << " originalAvgRadius="
                                    << originalAverageRadius

                                    << " candidateAvgRadius="
                                    << candidateAverageRadius

                                    << " averageRadiusError="
                                    << averageRadiusError

                                    << " maxRadiusError="
                                    << maxRadiusError

                                    << " accepted="
                                    << rigidShapeAccepted

                                    << std::endl;
                            }


                            // ============================================================
                            // MH1.20.10C.21C
                            //
                            // EXACT OLD-HISTORY JUNCTION PLACEMENT HYPOTHESIS
                            //
                            // C21A targeted the semantic continuous helix point.
                            //
                            // Now test the more direct construction:
                            //
                            //     source = newLast.pos
                            //     target = oldFirst.pos
                            //
                            // Compute ONE rigid screw transform which maps the fresh
                            // increment junction endpoint directly onto the persistent
                            // history junction.
                            //
                            // The transform consists of:
                            //
                            //     1. signed rotation about the support / helix axis
                            //     2. axial translation along that axis
                            //
                            // Apply it only to a COPY.
                            //
                            // Diagnostic only.
                            // Production newIncrementNodes remains unchanged.
                            // ============================================================

                            Vec3D c21cAxisDirection =
                                requiredSupportAxisFrame.T;

                            const Vec3D c21cAxisPoint =
                                requiredSupportAxisFrame.P;


                            if (c21cAxisDirection.lengthSquared() > 1e-12)
                            {
                                c21cAxisDirection =
                                    c21cAxisDirection.normalized();


                                // --------------------------------------------------------
                                // SOURCE:
                                // discrete junction-side endpoint of the fresh increment
                                // --------------------------------------------------------

                                const Vec3D sourceRelative =
                                    newLast.pos
                                    - c21cAxisPoint;

                                const double sourceAxialCoordinate =
                                    dot(
                                        sourceRelative,
                                        c21cAxisDirection
                                    );

                                const Vec3D sourceRadial =
                                    sourceRelative
                                    - c21cAxisDirection
                                    * sourceAxialCoordinate;


                                // --------------------------------------------------------
                                // TARGET:
                                // actual persistent history junction point
                                // --------------------------------------------------------

                                const Vec3D targetRelative =
                                    oldFirst.pos
                                    - c21cAxisPoint;

                                const double targetAxialCoordinate =
                                    dot(
                                        targetRelative,
                                        c21cAxisDirection
                                    );

                                const Vec3D targetRadial =
                                    targetRelative
                                    - c21cAxisDirection
                                    * targetAxialCoordinate;


                                if (
                                    sourceRadial.lengthSquared() > 1e-12
                                    && targetRadial.lengthSquared() > 1e-12
                                    )
                                {
                                    const Vec3D sourceRadialUnit =
                                        sourceRadial.normalized();

                                    const Vec3D targetRadialUnit =
                                        targetRadial.normalized();


                                    // ----------------------------------------------------
                                    // Signed rotation around the helix axis.
                                    // ----------------------------------------------------

                                    const double rotationSin =
                                        dot(
                                            c21cAxisDirection,
                                            cross(
                                                sourceRadialUnit,
                                                targetRadialUnit
                                            )
                                        );

                                    const double rotationCos =
                                        std::clamp(
                                            dot(
                                                sourceRadialUnit,
                                                targetRadialUnit
                                            ),
                                            -1.0,
                                            1.0
                                        );

                                    const double c21cRotationAngle =
                                        std::atan2(
                                            rotationSin,
                                            rotationCos
                                        );


                                    // ----------------------------------------------------
                                    // Axial shift needed to map source to target.
                                    // ----------------------------------------------------

                                    const double c21cAxialTranslation =
                                        targetAxialCoordinate
                                        - sourceAxialCoordinate;


                                    // ----------------------------------------------------
                                    // COPY ONLY.
                                    // ----------------------------------------------------

                                    std::vector<PipeNode> c21cCandidate =
                                        newIncrementNodes;


                                    for (PipeNode& node : c21cCandidate)
                                    {
                                        RigidTransformUtils::rotateNodeAroundAxis(
                                            node,
                                            c21cAxisPoint,
                                            c21cAxisDirection,
                                            c21cRotationAngle
                                        );

                                        node.pos +=
                                            c21cAxisDirection
                                            * c21cAxialTranslation;
                                    }


                                    if (!c21cCandidate.empty())
                                    {
                                        const PipeNode& candidateLast =
                                            c21cCandidate.back();


                                        // ------------------------------------------------
                                        // Exact endpoint closure check.
                                        // ------------------------------------------------

                                        const Vec3D endpointDelta =
                                            oldFirst.pos
                                            - candidateLast.pos;

                                        const double endpointGap =
                                            endpointDelta.length();


                                        // ------------------------------------------------
                                        // Tangent continuity.
                                        // ------------------------------------------------

                                        double tangentDot = 0.0;

                                        if (
                                            candidateLast.T.lengthSquared() > 1e-12
                                            && oldFirst.T.lengthSquared() > 1e-12
                                            )
                                        {
                                            tangentDot =
                                                dot(
                                                    candidateLast.T.normalized(),
                                                    oldFirst.T.normalized()
                                                );
                                        }


                                        // ------------------------------------------------
                                        // Radius preservation / compatibility.
                                        //
                                        // The source and target must lie at the same
                                        // radius for a perfect rigid screw mapping.
                                        // ------------------------------------------------

                                        const double sourceRadius =
                                            sourceRadial.length();

                                        const double targetRadius =
                                            targetRadial.length();

                                        const double radialCompatibilityError =
                                            std::abs(
                                                targetRadius
                                                - sourceRadius
                                            );


                                        std::cout
                                            << "[MH1.20.10C.21C EXACT HISTORY PLACEMENT]"

                                            << " rotationAngle="
                                            << c21cRotationAngle

                                            << " axialTranslation="
                                            << c21cAxialTranslation

                                            << " sourceRadius="
                                            << sourceRadius

                                            << " targetRadius="
                                            << targetRadius

                                            << " radialCompatibilityError="
                                            << radialCompatibilityError

                                            << " endpointGap="
                                            << endpointGap

                                            << " tangentDot="
                                            << tangentDot

                                            << std::endl;




                                        // ============================================================
                                        // MH1.20.10C.21D
                                        //
                                        // RADIAL RESIDUAL ORIGIN ANALYSIS
                                        //
                                        // C21C proved that the remaining endpoint gap is essentially
                                        // equal to the source/target radial incompatibility.
                                        //
                                        // Now identify WHICH geometry carries the radial deviation:
                                        //
                                        //     sourceRadius   = fresh discrete newLast
                                        //     targetRadius   = persistent oldFirst
                                        //     semanticRadius = C20 semantic continuous target
                                        //
                                        // Compare all three against the theoretical loaded helix radius.
                                        //
                                        // Diagnostic only.
                                        // ============================================================


                                        // ------------------------------------------------------------
                                        // Theoretical loaded radius.
                                        //
                                        // This should already be the authoritative loaded helix radius
                                        // used by the forming process.
                                        // ------------------------------------------------------------

                                        const double theoreticalLoadedRadius =
                                            loadedHelixRadius;


                                        // ------------------------------------------------------------
                                        // Semantic-target radius.
                                        // ------------------------------------------------------------

                                        const Vec3D semanticRelative =
                                            semanticTargetPosition
                                            - c21cAxisPoint;

                                        const double semanticAxialCoordinate =
                                            dot(
                                                semanticRelative,
                                                c21cAxisDirection
                                            );

                                        const Vec3D semanticRadialVector =
                                            semanticRelative
                                            - c21cAxisDirection
                                            * semanticAxialCoordinate;

                                        const double semanticRadius =
                                            semanticRadialVector.length();


                                        // ------------------------------------------------------------
                                        // Signed deviations from theoretical loaded radius.
                                        //
                                        // Positive:
                                        //     point lies slightly OUTSIDE ideal loaded cylinder.
                                        //
                                        // Negative:
                                        //     point lies slightly INSIDE ideal loaded cylinder.
                                        // ------------------------------------------------------------

                                        const double sourceRadiusError =
                                            sourceRadius
                                            - theoreticalLoadedRadius;

                                        const double targetRadiusError =
                                            targetRadius
                                            - theoreticalLoadedRadius;

                                        const double semanticRadiusError =
                                            semanticRadius
                                            - theoreticalLoadedRadius;


                                        // ------------------------------------------------------------
                                        // Pairwise radial differences.
                                        // ------------------------------------------------------------

                                        const double sourceToTargetRadiusDelta =
                                            targetRadius
                                            - sourceRadius;

                                        const double sourceToSemanticRadiusDelta =
                                            semanticRadius
                                            - sourceRadius;

                                        const double semanticToTargetRadiusDelta =
                                            targetRadius
                                            - semanticRadius;


                                        // ------------------------------------------------------------
                                        // Diagnostic output.
                                        // ------------------------------------------------------------

                                        std::cout
                                            << "[MH1.20.10C.21D RADIAL ORIGIN]"

                                            << " theoreticalRadius="
                                            << theoreticalLoadedRadius

                                            << " sourceRadius="
                                            << sourceRadius

                                            << " sourceError="
                                            << sourceRadiusError

                                            << " semanticRadius="
                                            << semanticRadius

                                            << " semanticError="
                                            << semanticRadiusError

                                            << " targetRadius="
                                            << targetRadius

                                            << " targetError="
                                            << targetRadiusError

                                            << " sourceToTargetDelta="
                                            << sourceToTargetRadiusDelta

                                            << " sourceToSemanticDelta="
                                            << sourceToSemanticRadiusDelta

                                            << " semanticToTargetDelta="
                                            << semanticToTargetRadiusDelta

                                            << std::endl;
                                    }
                                }
                            }
                            std::cout
                                << "[MH1.20.10C.21A RIGID SCREW CANDIDATE]"

                                << " rotationAngle="
                                << c21RotationAngle

                                << " axialTranslation="
                                << c21AxialTranslation

                                << " targetGap="
                                << targetGap

                                << " historyGap="
                                << historyGap

                                << " tangentDot="
                                << tangentDot

                                << std::endl;


                            c21RigidCandidateValid = true;
                        }
                    }
                }

            }



        }

        std::cout
            << "[MH1.20.10C.8 PRE-CORRECTION JUNCTION]"
            << " gap="
            << preCorrectionGap
            << " tangentDot="
            << preCorrectionTangentDot
            << " correction=("
            << preCorrectionDelta.x << ", "
            << preCorrectionDelta.y << ", "
            << preCorrectionDelta.z << ")"
            << std::endl;

        // ============================================================
        // MH1.20.10C.9
        //
        // Decompose the PRE-CORRECTION junction mismatch into:
        //
        //     radial
        //     circumferential/tangential
        //     axial
        //
        // components relative to the support axis.
        //
        // Diagnostic only.
        // ============================================================

        const Vec3D junctionPointRelativeToAxis =
            newLast.pos
            - supportAxisPoint;


        // Remove the axial component to obtain the radial vector.
        const double junctionAxialCoordinate =
            dot(
                junctionPointRelativeToAxis,
                supportAxisDirection
            );

        const Vec3D junctionRadialVector =
            junctionPointRelativeToAxis
            - supportAxisDirection
            * junctionAxialCoordinate;


        if (junctionRadialVector.lengthSquared() > 1e-12)
        {
            const Vec3D junctionRadialDirection =
                junctionRadialVector.normalized();


            // Tangential direction around the support cylinder.
            const Vec3D junctionTangentialDirection =
                cross(
                    supportAxisDirection,
                    junctionRadialDirection
                ).normalized();


            const double correctionRadial =
                dot(
                    preCorrectionDelta,
                    junctionRadialDirection
                );

            const double correctionTangential =
                dot(
                    preCorrectionDelta,
                    junctionTangentialDirection
                );

            const double correctionAxial =
                dot(
                    preCorrectionDelta,
                    supportAxisDirection
                );


            std::cout
                << "[MH1.20.10C.9 JUNCTION COMPONENTS]"
                << " gap="
                << preCorrectionGap

                << " radial="
                << correctionRadial

                << " tangential="
                << correctionTangential

                << " axial="
                << correctionAxial

                << " deltaAngle="
                << deltaAngle

                << " deltaAxial="
                << deltaAxialPosition

                << std::endl;
        }





        const Vec3D junctionCorrection =
            oldFirst.pos
            - newLast.pos;
        const std::size_t count =
            newIncrementNodes.size();

       // if (count > 1)
       // {
         //   for (std::size_t i = 0;
           //     i < count;
           //     ++i)
           // {
            //    const double fraction =
            //        static_cast<double>(i + 1)
            //        / static_cast<double>(count);

              //  newIncrementNodes[i].pos +=
                //    junctionCorrection
                  //  * fraction;
            //}
       // }


        // ============================================================
// MH1.20.10C.21I.1
//
// PRODUCTION RIGID-JUNCTION REPLACEMENT
//
// OLD production behavior:
//
//     newIncrementNodes[i].pos +=
//         junctionCorrection * fraction;
//
// distributed the endpoint correction through the fresh
// increment and therefore DEFORMED its helix geometry.
//
// NEW production behavior:
//
//     apply ONE rigid screw transform to the entire fresh
//     increment:
//
//         1. rotation about loaded support / helix axis
//         2. translation along the same axis
//
// This preserves:
//
//     - every segment length
//     - local curvature
//     - local torsion
//     - loaded helix radius
//     - node spacing
//
// The source endpoint is:
//
//     newIncrementNodes.back()
//
// The target endpoint is:
//
//     formedHistoryNodes.front()
//
// IMPORTANT:
//
// A rigid screw transform cannot change radius.
// Therefore a tiny radial junction gap may remain.
// C21C already proved that this residual is extremely small.
//
// ============================================================


// ------------------------------------------------------------
// Support / helix axis.
//
// Use the SAME authoritative loaded support axis that all
// previous diagnostics and rigid-shadow reconstruction use.
// ------------------------------------------------------------

        Vec3D rigidJunctionAxisDirection =
            requiredSupportAxisFrame.T;

        const Vec3D rigidJunctionAxisPoint =
            requiredSupportAxisFrame.P;


        if (rigidJunctionAxisDirection.lengthSquared() < 1e-12)
        {
            return false;
        }

        rigidJunctionAxisDirection =
            rigidJunctionAxisDirection.normalized();


        // ------------------------------------------------------------
        // SOURCE:
        //
        // Junction-side endpoint of the fresh local increment.
        // ------------------------------------------------------------

        const Vec3D rigidSourceRelative =
            newLast.pos
            - rigidJunctionAxisPoint;

        const double rigidSourceAxialCoordinate =
            dot(
                rigidSourceRelative,
                rigidJunctionAxisDirection
            );

        const Vec3D rigidSourceRadial =
            rigidSourceRelative
            - rigidJunctionAxisDirection
            * rigidSourceAxialCoordinate;


        // ------------------------------------------------------------
        // TARGET:
        //
        // Persistent front of already-formed history.
        // ------------------------------------------------------------

        const Vec3D rigidTargetRelative =
            oldFirst.pos
            - rigidJunctionAxisPoint;

        const double rigidTargetAxialCoordinate =
            dot(
                rigidTargetRelative,
                rigidJunctionAxisDirection
            );

        const Vec3D rigidTargetRadial =
            rigidTargetRelative
            - rigidJunctionAxisDirection
            * rigidTargetAxialCoordinate;


        // ------------------------------------------------------------
        // Validate radial directions.
        // ------------------------------------------------------------

        if (
            rigidSourceRadial.lengthSquared() < 1e-12
            || rigidTargetRadial.lengthSquared() < 1e-12
            )
        {
            return false;
        }


        const double rigidSourceRadius =
            rigidSourceRadial.length();

        const double rigidTargetRadius =
            rigidTargetRadial.length();


        const Vec3D rigidSourceRadialUnit =
            rigidSourceRadial
            / rigidSourceRadius;

        const Vec3D rigidTargetRadialUnit =
            rigidTargetRadial
            / rigidTargetRadius;


        // ------------------------------------------------------------
        // Signed angular correction around support axis.
        //
        // atan2:
        //
        //     sin = axis · (source × target)
        //     cos = source · target
        //
        // gives the signed rotation needed to move source radial
        // direction toward target radial direction.
        // ------------------------------------------------------------

        const double rigidRotationSin =
            dot(
                rigidJunctionAxisDirection,
                cross(
                    rigidSourceRadialUnit,
                    rigidTargetRadialUnit
                )
            );

        const double rigidRotationCos =
            std::clamp(
                dot(
                    rigidSourceRadialUnit,
                    rigidTargetRadialUnit
                ),
                -1.0,
                1.0
            );

        const double rigidJunctionRotationAngle =
            std::atan2(
                rigidRotationSin,
                rigidRotationCos
            );


        // ------------------------------------------------------------
        // Axial correction.
        //
        // Rotation handles circumferential alignment.
        // Translation handles position along helix axis.
        // ------------------------------------------------------------

        const double rigidJunctionAxialTranslation =
            rigidTargetAxialCoordinate
            - rigidSourceAxialCoordinate;


        if (
            !std::isfinite(rigidJunctionRotationAngle)
            || !std::isfinite(rigidJunctionAxialTranslation)
            )
        {
            return false;
        }


        // ------------------------------------------------------------
        // PRODUCTION CHANGE.
        //
        // Apply exactly the SAME transform to every fresh node.
        //
        // This is the critical difference from the deleted distributed
        // correction:
        //
        // OLD:
        //
        //     each node moved by a different amount
        //
        // NEW:
        //
        //     every node receives one identical rigid transform
        //
        // ------------------------------------------------------------

        for (PipeNode& node : newIncrementNodes)
        {
            RigidTransformUtils::rotateNodeAroundAxis(
                node,
                rigidJunctionAxisPoint,
                rigidJunctionAxisDirection,
                rigidJunctionRotationAngle
            );

            node.pos +=
                rigidJunctionAxisDirection
                * rigidJunctionAxialTranslation;
        }


        // ============================================================
        // MH1.20.10C.21I.1
        //
        // Immediate production acceptance diagnostic.
        //
        // Check:
        //
        //     - residual endpoint gap
        //     - radial compatibility
        //     - tangent continuity
        //
        // Do NOT force residual gap to zero.
        // A rigid screw transform preserves radius, so any tiny
        // source/target radius difference remains.
        // ============================================================

        const PipeNode& rigidCorrectedLast =
            newIncrementNodes.back();


        const double rigidProductionJunctionGap =
            (
                oldFirst.pos
                - rigidCorrectedLast.pos
                ).length();


        double rigidProductionTangentDot =
            0.0;

        if (
            rigidCorrectedLast.T.lengthSquared() > 1e-12
            && oldFirst.T.lengthSquared() > 1e-12
            )
        {
            rigidProductionTangentDot =
                dot(
                    rigidCorrectedLast.T.normalized(),
                    oldFirst.T.normalized()
                );
        }


        const double rigidProductionRadiusMismatch =
            std::abs(
                rigidTargetRadius
                - rigidSourceRadius
            );


        const bool rigidProductionJunctionValid =
            std::isfinite(rigidProductionJunctionGap)
            && std::isfinite(rigidProductionTangentDot)
            && std::isfinite(rigidProductionRadiusMismatch);


        std::cout
            << "[MH1.20.10C.21I.1 PRODUCTION RIGID JUNCTION]"

            << " rotationAngle="
            << rigidJunctionRotationAngle

            << " axialTranslation="
            << rigidJunctionAxialTranslation

            << " sourceRadius="
            << rigidSourceRadius

            << " targetRadius="
            << rigidTargetRadius

            << " radiusMismatch="
            << rigidProductionRadiusMismatch

            << " junctionGap="
            << rigidProductionJunctionGap

            << " tangentDot="
            << rigidProductionTangentDot

            << " valid="
            << rigidProductionJunctionValid

            << std::endl;



        // ============================================================
        // MH1.20.10C.21F.2
        //
        // Measure the SAME increment front after the production
        // distributed junction correction, but before insertion into
        // persistent history.
        //
        // Diagnostic only.
        // ============================================================

        if (
            mh12010C21FBeforeValid
            && !newIncrementNodes.empty()
            )
        {
            Vec3D axisDirection =
                requiredSupportAxisFrame.T;

            if (axisDirection.lengthSquared() > 1e-12)
            {
                axisDirection =
                    axisDirection.normalized();

                const Vec3D relative =
                    newIncrementNodes.front().pos
                    - requiredSupportAxisFrame.P;

                const double axialCoordinate =
                    dot(
                        relative,
                        axisDirection
                    );

                const Vec3D radialVector =
                    relative
                    - axisDirection * axialCoordinate;

                mh12010C21FFrontRadiusAfterCorrection =
                    radialVector.length();

                mh12010C21FAfterValid =
                    std::isfinite(
                        mh12010C21FFrontRadiusAfterCorrection
                    );

                const double radiusBeforeError =
                    mh12010C21FFrontRadiusBeforeCorrection
                    - loadedHelixRadius;

                const double radiusAfterError =
                    mh12010C21FFrontRadiusAfterCorrection
                    - loadedHelixRadius;

                const double radiusMutation =
                    mh12010C21FFrontRadiusAfterCorrection
                    - mh12010C21FFrontRadiusBeforeCorrection;

                std::cout
                    << "[MH1.20.10C.21F.2 FRONT AFTER CORRECTION]"
                    << " radiusBefore="
                    << mh12010C21FFrontRadiusBeforeCorrection
                    << " radiusBeforeError="
                    << radiusBeforeError
                    << " radiusAfter="
                    << mh12010C21FFrontRadiusAfterCorrection
                    << " radiusAfterError="
                    << radiusAfterError
                    << " radiusMutation="
                    << radiusMutation
                    << " valid="
                    << mh12010C21FAfterValid
                    << std::endl;
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

    // ============================================================
// MH1.20.10C.21E.1
//
// Store the radial error of the NEW persistent history front
// immediately after insertion.
//
// This tells us what radial state is born at insertion time.
//
// Diagnostic only.
// ============================================================

    if (!formedHistoryNodes.empty())
    {
        Vec3D axisDirection =
            requiredSupportAxisFrame.T;

        if (axisDirection.lengthSquared() > 1e-12)
        {
            axisDirection =
                axisDirection.normalized();

            const Vec3D relative =
                formedHistoryNodes.front().pos
                - requiredSupportAxisFrame.P;

            const double axialCoordinate =
                dot(
                    relative,
                    axisDirection
                );

            const Vec3D radialVector =
                relative
                - axisDirection * axialCoordinate;

            const double radius =
                radialVector.length();

            mh12010C21EStoredFrontRadiusError =
                radius - loadedHelixRadius;

            mh12010C21EStoredFrontRadiusValid =
                std::isfinite(
                    mh12010C21EStoredFrontRadiusError
                );

            std::cout
                << "[MH1.20.10C.21E.1 FRONT AFTER INSERT]"
                << " radius="
                << radius
                << " radiusError="
                << mh12010C21EStoredFrontRadiusError
                << " valid="
                << mh12010C21EStoredFrontRadiusValid
                << std::endl;
        }
    }

    // ============================================================
    // MH1.20.10C.19C
    //
    // Record the semantic origin of the node that has NOW become
    // formedHistoryNodes.front().
    //
    // Because insertion preserves newIncrementNodes ordering,
    // this should always correspond to referenceNodes[1].
    //
    // Diagnostic only.
    // ============================================================



    if (!formedHistoryNodes.empty())
    {

       



        mh12010C19FrontLocalSourceIndex = 1;

        mh12010C19FrontRepresentedLocalLength =
            (
                referenceNodes[1].pos
                - referenceNodes[0].pos
                ).length();

        mh12010C19FrontActualDeltaLength =
            deltaLength;

        mh12010C19FrontOriginValid =
            true;


        std::cout
            << "[MH1.20.10C.19C NEW HISTORY FRONT ORIGIN]"

            << " sourceIndex="
            << mh12010C19FrontLocalSourceIndex

            << " representedLocalLength="
            << mh12010C19FrontRepresentedLocalLength

            << " timestepDeltaLength="
            << mh12010C19FrontActualDeltaLength

            << " historyNodes="
            << formedHistoryNodes.size()

            << std::endl;
    }


    // ============================================================
// MH1.20.10C.18D
//
// Store the newly established history front so that the next
// call can verify persistence and rigid transport.
//
// Diagnostic only.
// ============================================================

    if (!formedHistoryNodes.empty())
    {
        mh12010C18PreviousHistoryFrontPosition =
            formedHistoryNodes.front().pos;

        mh12010C18PreviousHistoryFrontValid =
            true;


        std::cout
            << "[MH1.20.10C.18D FRONT SNAPSHOT]"

            << " historyNodes="
            << formedHistoryNodes.size()

            << " front=("
            << mh12010C18PreviousHistoryFrontPosition.x
            << ", "
            << mh12010C18PreviousHistoryFrontPosition.y
            << ", "
            << mh12010C18PreviousHistoryFrontPosition.z
            << ")"

            << std::endl;
    } 


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


