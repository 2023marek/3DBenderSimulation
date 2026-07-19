#pragma once

#include <algorithm>
#include <iostream>
#include "Core/BendDirection.h"
#include "Core/Geometry/Frame.h"
#include "Core/Geometry/PipeNode.h"
#include "Core/Math/Vec3D.h"
#include "Core/Manufacturing/ManufacturingState.h"
#include "Core/Manufacturing/RotationKinematicMode.h"
#include "Core/Forming/AdditionalFormingPass.h"
#include "Core/Forming/AdditionalPassExecutionResult.h"
#include "Core/Forming/DeformableRegion.h"
#include "Core/Forming/DeformableRegionSelection.h"
                   

class ManufacturingPipeSimulator
{
public:
    ManufacturingPipeSimulator()
        : ds(0.5),
        rotationMode(RotationKinematicMode::PipeRoll),
        state()
        
    {
        resetFrames();
    }

    explicit ManufacturingPipeSimulator(double sampleStep)
        : ds(sampleStep),
        rotationMode(RotationKinematicMode::PipeRoll),
        state()
        
    {
        resetFrames();
    }


   

    void setDebugSnapshot(
        bool enabled
    )
    {
        debugSnapshot =
            enabled;
    }

    bool isBendActive() const
    {
        return state.activeZone.active;
    }

    bool isIncomingStockExhausted() const
    {
        return state.incomingStock.exhausted;
    }
    //Getters/Setters
    // =====================================================
// ADDITIONAL FORMING PASS EXECUTION
//
// Placeholder for real multi-pass manufacturing.
//
// This is not preview insertion.
// This is intended for future physical continuation:
// already formed pipe -> additional forming pass
// =====================================================
    AdditionalPassExecutionResult executeAdditionalFormingPass(
        const AdditionalFormingPass& additionalPass
    )
    {
        if (!additionalPass.enabled)
        {
            return AdditionalPassExecutionResult::Disabled;
        }

        if (!isValidFrame(
            additionalPass.entryFrame
        ))
        {
            return AdditionalPassExecutionResult::InvalidEntryFrame;
        }

        if (!additionalPass.deformableRegion.isValid())
        {
            return AdditionalPassExecutionResult::
                InvalidDeformableRegion;
        }

        if (!isSupportedAdditionalPassProcess(
            additionalPass.pass.processType
        ))
        {
            return AdditionalPassExecutionResult::
                UnsupportedProcess;
        }

        return AdditionalPassExecutionResult::Validated;
    }
        
       

        // Future:
        // - validate supported process type
        // - select deformable region
        // - configure machine/process constraints
        // - execute real forming operation
        // - update ManufacturingState

    


    double getIncomingStockRemainingLength() const
    {
        return state.incomingStock.remainingLength;
    }

    double getIncomingStockConsumedLength() const
    {
        return state.incomingStock.consumedLength;
    }

    double getIncomingStockTotalLength() const
    {
        return state.incomingStock.totalLength;
    }

    void setIncomingStockLength(double length)
    {
        if (length <= 0.0)
            return;

        state.incomingStock.totalLength = length;
        state.incomingStock.remainingLength = length;
        state.incomingStock.consumedLength = 0.0;

        renderNodes.clear();
        state.renderData.clear();
        state.incomingStock.exhausted = false;
        

    }


    const Frame& getMachineEntryFrame() const
    {
        return machineEntryFrame;
    }

    const Frame& getCurrentFrame() const
    {
        return currentFrame;
    }

    Frame& getMachineEntryFrame()
    {
        return machineEntryFrame;
    }

    Frame& getCurrentFrame()
    {
        return currentFrame;
    }


    //=====================================
    //Legacy
   



	//=====================================================
    void reset()
    {
        state.clear();
        renderNodes.clear();

        resetFrames();

        //axis.clear();
       // axis.markGeometryDirty();
    }

    ManufacturingState& getState()
    {
        return state;
    }

    const ManufacturingState& getState() const
    {
        return state;
    }



    double processFeed(double distance)
    {
        
        // =====================================================
        // FEED OPERATION
        //
        // OWNER:
        // ManufacturingPipeSimulator now owns manufacturing
        // feed-state logic.
        //
        // Meaning:
        // IncomingStock -> PositionedStraight
        //
        // Old frozen geometry is pushed forward together with
        // the material feed.
        // =====================================================

        if (distance <= 0.0)
            return 0.0;

        double actualFeed =
            std::min(distance, state.incomingStock.remainingLength);
        
        if (actualFeed <= 0.0)
        {
            state.incomingStock.exhausted =
                true;

            return 0.0;
        }

        state.incomingStock.remainingLength -= actualFeed;

        if (state.incomingStock.remainingLength <= 1e-9)
        {
            state.incomingStock.remainingLength =
                0.0;

            state.incomingStock.exhausted =
                true;
        }
        state.incomingStock.consumedLength += actualFeed;

        if (state.incomingStock.remainingLength < 0.0)
            state.incomingStock.remainingLength = 0.0;

        state.positionedStraight.length += actualFeed;

        moveFrozenGeometryDuringFeed(actualFeed);

        if (debugFeed)
        {
            std::cout << "[MFG SIM FEED] incomingRemaining="
                << state.incomingStock.remainingLength
                << " positionedStraight="
                << state.positionedStraight.length
                << " consumed="
                << state.incomingStock.consumedLength
                << std::endl;
        }


        printManufacturingSnapshot(
            "after feed"
        );

        return actualFeed;
    }

    void processBend(
        double radius,
        double targetAngle,
        double angleIncrement,
        BendDirection bendDirection)
    {
        // =====================================================
        // BEND OPERATION
        //
        // OWNER:
        // ManufacturingPipeSimulator now owns the top-level
        // processBend() flow.
        //
        // TEMPORARY:
        // PipeAxis3D still owns the low-level bend helpers.
        // =====================================================

        if (radius <= 1e-9)
            return;

        if (targetAngle <= 0.0)
            return;

        if (angleIncrement <= 0.0)
            return;

        if (state.positionedStraight.length <= 0.0)
        {
            std::cerr
                << "[MFG SIM BEND WARNING] No positioned straight material available."
                << std::endl;

            return;
        }

        if (!state.activeZone.active)
        {
            beginBendFromFrame(
                getBendStartFrame(),
                radius,
                targetAngle,
                bendDirection
            );
        }

        

        Frame previousFrozenAttachFrame =
          makePositionedStraightEndFrame(
                state.activeZone.frame,
                state.positionedStraight.length
            );

        

        double remainingAngle =
            state.activeZone.targetAngle
            - state.activeZone.accumulatedAngle;

        if (remainingAngle <= 0.0)
        {
            freezeActiveZone();
           
            return;
        }

        double maxAngleByMaterial =
            state.positionedStraight.length / radius;

        double stepAngle =
            std::min(
                angleIncrement,
                std::min(
                    remainingAngle,
                    maxAngleByMaterial
                )
            );
         
        if (stepAngle <= 0.0)
            return;

        double arcStepLength =
            radius * stepAngle;

        state.positionedStraight.length -= arcStepLength;

        if (state.positionedStraight.length < 0.0)
            state.positionedStraight.length = 0.0;

       updateActiveZone(stepAngle);

        Frame newFrozenAttachFrame =
            makePositionedStraightEndFrame(
                state.activeZone.frame,
                state.positionedStraight.length
            );

        transformFrozenGeometryBetweenFrames(
            previousFrozenAttachFrame,
            newFrozenAttachFrame
        );

        if (state.activeZone.accumulatedAngle
            >= state.activeZone.targetAngle)
        {
            freezeActiveZone();
        }

        if (debugBendStep)
        {
            std::cout
                << "[MFG SIM BEND] arcStep="
                << arcStepLength
                << " positionedStraightLeft="
                << state.positionedStraight.length
                << std::endl;
        }
    }



    void processRotate(double signedAngle)
    {
        if (std::abs(signedAngle) < 1e-12)
            return;

        RotationKinematicMode mode =
            rotationMode;

        if (mode == RotationKinematicMode::PipeRoll)
        {
            rotatePipeBodyAroundMachineAxis(signedAngle);

            if (debugRotate)
            {
                std::cout << "[MFG SIM ROTATE] PipeRoll angleDeg="
                    << signedAngle * 180.0 / PI
                    << std::endl;
            }
        }
        else if (mode == RotationKinematicMode::ToolHeadRotate)
        {
            rotateToolPlaneAroundMachineAxis(signedAngle);

            if (debugRotate)
            {
                std::cout << "[MFG SIM ROTATE] ToolHeadRotate angleDeg="
                    << signedAngle * 180.0 / PI
                    << std::endl;
            }
        }
    }

    void reconstructVisiblePipe()
    {
        buildManufacturingRenderData();
        flattenManufacturingRenderData();

        if (debugReconstruct)
        {
            std::cout << "[MFG SIM RECONSTRUCT] nodes="
                << renderNodes.size()
                << std::endl;
        }
    }

    void setRotationKinematicMode(RotationKinematicMode mode)
    {
        rotationMode = mode;
    }

    RotationKinematicMode getRotationKinematicMode() const
    {
        return rotationMode;
    }

    const ManufacturingRenderData& getManufacturingRenderData() const
    {
        return state.renderData;
    }

    const std::vector<PipeNode>& getNodes() const
    {
        return renderNodes;
    }

    void setDebugActiveZoneStep(
        bool enabled
    )
    {
        debugActiveZoneStep =
            enabled;
    }
    void setDebugActiveWindow(
        bool enabled
    )
    {
        debugActiveWindow =
            enabled;
    }
    void setDebugBendStep(
        bool enabled
    )
    {
        debugBendStep =
            enabled;
    }
    void setDebugRenderData(
        bool enabled
    )
    {
        debugRenderData =
            enabled;
    }
    void setDebugReconstruct(
        bool enabled
    )
    {
        debugReconstruct =
            enabled;
    }

    void setDebugFeed(
        bool enabled
    )
    {
        debugFeed =
            enabled;
    }

    void setDebugRotate(
        bool enabled
    )
    {
        debugRotate =
            enabled;
    }

    void setDebugFreeze(
        bool enabled
    )
    {
        debugFreeze =
            enabled;
    }

    void setDebugAll(
        bool enabled
    )
    {
        setDebugActiveWindow(enabled);
        setDebugActiveZoneStep(enabled);
        setDebugBendStep(enabled);
        setDebugRenderData(enabled);
        setDebugReconstruct(enabled);
        setDebugFeed(enabled);
        setDebugRotate(enabled);
        setDebugFreeze(enabled);
        setDebugSnapshot(enabled);
       
    }



    DeformableRegionSelection selectDeformableRegion(
        const DeformableRegion& region
    ) const
    {
        std::vector<PipeNode> sourceNodes =
            buildCompletePrimaryOutputNodes();

        return selectNodesByArcLengthRange(
            sourceNodes,
            region
        );
    }



	//Helper
   


    //Getter
    double getAvailablePrimaryOutputLength() const
    {
        return calculateAvailablePrimaryOutputLength();
    }

private:



    bool debugActiveWindow =
        false;
    bool debugActiveZoneStep =
        false;
    bool debugBendStep =
        false;
    bool debugRenderData =
        false;
    bool debugReconstruct =
        false;
    bool debugFeed =
        false;
    bool debugRotate =
        false;
    bool debugFreeze =
        false;

    bool debugSnapshot = false;

//Helper
    void appendNodeNoDuplicate(
        std::vector<PipeNode>& dst,
        const PipeNode& node
    ) const
    {
        if (!dst.empty()
            && nearlySamePoint(
                dst.back(),
                node
            ))
        {
            return;
        }

        dst.push_back(
            node
        );
    }
//Helper
    std::vector<PipeNode>
        buildCompletePrimaryOutputNodes() const
    {
        std::vector<PipeNode> completeNodes;

        // The currently positioned straight section is closest
        // to the machine entry.
        std::vector<PipeNode> positionedNodes =
            buildCurrentPositionedStraightNodes();

        for (const PipeNode& node : positionedNodes)
        {
            appendNodeNoDuplicate(
                completeNodes,
                node
            );
        }

        // Previously formed/frozen geometry follows downstream.
        for (const PipeNode& node : state.frozenNodes)
        {
            appendNodeNoDuplicate(
                completeNodes,
                node
            );
        }

        return completeNodes;
    }



//helper
    std::vector<PipeNode> buildCurrentPositionedStraightNodes() const
    {
        std::vector<PipeNode> result;

        if (state.positionedStraight.length <= 0.0)
            return result;

        if (ds <= 1e-9)
            return result;

        Frame startFrame =
            getPositionedStraightStartFrame();

        int steps =
            std::max(
                1,
                static_cast<int>(
                    std::ceil(
                        state.positionedStraight.length / ds
                    )
                    )
            );

        double stepLength =
            state.positionedStraight.length
            / static_cast<double>(steps);

        Vec3D direction =
            startFrame.T.normalized();

        for (int i = 0; i <= steps; ++i)
        {
            double arcDistance =
                stepLength
                * static_cast<double>(i);

            PipeNode node;

            node.pos =
                startFrame.P
                + direction * arcDistance;

            node.T = startFrame.T;
            node.N = startFrame.N;
            node.B = startFrame.B;

            result.push_back(
                node
            );
        }

        return result;
    }



    //helper
 


    double calculateAvailablePrimaryOutputLength() const
    {
        std::vector<PipeNode> completeNodes =
            buildCompletePrimaryOutputNodes();

        return calculateNodeListArcLength(
            completeNodes
        );
    }
    // =====================================================
// BEND START FRAME OWNERSHIP
//
// This helper is intentionally separate.
//
// Current rotary playback:
//     bend starts at machineEntryFrame.
//
// Future manufacturing history / additional forming:
//     bend may start at AdditionalFormingPass.entryFrame.
//
// Keeping this as a helper prevents processBend() from
// hardcoding one machine model forever.
// =====================================================
    Frame getBendStartFrame() const
    {
        // =====================================================
        // BEND START FRAME SELECTION
        //
        // Current behavior:
        //     normal rotary draw bending starts at machineEntryFrame.
        //
        // Future:
        //     additional forming pass may start from
        //     AdditionalFormingPass.entryFrame.
        // =====================================================

        return machineEntryFrame;
    }

    bool getFrozenEndFrame(
        Frame& outFrame
    ) const
    {
        if (state.frozenNodes.empty())
            return false;

        outFrame =
            frameFromNode(
                state.frozenNodes.back()
            );

        return true;
    }

    Frame frameFromNode(
        const PipeNode& node
    ) const
    {
        Frame frame;

        frame.P = node.pos;
        frame.T = node.T;
        frame.N = node.N;
        frame.B = node.B;

        return frame;
    }

//========================================================
    void printManufacturingSnapshot(
        const char* label
    ) const
    {
        if (!debugSnapshot)
            return;

        std::cout << "[MFG SNAPSHOT] "
            << label
            << " incoming="
            << state.incomingStock.remainingLength
            << " positioned="
            << state.positionedStraight.length
            << " trace="
            << state.currentBendTraceNodes.size()
            << " active="
            << state.activeZone.localNodes.size()
            << " frozen="
            << state.frozenNodes.size()
            << std::endl;
    }




    void resetFrames()
    {
        // =====================================================
        // MACHINE ENTRY FRAME
        //
        // Fixed machine entry / die reference.
        // Incoming stock is behind this frame.
        // =====================================================

        machineEntryFrame.P = { 0.0, 0.0, 0.0 };
        machineEntryFrame.T = { 1.0, 0.0, 0.0 };
        machineEntryFrame.N = { 0.0, 1.0, 0.0 };
        machineEntryFrame.B = { 0.0, 0.0, 1.0 };

        // =====================================================
        // CURRENT MATERIAL FRAME
        //
        // At reset, material frame starts at machine entry.
        // =====================================================

        currentFrame = machineEntryFrame;
    }



    void freezeActiveZone()
    {
        std::vector<PipeNode> oldFrozen =
            state.frozenNodes;

        std::vector<PipeNode> newFrozen;

        // =====================================================
        // 1. Current bend trace becomes frozen geometry.
        // =====================================================

        for (const auto& node : state.currentBendTraceNodes)
        {
            appendNodeNoDuplicate(
                newFrozen,
                node
            );
        }

        // =====================================================
        // 2. Remaining positioned straight becomes frozen too.
        // =====================================================

        std::vector<PipeNode> positionedFrozen =
            buildPositionedStraightFrozenNodes(
                state.activeZone.frame,
                state.positionedStraight.length
            );

        for (const auto& node : positionedFrozen)
        {
            appendNodeNoDuplicate(
                newFrozen,
                node
            );
        }

        // =====================================================
        // 3. Existing frozen body follows after that.
        // =====================================================

        for (const auto& node : oldFrozen)
        {
            appendNodeNoDuplicate(
                newFrozen,
                node
            );
        }

        state.frozenNodes =
            newFrozen;

        // =====================================================
        // Sync legacy currentFrame while PipeAxis3D still owns it.
        // =====================================================

        Frame frozenEndFrame;

        if (getFrozenEndFrame(frozenEndFrame))
        {
            currentFrame =
                frozenEndFrame;
        }
        else
        {
            currentFrame =
                state.activeZone.frame;
        }

        state.currentBendTraceNodes.clear();

        state.positionedStraight.length = 0.0;
        state.positionedStraight.nodes.clear();

        state.activeZone.localNodes.clear();
        state.activeZone.active = false;

        if (debugFreeze)
        {
            std::cout << "[MFG SIM FREEZE ACTIVE ZONE] frozenNodes="
                << state.frozenNodes.size()
                << std::endl;
        }
    }



    std::vector<PipeNode> buildPositionedStraightFrozenNodes(
        const Frame& startFrame,
        double length) const
    {
        std::vector<PipeNode> result;

        if (length <= 0.0)
            return result;

        if (ds <= 1e-9)
            return result;

        int stepCount =
            std::max(
                1,
                static_cast<int>(std::ceil(length / ds))
            );

        double stepLength =
            length / static_cast<double>(stepCount);

        Vec3D dir =
            startFrame.T.normalized();

        for (int i = 1; i <= stepCount; ++i)
        {
            double s =
                stepLength * static_cast<double>(i);

            PipeNode node;

            node.pos =
                startFrame.P + dir * s;

            node.T = startFrame.T;
            node.N = startFrame.N;
            node.B = startFrame.B;

            result.push_back(node);
        }

        return result;
    }


    void transformFrozenGeometryBetweenFrames(
        const Frame& oldFrame,
        const Frame& newFrame)
    {
        for (auto& node : state.frozenNodes)
        {
            transformNodeBetweenFrames(
                node,
                oldFrame,
                newFrame
            );
        }
    }

    void transformNodeBetweenFrames(
        PipeNode& node,
        const Frame& oldFrame,
        const Frame& newFrame)
    {
        node.pos =
            transformPointBetweenFrames(
                node.pos,
                oldFrame,
                newFrame
            );

        node.T =
            transformDirectionBetweenFrames(
                node.T,
                oldFrame,
                newFrame
            );

        node.N =
            transformDirectionBetweenFrames(
                node.N,
                oldFrame,
                newFrame
            );

        node.B =
            transformDirectionBetweenFrames(
                node.B,
                oldFrame,
                newFrame
            );
    }

    Vec3D transformDirectionBetweenFrames(
        const Vec3D& dir,
        const Frame& oldFrame,
        const Frame& newFrame) const
    {
        double x =
            dot(dir, oldFrame.T.normalized());

        double y =
            dot(dir, oldFrame.N.normalized());

        double z =
            dot(dir, oldFrame.B.normalized());

        return (
            newFrame.T.normalized() * x
            + newFrame.N.normalized() * y
            + newFrame.B.normalized() * z
            ).normalized();
    }

    Vec3D transformPointBetweenFrames(
        const Vec3D& point,
        const Frame& oldFrame,
        const Frame& newFrame) const
    {
        Vec3D r =
            point - oldFrame.P;

        double x =
            dot(r, oldFrame.T.normalized());

        double y =
            dot(r, oldFrame.N.normalized());

        double z =
            dot(r, oldFrame.B.normalized());

        return newFrame.P
            + newFrame.T.normalized() * x
            + newFrame.N.normalized() * y
            + newFrame.B.normalized() * z;
    }
    void appendNodeNoDuplicate(
        std::vector<PipeNode>& dst,
        const PipeNode& node)
    {
        if (!dst.empty() && nearlySamePoint(dst.back(), node))
            return;

        dst.push_back(node);
    }

    bool nearlySamePoint(
        const PipeNode& a,
        const PipeNode& b,
        double eps = 1e-6) const
    {
        Vec3D d = a.pos - b.pos;
        return d.lengthSquared() < eps * eps;
    }
    void moveFrozenGeometryDuringFeed(double distance)
    {
        if (distance <= 0.0)
            return;

        const auto& entryFrame =
            machineEntryFrame;

        Vec3D offset =
            entryFrame.T.normalized() * distance;

        for (auto& node : state.frozenNodes)
            node.pos += offset;

        for (auto& node : state.currentBendTraceNodes)
            node.pos += offset;

        for (auto& node : state.activeZone.localNodes)
            node.pos += offset;
    }

private:
//Helpers
    DeformableRegionSelection selectNodesByArcLengthRange(
        const std::vector<PipeNode>& sourceNodes,
        const DeformableRegion& region
    ) const
    {
        DeformableRegionSelection result;

        if (!region.isValid())
            return result;

        if (sourceNodes.size() < 2)
            return result;

        result.sourceArcLength =
            calculateNodeListArcLength(
                sourceNodes
            );

        // The complete requested region must exist
        // inside the source manufactured geometry.
        if (region.startArcLength
        > result.sourceArcLength
            || region.endArcLength
        > result.sourceArcLength)
        {
            return result;
        }

        double cumulativeArcLength =
            0.0;

        for (size_t i = 0; i < sourceNodes.size(); ++i)
        {
            if (i > 0)
            {
                cumulativeArcLength +=
                    distanceBetweenNodes(
                        sourceNodes[i - 1],
                        sourceNodes[i]
                    );
            }

            const PipeNode& node =
                sourceNodes[i];

            if (cumulativeArcLength < region.startArcLength)
            {
                result.beforeNodes.push_back(
                    node
                );
            }
            else if (cumulativeArcLength <= region.endArcLength)
            {
                result.selectedNodes.push_back(
                    node
                );
            }
            else
            {
                result.afterNodes.push_back(
                    node
                );
            }
        }

        result.selectedStartArcLength =
            region.startArcLength;

        result.selectedEndArcLength =
            region.endArcLength;

        result.valid =
            !result.selectedNodes.empty();

        return result;
    }



    double calculateNodeListArcLength(
        const std::vector<PipeNode>& nodes
    ) const
    {
        if (nodes.size() < 2)
            return 0.0;

        double totalLength =
            0.0;

        for (size_t i = 1; i < nodes.size(); ++i)
        {
            totalLength +=
                distanceBetweenNodes(
                    nodes[i - 1],
                    nodes[i]
                );
        }

        return totalLength;
    }



    double distanceBetweenNodes(
        const PipeNode& a,
        const PipeNode& b
    ) const
    {
        return (
            b.pos - a.pos
            ).length();
    }


    void updateActiveZone(double dA)
    {
        if (!state.activeZone.active)
            return;

        if (dA <= 0.0)
            return;

        Frame& frame =
            state.activeZone.frame;

        double radius =
            1.0 / state.activeZone.curvature;

        double signedDA =
            dA * bendDirectionSign(state.activeZone.direction);

        Vec3D oldT =
            frame.T.normalized();

        Vec3D oldN =
            frame.N.normalized();

        Vec3D oldB =
            frame.B.normalized();

        Vec3D center =
            frame.P
            + oldN * radius * bendDirectionSign(state.activeZone.direction);

        Vec3D radial =
            frame.P - center;

        Vec3D newRadial =
            rotateAroundAxis(
                radial,
                oldB,
                signedDA
            );

        frame.P =
            center + newRadial;

        frame.T =
            rotateAroundAxis(
                oldT,
                oldB,
                signedDA
            ).normalized();

        frame.N =
            rotateAroundAxis(
                oldN,
                oldB,
                signedDA
            ).normalized();

        frame.B =
            oldB;

        orthonormalizeFrame(frame);

        state.activeZone.accumulatedAngle += dA;

        PipeNode node;

        node.pos = frame.P;
        node.T = frame.T;
        node.N = frame.N;
        node.B = frame.B;

        state.activeZone.localNodes.push_back(node);
        state.currentBendTraceNodes.push_back(node);

        maintainActiveWindow();

        if (debugActiveZoneStep)
        {
            std::cout << "[MFG SIM ACTIVE ZONE] angle="
                << state.activeZone.accumulatedAngle
                << " / "
                << state.activeZone.targetAngle
                << " activeNodes="
                << state.activeZone.localNodes.size()
                << " traceNodes="
                << state.currentBendTraceNodes.size()
                << " frozenNodes="
                << state.frozenNodes.size()
                << std::endl;
        }
    }


    void maintainActiveWindow()
{
    if (ds <= 1e-9)
        return;

    size_t maxActiveNodes =
        static_cast<size_t>(
            std::ceil(state.activeZone.activeLength / ds)
        );

    maxActiveNodes =
        std::max<size_t>(2, maxActiveNodes);
    if (debugActiveWindow)
    {
        std::cout
            << "[MFG ACTIVE WINDOW] "
            << "activeLength="
            << state.activeZone.activeLength
            << " ds="
            << ds
            << " maxNodes="
            << maxActiveNodes
            << " currentNodes="
            << state.activeZone.localNodes.size()
            << std::endl;
    }
    while (state.activeZone.localNodes.size() > maxActiveNodes)
    {
        releaseOldestActiveNode();
    }
}

    void releaseOldestActiveNode()
    {
        if (state.activeZone.localNodes.empty())
            return;

        state.activeZone.localNodes.erase(
            state.activeZone.localNodes.begin()
        );
    }

    Frame makePositionedStraightEndFrame(
        const Frame& startFrame,
        double length) const
    {
        Frame result = startFrame;

        result.P =
            startFrame.P
            + startFrame.T.normalized() * length;

        return result;
    }

    void beginBendFromFrame(
        const Frame& startFrame,
        double radius,
        double targetAngle,
        BendDirection bendDirection)
    {
        state.activeZone.active = true;

        state.activeZone.frame = startFrame;

        state.activeZone.curvature = 1.0 / radius;
        state.activeZone.accumulatedAngle = 0.0;
        state.activeZone.targetAngle = targetAngle;
        state.activeZone.direction = bendDirection;

        state.activeZone.localNodes.clear();
        state.currentBendTraceNodes.clear();

        PipeNode startNode;

        startNode.pos = startFrame.P;
        startNode.T = startFrame.T;
        startNode.N = startFrame.N;
        startNode.B = startFrame.B;

        state.activeZone.localNodes.push_back(startNode);
        state.currentBendTraceNodes.push_back(startNode);

        std::cout << "[MFG SIM BEGIN BEND] R="
            << radius
            << " targetDeg="
            << targetAngle * 180.0 / PI
            << " dir="
            << bendDirectionToString(bendDirection)
            << std::endl;
    }

	//===================================================== 
    private:
        double ds = 0.5;

        RotationKinematicMode rotationMode =
            RotationKinematicMode::PipeRoll;

        Frame machineEntryFrame;
        // Compatibility/cache frame.
//
// When frozen geometry exists, this should mirror the
// end frame of frozenNodes.
//
// Do not treat this as the primary source of manufactured
// geometry state. Prefer frozenNodes / getFrozenEndFrame()
// when possible.
        Frame currentFrame;

        ManufacturingState state;
       //PipeAxis3D axis;

        std::vector<PipeNode> renderNodes;
//Helpers
        Frame getPositionedStraightStartFrame() const
        {
            if (state.activeZone.active)
                return state.activeZone.frame;

            return machineEntryFrame;
        }

    Vec3D rotateAroundAxis(
        const Vec3D& v,
        const Vec3D& axis,
        double angle) const
    {
        Vec3D k = axis.normalized();

        if (k.lengthSquared() < 1e-12)
            return v;

        return v * std::cos(angle)
            + cross(k, v) * std::sin(angle)
            + k * dot(k, v) * (1.0 - std::cos(angle));
    }
    void orthonormalizeFrame(Frame& frame) const
    {
        frame.T = frame.T.normalized();

        frame.N =
            (frame.N - frame.T * dot(frame.N, frame.T)).normalized();

        frame.B =
            cross(frame.T, frame.N).normalized();

        frame.N =
            cross(frame.B, frame.T).normalized();
    }

    Vec3D rotatePointAroundMachineAxis(
        const Vec3D& point,
        double angle) const
    {
        Vec3D axisPoint =
            machineEntryFrame.P;

        Vec3D axisDir =
            machineEntryFrame.T.normalized();

        Vec3D relative =
            point - axisPoint;

        Vec3D rotated =
            rotateAroundAxis(
                relative,
                axisDir,
                angle
            );

        return axisPoint + rotated;
    }

    void rotateNodeAroundMachineAxis(
        PipeNode& node,
        double angle)
    {
        Vec3D axisDir =
            machineEntryFrame.T.normalized();

        node.pos =
            rotatePointAroundMachineAxis(
                node.pos,
                angle
            );

        node.T =
            rotateAroundAxis(
                node.T,
                axisDir,
                angle
            ).normalized();

        node.N =
            rotateAroundAxis(
                node.N,
                axisDir,
                angle
            ).normalized();

        node.B =
            rotateAroundAxis(
                node.B,
                axisDir,
                angle
            ).normalized();
    }

    void rotateNodeListAroundMachineAxis(
        std::vector<PipeNode>& list,
        double angle)
    {
        for (auto& node : list)
        {
            rotateNodeAroundMachineAxis(
                node,
                angle
            );
        }
    }


    void rotateFrameAroundMachineAxis(
        Frame& frame,
        double angle)
    {
        Vec3D axisDir =
            machineEntryFrame.T.normalized();

        frame.P =
            rotatePointAroundMachineAxis(
                frame.P,
                angle
            );

        frame.T =
            rotateAroundAxis(
                frame.T,
                axisDir,
                angle
            ).normalized();

        frame.N =
            rotateAroundAxis(
                frame.N,
                axisDir,
                angle
            ).normalized();

        frame.B =
            rotateAroundAxis(
                frame.B,
                axisDir,
                angle
            ).normalized();

        orthonormalizeFrame(frame);
    }

    void syncCurrentFrameFromFrozen()
    {
        Frame frozenEndFrame;

        if (getFrozenEndFrame(frozenEndFrame))
        {
            currentFrame =
                frozenEndFrame;
        }
    }

    void rotatePipeBodyAroundMachineAxis(double angle)
{
    rotateNodeListAroundMachineAxis(
        state.frozenNodes,
        angle
    );

    rotateNodeListAroundMachineAxis(
        state.currentBendTraceNodes,
        angle
    );

    rotateNodeListAroundMachineAxis(
        state.activeZone.localNodes,
        angle
    );

    rotateNodeListAroundMachineAxis(
        state.positionedStraight.nodes,
        angle
    );

    rotateFrameAroundMachineAxis(
        currentFrame,
        angle
    );

    if (state.activeZone.active)
    {
        rotateFrameAroundMachineAxis(
            state.activeZone.frame,
            angle
        );
    }

    syncCurrentFrameFromFrozen();
}

    void rotateToolPlaneAroundMachineAxis(double angle)
    {
        Vec3D axisDir =
            machineEntryFrame.T.normalized();

        if (axisDir.lengthSquared() < 1e-12)
            return;

        machineEntryFrame.N =
            rotateAroundAxis(
                machineEntryFrame.N,
                axisDir,
                angle
            ).normalized();

        machineEntryFrame.B =
            rotateAroundAxis(
                machineEntryFrame.B,
                axisDir,
                angle
            ).normalized();

        orthonormalizeFrame(machineEntryFrame);
    }

    void buildIncomingStock()
    {
        if (!state.incomingStock.visible)
            return;

        if (state.incomingStock.remainingLength <= 0.0)
            return;

        if (ds <= 1e-9)
            return;

        const Frame& entry =
            machineEntryFrame;

        int steps =
            std::max(
                1,
                static_cast<int>(
                    std::ceil(state.incomingStock.remainingLength / ds)
                    )
            );

        double stepLength =
            state.incomingStock.remainingLength /
            static_cast<double>(steps);

        Vec3D dir =
            entry.T.normalized();

        // Incoming stock is behind the machine entry.
        for (int i = steps; i >= 0; --i)
        {
            double s =
                stepLength * static_cast<double>(i);

            PipeNode node;

            node.pos =
                entry.P - dir * s;

            node.T = entry.T;
            node.N = entry.N;
            node.B = entry.B;

            state.renderData.incomingStockNodes.push_back(node);
        }
    }
    void buildPositionedStraight()
    {
        if (!state.positionedStraight.visible)
            return;

        if (state.positionedStraight.length <= 0.0)
            return;

        if (ds <= 1e-9)
            return;

        Frame startFrame =
            getPositionedStraightStartFrame();

        int steps =
            std::max(
                1,
                static_cast<int>(
                    std::ceil(state.positionedStraight.length / ds)
                    )
            );

        double stepLength =
            state.positionedStraight.length /
            static_cast<double>(steps);

        Vec3D dir =
            startFrame.T.normalized();

        state.positionedStraight.nodes.clear();

        for (int i = 0; i <= steps; ++i)
        {
            double s =
                stepLength * static_cast<double>(i);

            PipeNode node;

            node.pos =
                startFrame.P + dir * s;

            node.T = startFrame.T;
            node.N = startFrame.N;
            node.B = startFrame.B;

            state.renderData.positionedStraightNodes.push_back(node);
            state.positionedStraight.nodes.push_back(node);
        }
    }

    void buildManufacturingRenderData()
    {
        state.renderData.clear();

        buildIncomingStock();
        buildPositionedStraight();

        state.renderData.currentBendTraceNodes =
            state.currentBendTraceNodes;

        state.renderData.activeZoneNodes =
            state.activeZone.localNodes;

        state.renderData.frozenNodes =
            state.frozenNodes;

        if (debugRenderData)
        {
            std::cout << "[MFG SIM RENDER DATA] "
                << "incoming=" << state.renderData.incomingStockNodes.size()
                << " positioned=" << state.renderData.positionedStraightNodes.size()
                << " trace=" << state.renderData.currentBendTraceNodes.size()
                << " active=" << state.renderData.activeZoneNodes.size()
                << " frozen=" << state.renderData.frozenNodes.size()
                << std::endl;
        }
    }

    // =====================================================
// LEGACY MANUFACTURING RENDER FLATTENING
//
// ManufacturingRenderData already stores the correct
// process-aware separated zones:
//
//     incomingStockNodes
//     positionedStraightNodes
//     currentBendTraceNodes
//     activeZoneNodes
//     frozenNodes
//
// This flattening helper exists only for legacy paths
// that still expect one continuous renderNodes list.
//
// New rendering code should prefer the separated zone
// data directly.
//
// Do not add manufacturing logic here.
// Do not reconstruct zones here.
// ManufacturingPipeSimulator owns zone generation.
// Renderer owns drawing only.
// =====================================================
    // =====================================================
// LEGACY FLATTEN ORDER
//
// Keep this order aligned with GLView manufacturing draw order:
//
// 1. incoming stock
// 2. positioned straight
// 3. frozen geometry
// 4. current bend trace
// 5. active zone last
// =====================================================
    void flattenManufacturingRenderData()
    {
        renderNodes.clear();

        appendZoneNodesToRenderNodes(
            state.renderData.incomingStockNodes
        );

        appendZoneNodesToRenderNodes(
            state.renderData.positionedStraightNodes
        );

        appendZoneNodesToRenderNodes(
            state.renderData.frozenNodes
        );

        appendZoneNodesToRenderNodes(
            state.renderData.currentBendTraceNodes
        );

        appendZoneNodesToRenderNodes(
            state.renderData.activeZoneNodes
        );
    }
    // helper
    void appendZoneNodesToRenderNodes(
        const std::vector<PipeNode>& zoneNodes
    )
    {
        for (const auto& node : zoneNodes)
        {
            renderNodes.push_back(
                node
            );
        }
    }

    bool isValidFrame(
        const Frame& frame
    ) const
    {
        return frame.T.lengthSquared() > 1e-12
            && frame.N.lengthSquared() > 1e-12
            && frame.B.lengthSquared() > 1e-12;
    }

    bool isSupportedAdditionalPassProcess(
        TubeFormingProcessType type
    ) const
    {
        switch (type)
        {
        case TubeFormingProcessType::HelixForming:
        case TubeFormingProcessType::StretchBending:
        case TubeFormingProcessType::TwoRollerContinuous:
            return true;

        default:
            return false;
        }
    }

};