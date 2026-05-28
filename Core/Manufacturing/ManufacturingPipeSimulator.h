#pragma once

#include <algorithm>
#include <iostream>

#include "Core/PipeAxis3D.h"
#include "Core/Manufacturing/ManufacturingState.h"
#include "Core/BendDirection.h"

class ManufacturingPipeSimulator
{
public:
    ManufacturingPipeSimulator()
        : ds(0.5),
        state(),
        axis(0.5, state)
    {
    }

    explicit ManufacturingPipeSimulator(double sampleStep)
        : ds(sampleStep),
        state(),
        axis(sampleStep, state)
    {
    }

    void reset()
    {
        state.clear();
        renderNodes.clear();

        axis.clear();
    }

    ManufacturingState& getState()
    {
        return state;
    }

    const ManufacturingState& getState() const
    {
        return state;
    }

    void processFeed(double distance)
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
            return;

        double actualFeed =
            std::min(distance, state.incomingStock.remainingLength);

        if (actualFeed <= 0.0)
            return;

        state.incomingStock.remainingLength -= actualFeed;
        state.incomingStock.consumedLength += actualFeed;

        if (state.incomingStock.remainingLength < 0.0)
            state.incomingStock.remainingLength = 0.0;

        state.positionedStraight.length += actualFeed;

        moveFrozenGeometryDuringFeed(actualFeed);

        std::cout << "[MFG SIM FEED] incomingRemaining="
            << state.incomingStock.remainingLength
            << " positionedStraight="
            << state.positionedStraight.length
            << " consumed="
            << state.incomingStock.consumedLength
            << std::endl;
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
                axis.getMachineEntryFrame(),
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
            axis.markGeometryDirty();
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

        std::cout << "[MFG SIM BEND] arcStep="
            << arcStepLength
            << " positionedStraightLeft="
            << state.positionedStraight.length
            << std::endl;

        axis.markGeometryDirty();
    }



    void processRotate(double signedAngle)
    {
        // =====================================================
        // ROTATE OPERATION
        //
        // OWNER:
        // ManufacturingPipeSimulator now owns manufacturing
        // rotate-state logic.
        //
        // PipeAxis3D still temporarily owns:
        // - machineEntryFrame
        // - currentFrame
        //
        // TWO MODES:
        //
        // PipeRoll:
        //     Pipe body rotates.
        //     Machine bend plane stays fixed.
        //
        // ToolHeadRotate:
        //     Pipe body stays fixed.
        //     Machine bend plane rotates.
        // =====================================================

        if (std::abs(signedAngle) < 1e-12)
            return;

        RotationKinematicMode mode =
            axis.getRotationKinematicMode();

        if (mode == RotationKinematicMode::PipeRoll)
        {
            rotatePipeBodyAroundMachineAxis(signedAngle);

            std::cout << "[MFG SIM ROTATE] PipeRoll angleDeg="
                << signedAngle * 180.0 / PI
                << std::endl;
        }
        else if (mode == RotationKinematicMode::ToolHeadRotate)
        {
            rotateToolPlaneAroundMachineAxis(signedAngle);

            std::cout << "[MFG SIM ROTATE] ToolHeadRotate angleDeg="
                << signedAngle * 180.0 / PI
                << std::endl;
        }
    }

    void reconstructVisiblePipe()
    {
        buildManufacturingRenderData();
        flattenManufacturingRenderData();

        std::cout << "[MFG SIM RECONSTRUCT] nodes="
            << renderNodes.size()
            << std::endl;
    }

    void setRotationKinematicMode(
        PipeAxis3D::RotationKinematicMode mode)
    {
        axis.setRotationKinematicMode(mode);
    }

    PipeAxis3D::RotationKinematicMode getRotationKinematicMode() const
    {
        return axis.getRotationKinematicMode();
    }

    const ManufacturingRenderData& getManufacturingRenderData() const
    {
        return state.renderData;
    }

    const std::vector<PipeNode>& getNodes() const
    {
        return renderNodes;
    }

    PipeAxis3D& legacyAxis()
    {
        return axis;
    }

    const PipeAxis3D& legacyAxis() const
    {
        return axis;
    }

private:




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

        if (!state.frozenNodes.empty())
        {
            const PipeNode& last =
                state.frozenNodes.back();

            Frame frame;

            frame.P = last.pos;
            frame.T = last.T;
            frame.N = last.N;
            frame.B = last.B;

            axis.setCurrentFrame(frame);
        }
        else
        {
            axis.setCurrentFrame(
                state.activeZone.frame
            );
        }

        state.currentBendTraceNodes.clear();

        state.positionedStraight.length = 0.0;
        state.positionedStraight.nodes.clear();

        state.activeZone.localNodes.clear();
        state.activeZone.active = false;

        std::cout << "[MFG SIM FREEZE ACTIVE ZONE] frozenNodes="
            << state.frozenNodes.size()
            << std::endl;
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
            axis.getMachineEntryFrame();

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
    double ds = 0.5;
    ManufacturingState state;
    PipeAxis3D axis;
    std::vector<PipeNode> renderNodes;
//Helpers
    Frame getPositionedStraightStartFrame() const
    {
        if (state.activeZone.active)
            return state.activeZone.frame;

        return axis.getMachineEntryFrame();
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
        const Frame& entry =
            axis.getMachineEntryFrame();

        Vec3D axisPoint =
            entry.P;

        Vec3D axisDir =
            entry.T.normalized();

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
        const Frame& entry =
            axis.getMachineEntryFrame();

        Vec3D axisDir =
            entry.T.normalized();

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
        const Frame& entry =
            axis.getMachineEntryFrame();

        Vec3D axisDir =
            entry.T.normalized();

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
        if (!state.frozenNodes.empty())
        {
            const PipeNode& last =
                state.frozenNodes.back();

            Frame frame;

            frame.P = last.pos;
            frame.T = last.T;
            frame.N = last.N;
            frame.B = last.B;

            axis.setCurrentFrame(frame);
        }
    }

    void rotatePipeBodyAroundMachineAxis(double angle)
    {
        // =====================================================
        // PIPE ROLL MODE
        //
        // Pipe body rotates around machineEntryFrame.T.
        // Machine bend plane stays fixed.
        // =====================================================

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

        Frame current =
            axis.getCurrentFrame();

        rotateFrameAroundMachineAxis(
            current,
            angle
        );

        axis.setCurrentFrame(current);

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
        // =====================================================
        // TOOL HEAD ROTATION MODE
        //
        // Pipe body stays fixed.
        // Machine bend plane rotates.
        // =====================================================

        Frame& entry =
            axis.getMachineEntryFrame();

        Vec3D axisDir =
            entry.T.normalized();

        if (axisDir.lengthSquared() < 1e-12)
            return;

        entry.N =
            rotateAroundAxis(
                entry.N,
                axisDir,
                angle
            ).normalized();

        entry.B =
            rotateAroundAxis(
                entry.B,
                axisDir,
                angle
            ).normalized();

        orthonormalizeFrame(entry);
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
            axis.getMachineEntryFrame();

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

        std::cout << "[MFG SIM RENDER DATA] "
            << "incoming=" << state.renderData.incomingStockNodes.size()
            << " positioned=" << state.renderData.positionedStraightNodes.size()
            << " trace=" << state.renderData.currentBendTraceNodes.size()
            << " active=" << state.renderData.activeZoneNodes.size()
            << " frozen=" << state.renderData.frozenNodes.size()
            << std::endl;
    }

    void flattenManufacturingRenderData()
    {
        renderNodes.clear();

        for (const auto& node : state.renderData.incomingStockNodes)
            renderNodes.push_back(node);

        for (const auto& node : state.renderData.positionedStraightNodes)
            renderNodes.push_back(node);

        for (const auto& node : state.renderData.currentBendTraceNodes)
            renderNodes.push_back(node);

        for (const auto& node : state.renderData.frozenNodes)
            renderNodes.push_back(node);

        for (const auto& node : state.renderData.activeZoneNodes)
            renderNodes.push_back(node);
    }


};