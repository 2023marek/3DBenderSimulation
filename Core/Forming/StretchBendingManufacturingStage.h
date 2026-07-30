#pragma once

// =====================================================
// STRETCH-BENDING MANUFACTURING STAGE
//
// Describes the high-level state of the stretch-bending
// manufacturing process.
//
// This is intentionally independent from the existing
// rotary-draw manufacturing state.
//
// Phase 10J only defines the states.
// Later phases will use them during playback.
// =====================================================

enum class StretchBendingManufacturingStage
{
    // -------------------------------------------------
    // No valid manufacturing process has been prepared.
    // -------------------------------------------------

    Invalid,

    // -------------------------------------------------
    // Pipe and machine are ready, but no forming load
    // has yet been applied.
    // -------------------------------------------------

    Ready,

    // -------------------------------------------------
    // Axial tension is being established.
    //
    // The pipe may be stretched, but bending has not
    // yet reached the commanded loaded curvature.
    // -------------------------------------------------

    ApplyingTension,

    // -------------------------------------------------
    // Bending is actively occurring inside the fixed
    // active zone.
    // -------------------------------------------------

    Forming,

    // -------------------------------------------------
    // Loaded machine geometry has been reached.
    //
    // The forming load is still considered present.
    // -------------------------------------------------

    LoadedHold,

    // -------------------------------------------------
    // Bending load is being removed and elastic
    // springback is occurring.
    // -------------------------------------------------

    Unloading,

    // -------------------------------------------------
    // Final unloaded geometry has been reached.
    // -------------------------------------------------

    Complete

   
};

const char* stretchBendingManufacturingStageToString(
    StretchBendingManufacturingStage stage);
