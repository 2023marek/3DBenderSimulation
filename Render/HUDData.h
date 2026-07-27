#pragma once

#include <cstddef>
#include <string>
#include "Core/Forming/AdditionalPassExecutionResult.h"
struct HUDData
{
    // ===== Raw state =====
    bool isPlaying = false;
    bool isPaused = false;

    // ===== Time / motion =====
    double speed = 0.0;
    double time = 0.0;

    double feedPosition = 0.0;
    double rotationDeg = 0.0;
    double bendDeg = 0.0;

    // ===== Machine active flags =====
    bool feeding = false;
    bool rotating = false;
    bool bending = false;

    // ===== Operations =====
    size_t currentOpIndex = 0;
    size_t totalOperations = 0;

    // ===== Progress =====
    double currentOpProgress = 0.0;
    double overallProgress = 0.0;
    //====================================debug

    bool plannedPreviewDebugVisible = true;
    // ===== Geometry =====
    size_t nodeCount = 0;

    bool hasAdditionalPassResult =
        false;

    bool deformableRegionOverlayVisible =
        true;

    bool spatialIntegratorPreviewVisible =
        true;
    AdditionalPassExecutionResult additionalPassResult =   AdditionalPassExecutionResult::Disabled;


    // ===== Display-ready =====
    std::string status = "IDLE";
    std::string currentOpName = "";
    std::string machineStateName = "IDLE";
    std::string simulationModeName = "";
    std::string placementModeName = "";
    std::string previewDebugName = "ON";
    std::string attachModeName = "-";
};