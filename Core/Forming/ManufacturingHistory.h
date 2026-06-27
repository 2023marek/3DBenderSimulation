#pragma once

#include <vector>

#include "Core/Forming/ManufacturingPass.h"
#include "Core/Forming/AdditionalFormingPass.h"

// =====================================================
// MANUFACTURING HISTORY
//
// Real process-history container.
//
// This is different from PlannedShapePreview.
//
// PlannedShapePreview:
//     What final shape do I want?
//
// ManufacturingHistory:
//     How was this pipe physically made?
//
// Future flow:
//
//     primary pass
//         ?
//     additional forming pass
//         ?
//     additional forming pass
//         ?
//     final manufactured state
// =====================================================

struct ManufacturingHistory
{
    std::vector<ManufacturingPass> primaryPasses;
    std::vector<AdditionalFormingPass> additionalPasses;

    void clear()
    {
        primaryPasses.clear();
        additionalPasses.clear();
    }

    bool empty() const
    {
        return primaryPasses.empty()
            && additionalPasses.empty();
    }
};
