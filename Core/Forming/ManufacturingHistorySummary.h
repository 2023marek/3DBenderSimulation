#pragma once

#include <string>
#include <vector>

#include "Core/Geometry/Frame.h"

// =====================================================
// MANUFACTURING HISTORY SUMMARY
//
// Small read-only data object prepared for future HUD
// or debug overlay display.
//
// This does not own simulation.
// This does not render.
// This only converts manufacturing-history data into
// easy-to-display text/numeric values.
// =====================================================
struct PrimaryPassSummary
{
    std::string processName;
    bool enabled = true;
};

struct AdditionalPassPlacementSummary
{
    std::string name;
    std::string processName;

    std::string placementModeName;
    bool resolved = false;

    std::string requestedValueText;

    double resolvedArcLength = 0.0;

    Frame entryFrame;
};

struct ManufacturingHistorySummary
{
    std::vector<PrimaryPassSummary> primaryPasses;
    std::vector<AdditionalPassPlacementSummary> additionalPasses;
};