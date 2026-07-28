#pragma once

enum class StretchBendingEvaluationStatus
{
    NotEvaluated,
    Valid,
    Disabled,
    InvalidInput,
    InvalidPipeSection,
    InvalidMaterial,
    InvalidGeometry,
    BelowYield,
    InnerWallCompressionRisk,
    OuterWallStrainExceeded,
    GeometryNotFeasible,
    NumericalFailure
};

const char* stretchBendingEvaluationStatusToString(
    StretchBendingEvaluationStatus status
);