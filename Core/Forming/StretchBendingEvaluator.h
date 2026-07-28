#pragma once

#include "Core/Forming/StretchBendingProcessInput.h"
#include "Core/Forming/StretchBendingEvaluationResult.h"

// =====================================================
// STRETCH-BENDING EVALUATOR
//
// Converts one stretch-bending process input into
// calculated section, strain, force, moment, and
// feasibility values.
//
// This class:
//
//     does not generate PipeNodes
//     does not run manufacturing playback
//     does not modify ManufacturingState
//     does not render anything
// =====================================================

class StretchBendingEvaluator
{
public:
    StretchBendingEvaluationResult evaluate(
        const StretchBendingProcessInput& input
    ) const;

private:
    StretchBendingEvaluationStatus determineStatus(
        const StretchBendingProcessInput& input,
        const StretchBendingEvaluationResult& result
    ) const;

    bool isFiniteResult(
        const StretchBendingEvaluationResult& result
    ) const;
};