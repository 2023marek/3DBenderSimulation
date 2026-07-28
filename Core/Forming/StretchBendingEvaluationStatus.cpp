#include "Core/Forming/StretchBendingEvaluationStatus.h"

const char* stretchBendingEvaluationStatusToString(
    StretchBendingEvaluationStatus status
)
{
    switch (status)
    {
    case StretchBendingEvaluationStatus::NotEvaluated:
        return "NotEvaluated";

    case StretchBendingEvaluationStatus::Valid:
        return "Valid";

    case StretchBendingEvaluationStatus::Disabled:
        return "Disabled";

    case StretchBendingEvaluationStatus::InvalidInput:
        return "InvalidInput";

    case StretchBendingEvaluationStatus::InvalidPipeSection:
        return "InvalidPipeSection";

    case StretchBendingEvaluationStatus::InvalidMaterial:
        return "InvalidMaterial";

    case StretchBendingEvaluationStatus::InvalidGeometry:
        return "InvalidGeometry";

    case StretchBendingEvaluationStatus::BelowYield:
        return "BelowYield";

    case StretchBendingEvaluationStatus::InnerWallCompressionRisk:
        return "InnerWallCompressionRisk";

    case StretchBendingEvaluationStatus::OuterWallStrainExceeded:
        return "OuterWallStrainExceeded";

    case StretchBendingEvaluationStatus::GeometryNotFeasible:
        return "GeometryNotFeasible";

    case StretchBendingEvaluationStatus::NumericalFailure:
        return "NumericalFailure";
    }

    return "Unknown";
}