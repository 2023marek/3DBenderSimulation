#pragma once

#include <vector>

#include "Core/Forming/ManufacturingHistory.h"
#include "Core/Forming/ManufacturingHistorySummary.h"

std::vector<AdditionalPassPlacementSummary>
buildAdditionalPassPlacementSummaries(
    const ManufacturingHistory& history
);