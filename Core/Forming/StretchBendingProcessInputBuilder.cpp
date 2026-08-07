#include "Core/Forming/StretchBendingProcessInputBuilder.h"

StretchBendingProcessInput
StretchBendingProcessInputBuilder::build(
    const StretchBendingOperation& operation)
{
    StretchBendingProcessInput input;

    if (!operation.isValid())
        return input;

    input.pipeSection =
        operation.pipeSection;

    input.material =
        operation.material;

    input.geometry.targetArcLength =
        operation.arcLength;

    input.geometry.targetCurvature =
        operation.targetFinalCurvature;

    input.geometry.targetTorsion =
        operation.targetTorsion;

    input.axialStretchStrain =
        operation.axialStretchStrain;

    input.feedSpeed =
        operation.feedSpeed;

    input.sampleStep =
        operation.sampleStep;

    input.springbackRatio =
        operation.springbackRatio;

    input.compensateSpringback =
        operation.compensateSpringback;

    input.enabled =
        operation.enabled;

    return input;
}