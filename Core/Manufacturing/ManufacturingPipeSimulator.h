#pragma once

#include "Core/PipeAxis3D.h"
#include "Core/Manufacturing/ManufacturingState.h"

class ManufacturingPipeSimulator
{
public:
    ManufacturingPipeSimulator()
        : state(),
        axis(0.5, state)
    {
    }

    explicit ManufacturingPipeSimulator(double sampleStep)
        : state(),
        axis(sampleStep, state)
    {
    }

    void reset()
    {
        state.clear();
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

    void setRotationKinematicMode(
        PipeAxis3D::RotationKinematicMode mode)
    {
        axis.setRotationKinematicMode(mode);
    }

    PipeAxis3D::RotationKinematicMode getRotationKinematicMode() const
    {
        return axis.getRotationKinematicMode();
    }

    const PipeAxis3D::ManufacturingRenderData&
        getManufacturingRenderData() const
    {
        return axis.getManufacturingRenderData();
    }

    const std::vector<PipeAxis3D::Node>& getNodes() const
    {
        return axis.getNodes();
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
    ManufacturingState state;
    PipeAxis3D axis;
};