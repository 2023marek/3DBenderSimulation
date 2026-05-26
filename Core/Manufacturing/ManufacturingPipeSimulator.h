#pragma once

#include "Core/PipeAxis3D.h"

// =====================================================
// MANUFACTURING PIPE SIMULATOR
//
// PHASE 4D BRIDGE WRAPPER
//
// Current purpose:
//     Own the old PipeAxis3D manufacturing implementation.
//
// Future purpose:
//     Own real manufacturing state directly:
//
//         IncomingStock
//         PositionedStraight
//         ActiveZone
//         CurrentBendTrace
//         FrozenGeometry
//         ManufacturingRenderData
//
// Important:
//     This class should not change behavior yet.
//     It only prepares architecture.
// =====================================================

class ManufacturingPipeSimulator
{
public:
    ManufacturingPipeSimulator()
        : axis(0.5)
    {
    }

    explicit ManufacturingPipeSimulator(double sampleStep)
        : axis(sampleStep)
    {
    }

    void reset()
    {
        axis = PipeAxis3D(0.5);
    }

    // -----------------------------------------------------
    // Manufacturing commands
    // -----------------------------------------------------

    void processFeed(double distance)
    {
        axis.processFeed(distance);
    }

    void processBend(
        double radius,
        double targetAngle,
        double angleIncrement,
        BendDirection bendDirection)
    {
        axis.processBend(
            radius,
            targetAngle,
            angleIncrement,
            bendDirection
        );
    }

    void processRotate(double signedAngle)
    {
        axis.processRotate(signedAngle);
    }

    void reconstructVisiblePipe()
    {
        axis.reconstructVisiblePipe();
    }

    // -----------------------------------------------------
    // Machine kinematic mode
    // -----------------------------------------------------

    void setRotationKinematicMode(
        PipeAxis3D::RotationKinematicMode mode)
    {
        axis.setRotationKinematicMode(mode);
    }

    PipeAxis3D::RotationKinematicMode getRotationKinematicMode() const
    {
        return axis.getRotationKinematicMode();
    }

    // -----------------------------------------------------
    // Render data
    // -----------------------------------------------------

    const PipeAxis3D::ManufacturingRenderData&
        getManufacturingRenderData() const
    {
        return axis.getManufacturingRenderData();
    }

    const std::vector<PipeAxis3D::Node>& getNodes() const
    {
        return axis.getNodes();
    }

    // -----------------------------------------------------
    // Temporary legacy access
    //
    // Needed while GLView/AppController still expect PipeAxis3D.
    // Later we remove this.
    // -----------------------------------------------------

    PipeAxis3D& legacyAxis()
    {
        return axis;
    }

    const PipeAxis3D& legacyAxis() const
    {
        return axis;
    }

private:
    PipeAxis3D axis;
};
