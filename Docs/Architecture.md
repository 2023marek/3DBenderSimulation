1. Project Vision

2. Layer Architecture

3. Pipe System

4. Machine System

5. Rendering System

6. Manufacturing Model

7. Curvature-Driven Roadmap

8. Future Features
===============================================================
# 1. Project Vision
For Docs/Architecture.md:

# Pipe Bender Simulator Architecture

## 1. Core Principle

The simulator must be curvature-driven.

Forbidden direction:

```text
Operation -> Nodes

Preferred direction:

Operation -> Segments -> Nodes

Target direction:

Operation -> Curvature Segment -> Physics -> Sampling -> Render Nodes

Nodes are a discretization/rendering result, not the primary simulation model.

2. Current High-Level Structure
MainWindow
    ?
AppController
    ?
SimulationController
    ??? PipeSystem
    ?       ??? GeometricPipeModel
    ?       ??? ManufacturingPipeSimulator
    ?
    ??? MachineSystem
            ??? MachineModel
            ??? MachineRuntimeState
            ??? MachineController
            ??? MachineRenderData

GLView
    ??? PipeRenderer
    ??? MachineRenderer
    ??? HUDPanel
3. Pipe System
GeometricPipeModel

Purpose:

Ideal CAD pipe representation.
Operation history.
Rebuildable clean geometry.
No manufacturing zones.
No active bend physics.
ManufacturingPipeSimulator

Purpose:

Manufacturing playback.
Incoming stock.
Positioned straight.
Active bend zone.
Bend trace.
Frozen geometry.
FEED / ROTATE / BEND execution.
Future PhysicalPipeSimulator

Purpose:

Springback.
Material behavior.
Residual strain.
Physical corrections.
4. Machine System

Machine system owns machine state, not pipe geometry.

MachineModel          static machine/tooling data
MachineRuntimeState   dynamic operation state
MachineController     updates machine runtime state
MachineRenderData     read-only rendering snapshot

Rule:

MachineController does not modify pipe nodes.
ManufacturingPipeSimulator does not modify machine state.
SimulationController coordinates both.
5. Rendering System
GLView decides WHEN to draw.
Renderer classes decide HOW to draw.
Simulation objects do not call OpenGL.

Current renderers:

PipeRenderer
MachineRenderer
MachineReferenceRenderer
HUDPanel
6. Mesh / STL Assets

Imported STL data is neutral mesh data.

Core/Mesh/TriangleMesh
Core/Mesh/StlLoader

Machine parts reference STL assets through:

MachinePart::meshPath

Machine STL assets should be separate by part:

bend_die.stl
clamp_die.stl
pressure_die.stl
mandrel.stl
base_frame.stl
7. Future Curvature-Driven Layer

Planned structure:

Core/Curve
    PipeCurveSegment
    LineCurveSegment
    ArcCurveSegment
    HelixCurveSegment
    VariableCurvatureSegment

Core/Sampling
    PipeCurveSampler

Final target:

Operation
    ?
Curvature Segment
    ?
Physics Correction
    ?
Sampling
    ?
PipeNode render data
8. Important Architectural Rules
Do not put GLM/OpenGL inside Core geometry types.
Do not let renderers modify simulation.
Do not let MachineController modify pipe geometry.
Do not use nodes as the primary physics model.
Do not add features directly into GLView if they belong to a renderer/system.

For `Docs/Roadmap.md`:

```md
# Pipe Bender Simulator Roadmap

## Current Stable State

Completed:

```text
PipeAxis3D retired to LegacyPipeAxis3D.
PipeSystem introduced.
GeometricPipeModel handles CAD preview.
ManufacturingPipeSimulator handles manufacturing playback.
MachineSystem introduced.
MachineRuntimeState connected to FEED / ROTATE / BEND.
HUD displays machine runtime state.
MachineRenderData added.
MachineRenderer added.
STL loading added.
STL bend die rendering works.
Current Active Direction

Move toward curvature-driven simulation:

Operation -> Curvature Segment -> Physics -> Sampling -> Nodes
Phase 6L

AssetPathResolver:

Replace absolute STL paths with project-relative asset paths.

Status:

Pending / In progress
Phase 7A

Introduce curve segment model:

PipeCurveSegment
LineCurveSegment
ArcCurveSegment
HelixCurveSegment
VariableCurvatureSegment

Goal:

Separate simulation geometry from render nodes.
Phase 7B

Introduce pipe curve sampler:

PipeCurveSampler

Goal:

Convert curve segments into PipeNode lists for rendering.
Phase 7C

Refactor GeometricPipeModel:

operations -> curve segments -> sampler -> nodes
Phase 7D

Refactor ManufacturingPipeSimulator:

manufacturing state -> curvature segments -> sampling
Later Phases
PhysicalPipeSimulator
springback
material behavior
strain/residual state
CollisionSystem
pipe vs machine
pipe vs pipe
warnings
DimensionSystem
measurements
bend annotations
operation dimensions
Multi-pass Manufacturing
manual continuation
restart from selected frame
secondary machine pass
helix bending machine