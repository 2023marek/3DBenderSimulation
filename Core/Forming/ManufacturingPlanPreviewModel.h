#pragma once

#include <vector>
#include <iostream>
#include <limits>
#include <cmath>
#include <string>

#include "Core/Forming/ManufacturingPlan.h"
#include "Core/Geometry/PipeNode.h"
#include "Core/Sampling/PipeCurveSampler.h"
#include "Core/Sampling/PipeCurveSampleQuery.h"
#include "Core/Curve/PipeCurveTransform.h"
#include "Core/Sampling/PipeCurveNodeQuery.h"

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
// =====================================================
// MANUFACTURING PLAN PREVIEW MODEL
//
// Purpose:
// - build final planned shape preview from ManufacturingPlan
// - support placement previews:
//      ArcLength
//      NodeIndex
//      ExplicitFrame
//
// Important:
// Thisoming stock
// - posit is NOT ManufacturingPlayback.
//
// It does NOT simulate:
// - incioned straight
// - active bend zone
// - frozen geometry
//
// ManufacturingPlayback owns the four-zone process model.
// =====================================================
//=========================================================

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

    //FLAG
    //flag setter
    void setDebugLogging(
        bool enabled)
    {
        debugLogging = enabled;
    }
//flag getter
    bool isDebugLoggingEnabled() const
    {
        return debugLogging;
    }
    //Setters getters
    void setShowInsertionMarker(bool enabled)
    {
        showInsertionMarker = enabled;
    }

    void setShowInsertionFrame(bool enabled)
    {
        showInsertionFrame = enabled;
    }

    void setShowTransformedInsertOverlay(bool enabled)
    {
        showTransformedInsertOverlay = enabled;
    }

    bool shouldShowInsertionMarker() const
    {
        return showInsertionMarker;
    }

    bool shouldShowInsertionFrame() const
    {
        return showInsertionFrame;
    }

    bool shouldShowTransformedInsertOverlay() const
    {
        return showTransformedInsertOverlay;
    }

    //HUD getter

    std::string getActivePlacementModeName() const
    {
        const ManufacturingPass* pass =
            findFirstInsertPlacementPass();

        if (!pass)
            return "APPEND";

        if (pass->placement.mode
            == PassPlacementMode::InsertAtArcLength)
        {
            return "ARC LENGTH";
        }

        if (pass->placement.mode
            == PassPlacementMode::InsertAtNodeIndex)
        {
            return "NODE INDEX";
        }

        if (pass->placement.mode
            == PassPlacementMode::ExplicitStartFrame)
        {
            return "EXPLICIT FRAME";
        }

        return "APPEND";
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
    bool debugLogging = true;
    bool showInsertionMarker = true;
    bool showInsertionFrame = true;
    bool showTransformedInsertOverlay = false;

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
            //findFirstInsertAtArcLengthPass();
            findFirstInsertPlacementPass();
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
        if (debugLogging)
        {
            std::cout << "[PLAN PREVIEW BUILD] passes="
                << plan.size()
                << " curveSegments="
                << curve.size()
                << " nodes="
                << nodes.size()
                << " transformedPreview="
                << usingTransformedPreviewNodes
                << std::endl;
        }
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


      double resolvedArcLength = 0.0;
Frame resolvedFrame;

if (!resolvePlacementStartFrame(
        pass,
        baseCurve,
        resolvedFrame,
        resolvedArcLength))
{
    return false;
}

        insertionFrame =
            resolvedFrame;

        hasInsertionFrame =
            true;

        resolvedInsertionFrame =
            resolvedFrame;

        hasResolvedInsertionFrame =
            true;

        insertionMarkerNode.pos =
            resolvedFrame.P;

        insertionMarkerNode.T =
            resolvedFrame.T;

        insertionMarkerNode.N =
            resolvedFrame.N;

        insertionMarkerNode.B =
            resolvedFrame.B;

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
                resolvedFrame
            );

        // After transformedInsertedNodes is created

        if (pass.placement.mode
            == PassPlacementMode::ExplicitStartFrame)
        {
            nodes =
                transformedInsertedNodes;

            usingTransformedPreviewNodes =
                true;

            hasInsertionMarker =
                true;

            insertionMarkerNode.pos =
                resolvedFrame.P;
            insertionMarkerNode.T =
                resolvedFrame.T;
            insertionMarkerNode.N =
                resolvedFrame.N;
            insertionMarkerNode.B =
                resolvedFrame.B;

            if (debugLogging)
            {
                std::cout << "[PLAN PREVIEW EXPLICIT INSERT] insertedNodes="
                    << transformedInsertedNodes.size()
                    << std::endl;
            }

            return true;
        }

        PipeCurveSplitResult split =
            baseCurve.splitAtArcLength(
               // pass.placement.arcLength
                resolvedArcLength
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
        if (debugLogging)
        {
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
                << resolvedArcLength// pass.placement.arcLength
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

            std::cout << "[PLAN PREVIEW EXPLICIT INSERT] insertedNodes="
                << transformedInsertedNodes.size()
                << " attachMode="
                << explicitFrameAttachModeToString(
                    pass.placement.explicitFrameAttachMode
                )
                << std::endl;
        }
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

    const ManufacturingPass* findFirstInsertPlacementPass() const
    {
        for (const auto& pass : plan.passes)
        {
            if (!pass.enabled)
                continue;

            if (pass.placement.mode
                == PassPlacementMode::InsertAtArcLength
                || pass.placement.mode
                == PassPlacementMode::InsertAtNodeIndex
                || pass.placement.mode
                == PassPlacementMode::ExplicitStartFrame)
            {
                return &pass;
            }
        }

        return nullptr;
    }



    //HELPER

    bool resolvePlacementArcLength(
        const ManufacturingPass& pass,
        const PipeCurve& baseCurve,
        double& outArcLength) const
    {
        // =====================================================
        // PLACEMENT RESOLUTION
        //
        // Converts pass placement into arc length on base curve.
        //
        // Supported:
        // - InsertAtArcLength
        // - InsertAtNodeIndex
        //
        // Later:
        // - ExplicitStartFrame
        // =====================================================

        if (pass.placement.mode
            == PassPlacementMode::InsertAtArcLength)
        {
            outArcLength =
                pass.placement.arcLength;

            return true;
        }

        if (pass.placement.mode
            == PassPlacementMode::InsertAtNodeIndex)
        {
            auto baseNodes =
                PipeCurveSampler::sample(
                    baseCurve,
                    ds
                );

            if (baseNodes.empty())
                return false;

            outArcLength =
                PipeCurveNodeQuery::arcLengthAtNodeIndex(
                    baseNodes,
                    pass.placement.nodeIndex
                );

            if (pass.placement.mode
                == PassPlacementMode::ExplicitStartFrame)
            {
                return false;
            }

            if (debugLogging)
            {
                std::cout << "[PLAN PREVIEW NODE PLACEMENT] nodeIndex="
                    << pass.placement.nodeIndex
                    << " arcLength="
                    << outArcLength
                    << " nodeCount="
                    << baseNodes.size()
                    << std::endl;
            }

            return true;
        }

        return false;
    }


    //Helper
    bool resolvePlacementStartFrame(
        const ManufacturingPass& pass,
        const PipeCurve& baseCurve,
        Frame& outFrame,
        double& outArcLength) const
    {
        // =====================================================
        // START FRAME RESOLUTION
        //
        // Converts placement into a concrete Frame.
        //
        // Supported:
        // - InsertAtArcLength
        // - InsertAtNodeIndex
        // - ExplicitStartFrame
        //
        // outArcLength:
        // - meaningful for arc/node placement
        // - 0 for ExplicitStartFrame
        // =====================================================

        if (pass.placement.mode
            == PassPlacementMode::ExplicitStartFrame)
        {
            outFrame =
                pass.placement.startFrame;

            outArcLength =
                0.0;

            if (debugLogging)
            {
                std::cout << "[PLAN PREVIEW EXPLICIT FRAME] P=("
                    << outFrame.P.x << ", "
                    << outFrame.P.y << ", "
                    << outFrame.P.z << ")"
                    << std::endl;
            }

            return true;
        }

        if (!resolvePlacementArcLength(
            pass,
            baseCurve,
            outArcLength))
        {
            return false;
        }

        auto baseNodes =
            PipeCurveSampler::sample(
                baseCurve,
                ds
            );

        auto frameQuery =
            PipeCurveSampleQuery::findFrameAtArcLength(
                baseNodes,
                outArcLength
            );

        if (!frameQuery.valid)
            return false;

        outFrame =
            frameQuery.frame;

        return true;
    }
};
