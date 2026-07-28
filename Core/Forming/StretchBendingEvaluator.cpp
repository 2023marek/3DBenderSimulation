#include "Core/Forming/StretchBendingEvaluator.h"

#include <cmath>

StretchBendingEvaluationResult
StretchBendingEvaluator::evaluate(
    const StretchBendingProcessInput& input
) const
{
    StretchBendingEvaluationResult result;

    // =====================================================
    // 1. INPUT CLASSIFICATION
    //
    // Keep detailed invalid states rather than returning
    // only a generic InvalidInput status.
    // =====================================================

    if (!input.enabled)
    {
        result.status =
            StretchBendingEvaluationStatus::Disabled;

        return result;
    }

    if (!input.pipeSection.isValid())
    {
        result.status =
            StretchBendingEvaluationStatus::
            InvalidPipeSection;

        return result;
    }

    if (!input.material.isValid())
    {
        result.status =
            StretchBendingEvaluationStatus::
            InvalidMaterial;

        return result;
    }

    if (!input.geometry.isValid())
    {
        result.status =
            StretchBendingEvaluationStatus::
            InvalidGeometry;

        return result;
    }

    if (!input.isValid())
    {
        result.status =
            StretchBendingEvaluationStatus::
            InvalidInput;

        return result;
    }

    result.inputValid =
        true;

    // =====================================================
    // 2. SECTION PROPERTIES
    // =====================================================

    result.innerDiameter =
        input.pipeSection.innerDiameter();

    result.area =
        input.pipeSection.area();

    result.secondMomentArea =
        input.pipeSection.secondMomentArea();

    // =====================================================
    // 3. MATERIAL AND TARGET GEOMETRY
    // =====================================================

    result.yieldStrain =
        input.material.yieldStrain();

    result.targetArcLength =
        input.geometry.targetArcLength;

    result.targetCurvature =
        input.geometry.targetCurvature;

    result.targetTorsion =
        input.geometry.targetTorsion;

    result.finalTargetCurvature =
        input.geometry.targetCurvature;

    // =====================================================
// SPRINGBACK COMMAND
//
// targetCurvature is interpreted as the required final
// curvature after unloading.
//
// If compensation is enabled:
//
//     kappa_load = kappa_target / (1 - ratio)
//
// Otherwise:
//
//     kappa_load = kappa_target
// =====================================================

    result.finalTargetCurvature =
        input.geometry.targetCurvature;

    result.springbackRatio =
        input.springbackRatio;

    result.springbackCompensationApplied =
        input.compensateSpringback
        && input.springbackRatio > 0.0;

    if (result.springbackCompensationApplied)
    {
        const double retainedCurvatureFactor =
            1.0 - input.springbackRatio;

        if (retainedCurvatureFactor <= 1e-12)
        {
            result.status =
                StretchBendingEvaluationStatus::
                NumericalFailure;

            return result;
        }

        result.loadedCurvatureCommand =
            result.finalTargetCurvature
            / retainedCurvatureFactor;
    }
    else
    {
        result.loadedCurvatureCommand =
            result.finalTargetCurvature;
    }

    result.predictedFinalCurvature =
        result.loadedCurvatureCommand
        * (
            1.0
            - result.springbackRatio
            );

    result.finalCurvatureError =
        result.predictedFinalCurvature
        - result.finalTargetCurvature;

    result.springbackPredictionValid =
        std::isfinite(
            result.loadedCurvatureCommand
        )
        && std::isfinite(
            result.predictedFinalCurvature
        )
        && std::isfinite(
            result.finalCurvatureError
        );

    result.axialStretchStrain =
        input.axialStretchStrain;

    // =====================================================
    // 4. YIELD CURVATURE
    //
    // For a round tube:
    //
    //     epsilon_b = kappa * D / 2
    //
    // First outer-fiber yield without axial stretch:
    //
    //     epsilon_y = kappa_y * D / 2
    //
    // Therefore:
    //
    //     kappa_y = 2 * epsilon_y / D
    // =====================================================

    result.yieldCurvature =
        2.0
        * result.yieldStrain
        / input.pipeSection.outerDiameter;

    // =====================================================
    // 5. BENDING AND WALL STRAINS
    //
    // epsilon_b:
    //     strain magnitude created by curvature at the
    //     outside radius D/2.
    //
    // epsilon_outer = epsilon_0 + epsilon_b
    // epsilon_inner = epsilon_0 - epsilon_b
    // =====================================================

    result.loadedBendingStrain =
        result.loadedCurvatureCommand
        * input.pipeSection.outerDiameter
        / 2.0;

    result.bendingStrain =
        result.loadedBendingStrain;

    result.outerWallStrain =
        result.axialStretchStrain
        + result.bendingStrain;

    result.innerWallStrain =
        result.axialStretchStrain
        - result.bendingStrain;

    // =====================================================
    // 6. ALLOWED AXIAL-STRETCH RANGE
    //
    // To avoid inner-wall compression:
    //
    //     epsilon_0 >= epsilon_b
    //
    // To avoid exceeding outer allowable strain:
    //
    //     epsilon_0 <= epsilon_allow - epsilon_b
    // =====================================================

    result.minimumRequiredAxialStrain =
        result.bendingStrain;

    result.maximumAllowedAxialStrain =
        input.material.allowableStrain
        - result.bendingStrain;

    result.geometryFeasible =
        result.minimumRequiredAxialStrain
        <= result.maximumAllowedAxialStrain;


    // =====================================================
    // RECOMMENDED AXIAL-STRAIN COMMAND
    //
    // The first control strategy uses the midpoint of the
    // feasible strain interval.
    //
    // This gives equal strain margin toward:
    //
    //     inner-wall compression limit
    //     outer-wall allowable-strain limit
    //
    // It is a simple initial command, not yet an optimized
    // production-machine control law.
    // =====================================================

    if (result.geometryFeasible)
    {
        result.axialStrainRange =
            result.maximumAllowedAxialStrain
            - result.minimumRequiredAxialStrain;

        result.recommendedAxialStrain =
            0.5
            * (
                result.minimumRequiredAxialStrain
                + result.maximumAllowedAxialStrain
                );

        constexpr double STRAIN_RANGE_TOLERANCE =
            1e-12;

        result.commandedStrainInsideRecommendedRange =
            result.axialStretchStrain
            >= result.minimumRequiredAxialStrain
            - STRAIN_RANGE_TOLERANCE
            && result.axialStretchStrain
            <= result.maximumAllowedAxialStrain
            + STRAIN_RANGE_TOLERANCE;
    }
    else
    {
        result.axialStrainRange =
            0.0;

        result.recommendedAxialStrain =
            0.0;

        result.commandedStrainInsideRecommendedRange =
            false;
    }  

    // =====================================================
    // 7. FORCE AND MOMENT
    //
    // T = E A epsilon_0
    //
    // M = E I kappa
    //
    // These are elastic reference values.
    // A later phase will introduce elastic-plastic and
    // springback corrections.
    // =====================================================

    // =====================================================
// AXIAL TENSION COMMANDS
//
// Linear elastic reference:
//
//     T = E * A * epsilon_0
//
// These values are initial machine-command estimates.
// Later elastic-plastic and control-limit models may
// modify them.
// =====================================================

    const double axialRigidity =
        input.material.youngModulus
        * result.area;

    result.commandedTension =
        axialRigidity
        * result.axialStretchStrain;

    // Keep the old field synchronized for compatibility.
    result.axialTension =
        result.commandedTension;

    if (result.geometryFeasible)
    {
        result.minimumRequiredTension =
            axialRigidity
            * result.minimumRequiredAxialStrain;

        result.maximumAllowedTension =
            axialRigidity
            * result.maximumAllowedAxialStrain;

        result.recommendedTension =
            axialRigidity
            * result.recommendedAxialStrain;
    }

    result.elasticBendingMoment =
        input.material.youngModulus
        * result.secondMomentArea
        * result.loadedCurvatureCommand;

    // =====================================================
    // 8. SAFETY MARGINS
    //
    // Positive inner margin:
    //     inner wall is in non-compressive strain.
    //
    // Positive outer margin:
    //     allowable strain remains available.
    // =====================================================

    result.innerCompressionMargin =
        result.innerWallStrain;

    result.outerStrainMargin =
        input.material.allowableStrain
        - result.outerWallStrain;

    result.innerWallSafe =
        result.innerWallStrain >= 0.0;

    result.outerWallSafe =
        result.outerWallStrain
        <= input.material.allowableStrain;

    // A first useful yield flag:
    //
    // At least the outer wall has reached yield strain.
    result.aboveYield =
        result.outerWallStrain
        >= result.yieldStrain;

    // =====================================================
    // 9. NUMERICAL VALIDATION
    // =====================================================

    if (!isFiniteResult(
        result
    ))
    {
        result.status =
            StretchBendingEvaluationStatus::
            NumericalFailure;

        result.valid =
            false;

        return result;
    }

    // =====================================================
    // 10. PRIMARY STATUS
    // =====================================================

    result.status =
        determineStatus(
            input,
            result
        );

    result.valid =
        result.status
        == StretchBendingEvaluationStatus::Valid;

    return result;
}

StretchBendingEvaluationStatus
StretchBendingEvaluator::determineStatus(
    const StretchBendingProcessInput& input,
    const StretchBendingEvaluationResult& result
) const
{
    if (!result.inputValid)
    {
        return StretchBendingEvaluationStatus::
            InvalidInput;
    }

    if (!result.geometryFeasible)
    {
        return StretchBendingEvaluationStatus::
            GeometryNotFeasible;
    }

    if (!result.outerWallSafe)
    {
        return StretchBendingEvaluationStatus::
            OuterWallStrainExceeded;
    }

    if (!result.innerWallSafe)
    {
        return StretchBendingEvaluationStatus::
            InnerWallCompressionRisk;
    }

    if (!result.aboveYield)
    {
        return StretchBendingEvaluationStatus::
            BelowYield;
    }

    return StretchBendingEvaluationStatus::Valid;
}

bool StretchBendingEvaluator::isFiniteResult(
    const StretchBendingEvaluationResult& result
) const
{
    return std::isfinite(result.innerDiameter)
        && std::isfinite(result.area)
        && std::isfinite(result.secondMomentArea)

        && std::isfinite(result.yieldStrain)
        && std::isfinite(result.yieldCurvature)

        && std::isfinite(result.targetCurvature)
        && std::isfinite(result.targetTorsion)
        && std::isfinite(result.targetArcLength)

        && std::isfinite(result.bendingStrain)
        && std::isfinite(result.axialStretchStrain)
        && std::isfinite(result.innerWallStrain)
        && std::isfinite(result.outerWallStrain)

        && std::isfinite(
            result.minimumRequiredAxialStrain
        )
        && std::isfinite(
            result.maximumAllowedAxialStrain
        )

        && std::isfinite(result.axialTension)
        && std::isfinite(
            result.elasticBendingMoment
        )

        && std::isfinite(
            result.innerCompressionMargin
        )
        && std::isfinite(
            result.outerStrainMargin
            && std::isfinite(
                result.recommendedAxialStrain
            )
            && std::isfinite(
                result.axialStrainRange
            )

            && std::isfinite(
                result.commandedTension
            )
            && std::isfinite(
                result.minimumRequiredTension
            )
            && std::isfinite(
                result.maximumAllowedTension
            )
            && std::isfinite(
                result.recommendedTension
            )

            && std::isfinite(
                result.finalTargetCurvature
            )
            && std::isfinite(
                result.springbackRatio
            )
            && std::isfinite(
                result.loadedCurvatureCommand
            )
            && std::isfinite(
                result.predictedFinalCurvature
            )
            && std::isfinite(
                result.finalCurvatureError
            )
            && std::isfinite(
                result.loadedBendingStrain
            )
        );
}