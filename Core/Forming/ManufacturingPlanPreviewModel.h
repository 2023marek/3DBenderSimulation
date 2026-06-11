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

    //======================================

    void build() const
    {
        hasInsertionMarker = false;
        hasInsertionFrame = false;
        hasResolvedInsertionFrame = false;
        usingTransformedPreviewNodes = false;

        transformedInsertedNodes.clear();

        const ManufacturingPass* insertPass =
            findFirstInsertAtArcLengthPass();

        if (insertPass)
        {
            curve =
                buildBaseCurveBeforePass(
                    *insertPass
                );

            nodes =
                PipeCurveSampler::sample(
                    curve,
                    ds
                );

            applyTransformedInsertionPreview(
                *insertPass
            );
        }
        else
        {
            curve =
                plan.buildCombinedCurve();

            nodes =
                PipeCurveSampler::sample(
                    curve,
                    ds
                );
        }

        std::cout << "[PLAN PREVIEW BUILD] passes="
            << plan.size()
            << " curveSegments="
            << curve.size()
            << " nodes="
            << nodes.size()
            << " transformedPreview="
            << usingTransformedPreviewNodes
            << std::endl;

        dirty = false;
    }




    // Helper Reason:
    //When joining curves, first node of next section often duplicates
    //the last node of previous section.


    static void appendNodesSkippingFirst(
        std::vector<PipeNode>& target,
        const std::vector<PipeNode>& source)
    {
        if (source.empty())
            return;

        size_t startIndex =
            target.empty() ? 0 : 1;

        for (size_t i = startIndex; i < source.size(); ++i)
        {
            target.push_back(
                source[i]
            );
        }
    }

    static Frame frameFromNode(
        const PipeNode& node)
    {
        Frame f;

        f.P = node.pos;
        f.T = node.T;
        f.N = node.N;
        f.B = node.B;

        return f;
    }

    PipeCurve buildBaseCurveBeforePass(
        const ManufacturingPass& targetPass) const
    {
        PipeCurve baseCurve;

        for (const auto& pass : plan.passes)
        {
            if (&pass == &targetPass)
                break;

            if (!pass.enabled)
                continue;

            baseCurve.appendCurve(
                pass.outputCurve
            );
        }

        return baseCurve;
    }

   

    bool applyTransformedInsertionPreview(
        const ManufacturingPass& pass) const
    {
        // =====================================================
        // TRANSFORMED INSERTION PREVIEW
        //
        // Builds preview nodes as:
        //
        // base before insertion
        //      +
        // transformed inserted pass
        //      +
        // base after insertion
        //
        // This is preview-node composition only.
        // It does not modify the underlying PipeCurve model.
        // =====================================================

        PipeCurve baseCurve =
            buildBaseCurveBeforePass(
                pass
            );

        auto baseNodes =
            PipeCurveSampler::sample(
                baseCurve,
                ds
            );

        auto frameQuery =
            PipeCurveSampleQuery::findFrameAtArcLength(
                baseNodes,
                pass.placement.arcLength
            );

        if (!frameQuery.valid)
            return false;

        insertionFrame =
            frameQuery.frame;

        hasInsertionFrame =
            true;

        resolvedInsertionFrame =
            frameQuery.frame;

        hasResolvedInsertionFrame =
            true;

        insertionMarkerNode.pos =
            frameQuery.frame.P;

        insertionMarkerNode.T =
            frameQuery.frame.T;

        insertionMarkerNode.N =
            frameQuery.frame.N;

        insertionMarkerNode.B =
            frameQuery.frame.B;

        hasInsertionMarker =
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

        PipeCurveSplitResult split =
            baseCurve.splitAtArcLength(
                pass.placement.arcLength
            );

        if (!split.valid)
            return false;

        auto beforeNodes =
            PipeCurveSampler::sample(
                split.before,
                ds
            );

        auto localAfterNodes =
            PipeCurveSampler::sample(
                split.after,
                ds
            );

        std::vector<PipeNode> afterNodes;

        if (!transformedInsertedNodes.empty())
        {
            Frame insertedEndFrame =
                frameFromNode(
                    transformedInsertedNodes.back()
                );

            afterNodes =
                PipeCurveTransform::transformNodesToFrame(
                    localAfterNodes,
                    insertedEndFrame
                );
        }
        else
        {
            afterNodes =
                localAfterNodes;
        }

        std::vector<PipeNode> rebuiltNodes;

        appendNodesSkippingFirst(
            rebuiltNodes,
            beforeNodes
        );

        appendNodesSkippingFirst(
            rebuiltNodes,
            transformedInsertedNodes
        );

        appendNodesSkippingFirst(
            rebuiltNodes,
            afterNodes
        );

        if (rebuiltNodes.empty())
            return false;

        nodes =
            rebuiltNodes;

        usingTransformedPreviewNodes =
            true;

        std::cout << "[PLAN PREVIEW NODE REBUILD] beforeNodes="
            << beforeNodes.size()
            << " insertedNodes="
            << transformedInsertedNodes.size()
            << " afterNodes="
            << afterNodes.size()
            << " finalNodes="
            << nodes.size()
            << std::endl;

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

        return true;
    }
    const ManufacturingPass* findFirstInsertAtArcLengthPass() const
    {
        for (const auto& pass : plan.passes)
        {
            if (!pass.enabled)
                continue;

            if (pass.placement.mode
                == PassPlacementMode::InsertAtArcLength)
            {
                return &pass;
            }
        }

        return nullptr;
    }
};
