#pragma once

#include "Core/Forming/StretchBendingOperation.h"
#include "Core/Forming/StretchBendingProcessInput.h"

// =====================================================
// STRETCH-BENDING PROCESS INPUT BUILDER
//
// Converts a reusable operation into the exact input
// required by StretchBendingEvaluator.
// =====================================================

class StretchBendingProcessInputBuilder
{
public:
    static StretchBendingProcessInput build(
        const StretchBendingOperation& operation
    );
};