#pragma once

#include <vector>

#include "Core/Forming/ManufacturingHistory.h"
#include "Core/Forming/ManufacturingHistorySummary.h"

std::vector<PrimaryPassSummary>
buildPrimaryPassSummaries(
    const ManufacturingHistory& history
);

std::vector<AdditionalPassPlacementSummary>
buildAdditionalPassPlacementSummaries(
    const ManufacturingHistory& history

);

ManufacturingHistorySummary
buildManufacturingHistorySummary(
    const ManufacturingHistory& history
);

