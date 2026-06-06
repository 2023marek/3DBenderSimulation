#pragma once

#include <vector>
#include <iostream>

#include "Core/Forming/ManufacturingPlan.h"
#include "Core/Sampling/PipeCurveSampler.h"
#include "Core/Geometry/PipeNode.h"

// =====================================================
// MANUFACTURING PLAN PREVIEW MODEL
//
// OWNER:
// Preview-only model for a full multi-pass manufacturing plan.
//
// PURPOSE:
// Converts a ManufacturingPlan into sampled PipeNodes.
//
// IMPORTANT:
// This does NOT simulate manufacturing playback.
// This does NOT own incoming stock.
// This does NOT own active bend zones.
// This does NOT move machine parts.
//
// It is a CAD-like preview of the final intended shape.
//
// Pipeline:
//
// ManufacturingPlan
//      ?
// combined PipeCurve
//      ?
// PipeCurveSampler
//      ?
// preview nodes
// =====================================================

class ManufacturingPlanPreviewModel
{
public:
    explicit ManufacturingPlanPreviewModel(
        double sampleStep = 0.5)
        : ds(sampleStep)
    {
    }

    void clear()
    {
        plan.clear();
        curve.clear();
        nodes.clear();

        dirty = true;
    }

    void setPlan(
        const ManufacturingPlan& newPlan)
    {
        // =====================================================
        // Store the full manufacturing plan.
        //
        // We do not build immediately.
        // Build is lazy, like GeometricPipeModel.
        // =====================================================

        plan =
            newPlan;

        dirty =
            true;
    }

    const ManufacturingPlan& getPlan() const
    {
        return plan;
    }

    const PipeCurve& getCurve() const
    {
        buildIfDirty();
        return curve;
    }

    const std::vector<PipeNode>& getNodes() const
    {
        buildIfDirty();
        return nodes;
    }

    size_t getNodeCount() const
    {
        buildIfDirty();
        return nodes.size();
    }

private:
    double ds = 0.5;

    mutable bool dirty = true;

    ManufacturingPlan plan;

    mutable PipeCurve curve;
    mutable std::vector<PipeNode> nodes;

private:
    void buildIfDirty() const
    {
        if (!dirty)
            return;

        build();
    }

    void build() const
    {
        // =====================================================
        // MULTI-PASS PREVIEW BUILD
        //
        // Important:
        // The ManufacturingPlan already contains output curves
        // from each pass.
        //
        // We only compose and sample them here.
        // =====================================================

        curve =
            plan.buildCombinedCurve();

        nodes =
            PipeCurveSampler::sample(
                curve,
                ds
            );

        std::cout << "[PLAN PREVIEW BUILD] passes="
            << plan.size()
            << " curveSegments="
            << curve.size()
            << " nodes="
            << nodes.size()
            << std::endl;

        dirty =
            false;
    }
};
