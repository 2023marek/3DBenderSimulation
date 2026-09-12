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

    // ============================================================
// APPEND INCOMING / UNFORMED GEOMETRY
// ============================================================

    if (!appendIncomingGeometry(currentNodes))
    {
        return false;
    }
    // ============================================================
    // APPEND FORMED HISTORY
    // ============================================================

    if (!appendFormedHistory(currentNodes))
    {
        return false;
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

           

            break;
        }
    }
       
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

        previousSupportAxialPosition =
            state.supportAxialPosition;

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

    
     if (!newIncrementNodes.empty()
        && !formedHistoryNodes.empty())
    {
        const PipeNode& newLast =
            newIncrementNodes.back();

        const PipeNode& oldFirst =
            formedHistoryNodes.front();

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


