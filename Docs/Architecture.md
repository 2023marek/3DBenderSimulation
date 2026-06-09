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
=========================================================
his is exactly why Phase 7A/7B was important. The shared rule:

Operation -> Curve Segment -> Sampler -> Nodes

is the correct foundation for many tube-forming systems.

What we should do now is expand the architecture vocabulary before refactoring GeometricPipeModel.

Key decision

Do not make every forming process a different node generator.

Instead:

Different forming machines
    ?
produce different curve / curvature segments
    ?
same sampler
    ?
same render nodes

So the architecture should become:

TubeFormingProcess
    ??? RotaryDrawBending
    ??? ThreeRollForming
    ??? TwoRollContinuousForming
    ??? MultiAxisRollBending
    ??? StretchBending
    ??? HelixForming

        ?

PipeCurve / PipeCurveSegment

        ?

PipeCurveSampler

        ?

PipeNode

This prevents models from falling apart.

Your systems mapped to curvature model
A. Rotary draw bending

Already mostly done.

FEED
ROTATE
BEND

Produces:

LineSegment
RotationOnly
CircularArcSegment

This is your current manufacturing system.

B. 3-roller forming section

This is not a discrete bend. It is usually better represented as:

VariableCurvatureSegment

or approximately:

long arc with curvature ?(s)

Output:

?(s)

where curvature changes depending on roller positions.

C. 2-roller continuous forming system

Good for:

arc
spiral
continuous curvature

This should become:

ConstantCurvatureSegment
VariableCurvatureSegment
HelixSegment

Important: this should not be modeled as many tiny BEND operations. It should be one continuous segment.

D. Multi-axis 3D roll bending

Future.

This becomes:

VariableCurvatureTorsionSegment

Meaning:

curvature ?(s)
torsion ?(s)

This is exactly why PipeCurveSegment already has:

curvature
torsion
curvatureSamples

Good direction.

E. Stretch bending

Yes, this is different because it has mechanical tension.

But geometrically, output can still be:

ArcSegment / FormWrappedSegment / HelixSegment

Physically, it needs:

strain
stretch ratio
springback model
clamp frames

So it should eventually be:

StretchBendingProcess
    input:
        clamp frames
        form radius / form curve
        tension
    output:
        PipeCurveSegment + physical metadata

For heating elements / helix:

HelixForming / StretchHelixForming

should output:

HelixSegment
    curvature ?
    torsion ?
    length

    ==============================================
    ManufacturingPass is a container for one forming stage.

It does not simulate anything yet. It only records:

which forming process was used
which operations belong to this pass
what curve came out of this pass

Example future flow:

Pass 1:
    RotaryDrawBending
    FEED / BEND / ROTATE / FEED / BEND
    output PipeCurve

Pass 2:
    StretchBending / HelixForming
    helix parameters
    output PipeCurve

Pass 3:
    ManualRework
    extra manual correction bend
    output PipeCurve

So add a lightweight skeleton.

==========================================
Phase 7C — Refactor GeometricPipeModel to use PipeCurve + PipeCurveSampler

Goal:

GeometricPipeModel:
    operations -> PipeCurve -> PipeCurveSampler -> nodes
    Phase 7C — Refactor GeometricPipeModel

    ==========================================
    Phase 7D — Add Helix segment test

Goal:

Prove the new curvature/torsion pipeline can generate a helix-like curve.

PipeCurveSegment::Helix
    ?
PipeCurveSampler
    ?
PipeNode

No manufacturing changes yet.

=================================================
Phase 7E — Add HelixOperation / helix curve producer skeleton

Goal:

Represent helix forming as a real forming command,
not just a temporary test.

=======================================================

Phase 7F — Upgrade HelixOperation to radius/pitch input
Upgrade HelixOperation to radius/pitch input

Goal:

HelixOperation
    helix radius + pitch + length
        ?
HelixCurveBuilder
        ?
curvature ? + torsion ?
        ?
PipeCurveSegment::Helix
        ?
PipeCurveSampler

No springback yet.
No material physics yet.
Only geometric helix + basic machine kinematics.
====================================================
Phase 7G — Add HelixFormingPass skeleton

Goal:

ManufacturingPass can represent helix forming as a real pass.

Not connected to manufacturing playback yet.

We will add:

HelixFormingPass / HelixPassBuilder

so later you can have:

Pass 1: RotaryDrawBending
Pass 2: HelixForming

and both output PipeCurve.
====================================================
Phase 7H — Add multi-pass ManufacturingPlan curve composition

That will allow:

RotaryDrawBending output curve
    +
HelixForming output curve
    ?
combined PipeCurve
    ?
PipeCurveSampler

====================================================
Current behavior:

Pass 1 curve
    ends here
Pass 2 helix
    starts immediately at Pass 1 end frame

So the spiral starts at the end of the previous pass.

To control insertion point later, we need one of these options:

1. Append mode
   helix starts at previous pass end

2. Insert-at-distance mode
   helix starts at arc length s along previous curve

3. Insert-at-node mode
   helix starts at selected PipeNode index

4. Insert-at-frame mode
   helix starts from explicit Frame

For now, Phase 7J should stay simple and use append mode. Then Phase 7K can add insertion control.

Phase 7J — Render ManufacturingPlanPreviewModel

Goal:

Show multi-pass preview:
    rotary draw bending + helix forming

This will prove visually:

ManufacturingPlan -> combined curve -> sampled nodes -> GLView
========================================================
++++++++++++++++++++++++++++++++++++++++++++++++++++++
REAL MACHINE THINKING

Physically:

straight tube stock enters machine ->  modify: here prebending tube stock

        ?
material enters bend zone  zone1
        ?
local deformation occurs    zone2
        ?
material exits bend zone
        ?
shape freezes                  zone3 
        |
  tube stock rigid part    zone4   




================================================================
OLD THINKING:

buildArc()
creates bend

NEW THINKING:

material continuously flows
through a deformation field
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
===============================================================

===\
    \
tail ===================> [ machineEntryFrame ] -----------> \\\\\\\\ )))))))========\Head incoming stock
                                                                                      \======>
       incoming stock              entry        positioned     active   frozen
                                                straight       zone     geometry
================================================================================================


## Simulation Modes

### CADPreview

Displays the final ideal pipe from the CAD operation list.

### PlannedShapePreview

Displays the final composed shape from a multi-pass manufacturing plan.

This mode is CAD-like. It does not simulate the manufacturing process.

It must not be used for:
- incoming stock
- positioned straight material
- active bend zone
- frozen geometry
- process collision simulation

### ManufacturingPlayback

Displays the real forming process.

This mode owns the four-zone manufacturing model:

```text
IncomingStock -> machineEntryFrame -> PositionedStraight -> ActiveZone -> FrozenGeometry
=================================================================================================

ManufacturingPass can describe WHERE its outputCurve should start.

For now:
    metadata only

No curve insertion logic yet.
No rendering change yet.
No playback change yet.

This prepares:

AppendToPrevious
InsertAtArcLength
InsertAtNodeIndex
ExplicitStartFrame

=======================================================
7O-1: Add curve length helpers.
7O-2: Add basic split-at-segment-boundary support.
7O-3: Add real split inside Line / CircularArc.
7O-4: Use it inside ManufacturingPlan::buildCombinedCurve().
==========================================================
Phase 7O-1 will only add helper functions for measuring and locating curve positions. We won’t split or insert anything yet, so rendering should remain unchanged.

Phase 7O-1 — Add curve length helpers

Goal:

PipeCurve can answer:
    total length
    which segment contains arc length s
    local distance inside that segment
    ==========================================================

    7O-2 will add basic split-at-segment-boundary support. This means if s falls exactly on a segment boundary, we can split the curve there and insert the new segment.
    Add curve split helpers for line segments

Goal:

Split PipeCurve at arc length s.

For now:
    only split inside Line segments.

If s falls inside an arc/helix/rotation:
    no real split yet
    fallback behavior for safety

This prepares:

beforeCurve + insertedPass + afterCurve
===========================================================
Phase 7O-4 — Add arc segment split support
====================================================
Goal:

InsertAtArcLength should also work inside circular arc segments.

Currently line split works.



Now we add circular arc split.

Example:

Line 198
Arc length 31.416
Insert at s = 202

202 is inside arc:
    localS = 4

Expected split:

before:
    Line 198
    Arc first part

insert:
    Helix

after:
    Arc remaining part
    Rotate
    Line