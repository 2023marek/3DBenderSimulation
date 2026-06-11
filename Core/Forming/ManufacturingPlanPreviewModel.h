#pragma once

#include <vector>
#include <iostream>
#include <limits>
#include <cmath>

#include "Core/Forming/ManufacturingPlan.h"
#include "Core/Geometry/PipeNode.h"
#include "Core/Sampling/PipeCurveSampler.h"
#include "Core/Sampling/PipeCurveSampleQuery.h"
#include "Core/Curve/PipeCurveTransform.h"

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
        hasInsertionMarker = false;
        hasInsertionFrame = false;
        hasResolvedInsertionFrame = false;
        transformedInsertedNodes.clear();
        usingTransformedPreviewNodes = false;
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


    bool hasInsertionMarkerNode() const
    {
        buildIfDirty();
        return hasInsertionMarker;
    }

    const PipeNode& getInsertionMarkerNode() const
    {
        buildIfDirty();
        return insertionMarkerNode;
    }

    bool hasInsertionStartFrame() const
    {
        buildIfDirty();
        return hasInsertionFrame;
    }

    const Frame& getInsertionStartFrame() const
    {
        buildIfDirty();
        return insertionFrame;
    }

    bool hasResolvedStartFrame() const
    {
        buildIfDirty();
        return hasResolvedInsertionFrame;
    }

    const Frame& getResolvedStartFrame() const
    {
        buildIfDirty();
        return resolvedInsertionFrame;
    }

    const std::vector<PipeNode>& getTransformedInsertedNodes() const
    {
        buildIfDirty();
        return transformedInsertedNodes;
    }

    bool isUsingTransformedPreviewNodes() const
    {
        buildIfDirty();
        return usingTransformedPreviewNodes;
    }

private:
    double ds = 0.5;

    mutable bool dirty = true;

    ManufacturingPlan plan;

    mutable PipeCurve curve;
    mutable std::vector<PipeNode> nodes;
    mutable bool hasInsertionMarker = false;
    mutable PipeNode insertionMarkerNode;
    mutable bool hasInsertionFrame = false;
    mutable Frame insertionFrame;
    mutable bool hasResolvedInsertionFrame = false;
    mutable Frame resolvedInsertionFrame;
    mutable std::vector<PipeNode> transformedInsertedNodes;
    mutable bool usingTransformedPreviewNodes = false;

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
   // Reset cached debug/overlay data for this build.
   // =====================================================

        hasInsertionMarker = false;
        hasInsertionFrame = false;
        hasResolvedInsertionFrame = false;
        transformedInsertedNodes.clear();  
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
        // =====================================================
// DEBUG INSERTION MARKER
//
// Finds first pass using InsertAtArcLength and stores
// nearest sampled node as marker.
//
// This is visual/debug only.
// =====================================================

        hasInsertionMarker = false;

        for (const auto& pass : plan.passes)
        {
            if (pass.placement.mode
                != PassPlacementMode::InsertAtArcLength)
            {
                continue;
            }

            double targetS =
                pass.placement.arcLength;

            double accumulated =
                0.0;

            if (!nodes.empty())
            {
                insertionMarkerNode =
                    nodes.front();

                double bestError =
                    std::numeric_limits<double>::max();

                for (size_t i = 1; i < nodes.size(); ++i)
                {
                    double step =
                        (nodes[i].pos - nodes[i - 1].pos).length();

                    accumulated += step;

                    double error =
                        std::abs(accumulated - targetS);

                    if (error < bestError)
                    {
                        bestError = error;
                        insertionMarkerNode = nodes[i];
                        hasInsertionMarker = true;
                    }
                }
            }

            break;
        }
        // =====================================================
// DEBUG / FUTURE INSERTION START FRAME
//
// For the first InsertAtArcLength pass, query the frame
// at the requested insertion arc length.
//
// This frame will later be used to orient inserted curves,
// for example:
//     helix starts from selected pipe frame.
// =====================================================

        hasInsertionFrame = false;

        for (const auto& pass : plan.passes)
        {
            if (pass.placement.mode
                != PassPlacementMode::InsertAtArcLength)
            {
                continue;
            }



            auto frameQuery =
                PipeCurveSampleQuery::findFrameAtArcLength(
                    nodes,
                    pass.placement.arcLength
                );

            if (frameQuery.valid)
            {
                insertionFrame =
                    frameQuery.frame;

                hasInsertionFrame =
                    true;

                resolvedInsertionFrame =
                    frameQuery.frame;

                hasResolvedInsertionFrame =
                    true;
                auto localInsertedNodes =
                    PipeCurveSampler::sample(
                        pass.outputCurve,
                        ds
                    );

                transformedInsertedNodes =
                    PipeCurveTransform::transformNodesToFrame(
                        localInsertedNodes,
                        frameQuery.frame
                    );

                std::cout << "[PLAN PREVIEW RESOLVED FRAME] arcLength="
                    << pass.placement.arcLength
                    << " P=("
                    << resolvedInsertionFrame.P.x << ", "
                    << resolvedInsertionFrame.P.y << ", "
                    << resolvedInsertionFrame.P.z << ")"
                    << std::endl;

                std::cout << "[PLAN PREVIEW TRANSFORMED INSERT] localNodes="
                    << localInsertedNodes.size()
                    << " transformedNodes="
                    << transformedInsertedNodes.size()
                    << std::endl;


            }
           
            break;
        }
        dirty =
            false;
    }
};
