#pragma once

#include <vector>

#include "OperationQueue.h"
#include "Core/Machine/MachineSystem.h"
#include "Core/Manufacturing/RotationKinematicMode.h"
#include "Core/PipeSystem.h"
#include "Core/Forming/ManufacturingPlanPreviewModel.h"


class SimulationController
{
public:
    enum class SimulationMode
    {
        CADPreview,

        // Final multi-pass planned shape.
        // This is NOT process playback.
        PlannedShapePreview,

        // Real manufacturing process simulation.
        // Uses incoming stock / positioned straight /
        // active zone / frozen geometry.
        ManufacturingPlayback
    };

public:
    SimulationController();

    void loadProgram(const std::vector<Operation>& ops);

    void play();
    void pause();
    void step();
    void reset();
    void update(double deltaTime);

    //Accessors

    ManufacturingPlanPreviewModel& getManufacturingPlanPreview()
    {
        return pipeSystem.planedShapePreview();
    }

    const ManufacturingPlanPreviewModel& getManufacturingPlanPreview() const
    {
        return pipeSystem.planedShapePreview();
    }

    // =====================================================
    // Rotation kinematic mode
    // =====================================================

    void setRotationKinematicMode(RotationKinematicMode mode)
    {
        rotationKinematicMode = mode;
        pipe().setRotationKinematicMode(mode);
    }

    RotationKinematicMode getRotationKinematicMode() const
    {
        return rotationKinematicMode;
    }
    //=====================================================
    // Public accessors
    // ===================================================
    //read access for UI/debug
    const MachineRuntimeState& getMachineRuntimeState() const
    {
        return machineSystem.getRuntimeState();
    }
    //=====================================================
    ManufacturingPipeSimulator& pipe();
    const ManufacturingPipeSimulator& pipe() const;

    MachineController& machine()
    {
        return machineSystem.getController();
    }

    const MachineController& machine() const
    {
        return machineSystem.getController();
    }
  
	//=====================================================
    MachineSystem& getMachineSystem()
    {
        return machineSystem;
    }

    const MachineSystem& getMachineSystem() const
    {
        return machineSystem;
    }
    // =====================================================
    // Simulation mode
    // =====================================================

    void setMode(SimulationMode newMode)
    {
        mode = newMode;
    }

    SimulationMode getMode() const
    {
        return mode;
    }

    // =====================================================
    // Machine state
    // =====================================================

    

    // =====================================================
    // Manufacturing legacy pipe access
    //
    // Used by GLView / old renderer code.
    // Returns old PipeAxis3D through ManufacturingPipeSimulator.
    // =====================================================

    

    // =====================================================
    // CAD pipe access
    // =====================================================

    GeometricPipeModel& getCadPipeGeometry()
    {
        return pipeSystem.cadPipe();
    }

    const GeometricPipeModel& getCadPipeGeometry() const
    {
        return pipeSystem.cadPipe();
    }

    // =====================================================
    // Manufacturing simulator access
    // =====================================================

    ManufacturingPipeSimulator& getManufacturingPipe()
    {
        return pipeSystem.manufacturingPipe();
    }

    const ManufacturingPipeSimulator& getManufacturingPipe() const
    {
        return pipeSystem.manufacturingPipe();
    }

    // =====================================================
    // Playback state
    // =====================================================

    bool isPlaying() const
    {
        return playing;
    }

    bool isPaused() const
    {
        return paused;
    }

    double getSpeed() const
    {
        return speed;
    }

    void setSpeed(double speedMmPerSec)
    {
        speed = speedMmPerSec;
    }

    void setRotationSpeedRadPerSec(double value)
    {
        if (value > 0.0)
            rotationSpeedRadPerSec = value;
    }

    double getRotationSpeedRadPerSec() const
    {
        return rotationSpeedRadPerSec;
    }

    double getCurrentOperationProgress() const;
    double getOverallProgress() const;

    size_t getCurrentOperationIndex() const
    {
        return operationQueue.getCurrentIndex();
    }

    size_t getTotalOperations() const
    {
        return operationQueue.getTotalOperations();
    }

    OperationQueue& getQueue();
    const OperationQueue& getQueue() const;

    PipeSystem& getPipeSystem();
    const PipeSystem& getPipeSystem() const;

    // Machine Getters
    MachineRenderData getMachineRenderData() const
    {
        return machineSystem.getRenderData();
    }


private:
    // =====================================================
    // Mode / speed
    // =====================================================

    double rotationSpeedRadPerSec = PI;

    SimulationMode mode =
        SimulationMode::ManufacturingPlayback;

    RotationKinematicMode rotationKinematicMode =
        RotationKinematicMode::PipeRoll;

    // =====================================================
    // Playback state
    // =====================================================

    bool playing = false;
    bool paused = false;

    double speed = 40.0;

    double accumulatedDistance = 0.0;
    double accumulatedAngle = 0.0;
    double accumulatedRotation = 0.0;

    // =====================================================
    // Core components
    // =====================================================

    OperationQueue operationQueue;
    //MachineState machineState;
    PipeSystem pipeSystem;
    MachineSystem machineSystem;
    // Full loaded program copy
    std::vector<Operation> loadedOperations;

private:
    // =====================================================
    // Internal execution
    // =====================================================

    void executeOperation(double deltaTime);

    void executeFeed(double distance);
    void executeBend(double angleIncrement);
    void executeRotate(double angleIncrement);

    void advanceToNextOperation();

    void updatePipeGeometryCAD();
    void updatePipeGeometryManufacturing();
};