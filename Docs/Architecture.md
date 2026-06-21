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

    ============================================================
    Phase 7O-5 — Add visual insertion marker/debug frame

Goal:

Show insertion point visually in PlannedShapePreview.

No curve change.
No manufacturing playback change.
No process simulation change.

====================================================
Phase 7P — Add explicit insertion-start frame support

Goal:

At insertion point, capture the frame:
    P, T, N, B

This will later allow:
    helix starts with correct orientation
    inserted curve aligns to selected pipe frame

Right now we insert by curve order, but the inserted helix still uses its own default frame behavior. Next we need to make insertion frame-aware.

This is the bridge to:

InsertAtArcLength
    ?
find frame at s
    ?
start inserted helix from that frame

We should do this carefully in small steps.
======
ASCII diagram: frame lookup at arc length
Pipe curve sampled into nodes:

s=0                                                        s=end
?                                                          ?
?                                                          ?

N0 ---- N1 ---- N2 ---- N3 ---- N4 ---- N5 ---- N6 ---- N7
0     0.5    1.0    1.5    2.0    2.5    3.0    3.5   ... mm

                         targetS
                            ?
                            ?
                         s = 2.2

Nearest sampled node:

N4 at s=2.0     error = 0.2
N5 at s=2.5     error = 0.3

Chosen:
    N4

Returned frame:

Frame at N4:
    P = node.pos
    T = node.T
    N = node.N
    B = node.B

For your use case:

Rotary curve:

Line 198 mm
?
???????????????????????????????????????????????
0                                           198
                                             ?
                                             ?
                                         Arc begins

Arc length 31.416 mm
198 -------------------- 202 -------------------- 229.416
                          ?
                          ?
                    insertion target

Frame query returns:

P = position on bend
T = tangent of pipe at s=202
N = normal direction at s=202
B = binormal at s=202

That frame will later become:

start frame for inserted helix
Simplified analytic exact frame query example

Sample-based query:

sample curve into nodes
find nearest node
return that node frame

Analytic exact query:

find segment containing s
compute frame exactly inside that segment
without relying on nearest sampled node

Example for a line segment:

start frame:
    P0, T0, N0, B0

local arc length:
    localS

Exact frame:

P = P0 + T0 * localS
T = T0
N = N0
B = B0

C++-style:

Frame frameOnLine(
    const Frame& startFrame,
    double localS)
{
    Frame f = startFrame;

    f.P =
        startFrame.P
        + startFrame.T.normalized() * localS;

    return f;
}

Example for a circular arc:

Given:

radius R
local arc length localS
bend angle a = localS / R
bend direction sign = +1 or -1

Start frame:
    P0, T0, N0, B0

Rotate tangent around B:

T1 = rotateAroundAxis(T0, B0, sign * a)

Move position along arc approximately/exactly:

P1 = P0
     + T0 * (R * sin(a))
     + N0 * (sign * R * (1 - cos(a)))

Frame:
    P = P1
    T = T1
    N/B updated by transport or rotation

Simplified C++-style:

Frame frameOnCircularArc(
    const Frame& startFrame,
    double radius,
    double localS,
    BendDirection bendDirection)
{
    Frame f = startFrame;

    double sign =
        bendDirectionSign(bendDirection);

    double a =
        localS / radius;

    Vec3D T0 =
        startFrame.T.normalized();

    Vec3D N0 =
        startFrame.N.normalized();

    Vec3D B0 =
        startFrame.B.normalized();

    f.P =
        startFrame.P
        + T0 * (radius * std::sin(a))
        + N0 * (sign * radius * (1.0 - std::cos(a)));

    f.T =
        rotateAroundAxis(
            T0,
            B0,
            sign * a
        ).normalized();

    f.N =
        rotateAroundAxis(
            N0,
            B0,
            sign * a
        ).normalized();

    f.B =
        B0;

    return f;
}

So the future direction is:

Phase now:
    sample-based frame lookup

Later:
    analytic frame lookup per segment type:
        Line
        CircularArc
        Helix
        VariableCurvature
        ============================================
        Phase 7P-2 — Store insertion frame in preview model

Goal:

ManufacturingPlanPreviewModel
    finds insertion arc length
    queries frame at that location
    stores insertion Frame

This prepares:

inserted helix starts from selected frame
===========================================================
Phase 7P-3 — Draw insertion frame axes

Goal:

Show T / N / B axes at insertion point

This will visually confirm not only position, but
also orientation of the insertion frame.


====
7P-3 — Draw insertion frame axes

Goal:

At insertion point, draw:
    T axis
    N axis
    B axis

This verifies orientation, not only position.
===========================================================
Phase 7Q — Prepare inserted curve frame alignment

Goal:

Inserted helix should eventually start from insertion frame orientation:
    P, T, N, B

Small first step:

Add metadata:
    ManufacturingPass::resolvedStartFrame
    ManufacturingPass::hasResolvedStartFrame

No transform yet.

This prepares:

InsertAtArcLength
    ?
find frame
    ?
store resolved frame
    ?
next phase transforms inserted curve into that fram

=====
Insertion pass can store its resolved start frame:
    P, T, N, B

No curve transform yet.
No visual change expected.
============================================================
Phase 7R — Transform inserted curve to resolved frame

Goal:

Inserted helix curve should start from resolved insertion frame:
    P, T, N, B

Careful small step:

7R-1:
    add PipeCurveTransform helper
    transform sampled nodes first for visual validation

7R-2:
    later transform PipeCurveSegment start frame / curve itself

    ============================================================
    Phase 7R-2 — Visualize transformed inserted helix overlay

Goal:

Draw transformed inserted helix as a debug overlay.

This proves:
    local helix
        ?
    transformed into insertion frame
        ?
    visually follows selected T/N/B orientation
    ====
    Yes. Phase 7R-2 complete.

Confirmed:

[PLAN PREVIEW TRANSFORMED INSERT] localNodes=401 transformedNodes=401

and visually:

1. composed helix visible
2. transformed overlay helix visible

Suggested commit:

git commit -m "Visualize transformed inserted curve overlay"

Next phase:

Phase 7R-3 — Replace composed inserted curve with transformed sampled preview

Goal:

For PlannedShapePreview only:
    before curve nodes
    +
    transformed inserted helix nodes
    +
    after curve nodes

This means the preview will show the correct frame-aligned inserted helix, instead of the old default-frame inserted helix.

Important:

This will still be preview-node composition.
It will not yet transform PipeCurveSegment analytically.
ManufacturingPlayback unchanged.
============================================================     
Phase 7R-3 — Replace composed inserted curve with transformed sampled preview

Goal:

PlannedShapePreview nodes =
    before insertion nodes
    + transformed inserted helix nodes
    + after insertion nodes

This is still preview-only.

ManufacturingPlayback unchanged
CADPreview unchanged
PipeCurve segment model unchanged

============================================================


So you are rebuilding from the already-composed curve:

Line + old helix + rest

Then you add:

transformed helix

Result:

transformed helix at marker
+
old default helix near origin / wrong frame

That is why you see two helixes.

Correct rule

For transformed preview we must split the base curve before insertion, not the already-composed curve.

Correct structure:

base curve = rotary pass only

split base curve at s

preview nodes =
    base before
    +
    transformed inserted helix
    +
    base after

Not:

combined curve with old helix already inserted 

======================================
Phase 7S — Clean preview insertion code
Goal:
Move the node-rebuild logic out of build()
into a helper function.

ManufacturingPlanPreviewModel::build()
    becomes smaller

Insertion preview logic moves into helper functions.
============================================================
Phase 7T — Remove old composed insertion from preview path
PlannedShapePreview should not rely on:
    plan.buildCombinedCurve()
for InsertAtArcLength preview nodes
Why:
buildCombinedCurve() still inserts the pass in curve-segment form.
Then preview-node rebuild overrides it.

This works, but is confusing.
next clean up
If plan contains InsertAtArcLength:
    build preview directly from transformed-node path

Else:
    use normal combined curve sampling
    ===============================================
    Phase 7U — Clean temporary debug tests from
    Remove old phase test code from constructor
or move it behind a debug flag.
ops
sim.loadProgram(ops)
rotaryPass
helixPass
multiPassPlan
sim.getManufacturingPlanPreview().setPlan(...)
sim.setMode(...)
==============================================================
Phase 7W — Add preview debug flags

Preview logs become optional.
Default can stay ON for now.
Later you can switch them OFF.
==============================================================  
Phase 7X — Add preview mode selector helper

Goal:

Make AppController mode selection cleaner.
=============================================================
Phase 7Y — Add mode toggle action
Goal:Keyboard/action can switch between:

CADPreview
PlannedShapePreview
ManufacturingPlayback
==
CADPreview
    ?
PlannedShapePreview
    ?
ManufacturingPlayback
    ?
CADPreview
===
UserAction::ToggleSimulationMode
============================================================================
=============================================================================
Planned next phases
Phase 7Z
Add current simulation mode name to HUD.

Phase 8A
Clean AppController setup into helper functions:
buildTestOperations()
buildTestManufacturingPlan()
configureInitialMode()

Phase 8B
Add planned-shape preview settings:
show insertion marker
show insertion frame
show transformed insert debug

Phase 8C
Add InsertAtNodeIndex metadata support.
No real insertion yet, only lookup/test.

Phase 8D
Add InsertAtNodeIndex preview implementation.
Similar to InsertAtArcLength, but selected by sampled node index.

Phase 8E
Prepare ExplicitStartFrame insertion.
This will allow helix/shape to start from a manually supplied frame.
=============================================================================
general concept, the project has two separate but connected systems:

1. PlannedShapePreview
   final intended geometry

2. ManufacturingPlayback
   manufacturing-history-driven process simulator
   with four zones

   ============================
   The four-zone simulator is not secondary. It is the real process simulation core.
   tail / incoming material

IncomingStock
    ?
machineEntryFrame
    ?
PositionedStraight
    ?
ActiveZone
    ?
FrozenGeometry

head / completed pipe
=================
Manufacturing history
    FEED / ROTATE / BEND / HELIX / ROLL / MANUAL
        ?
ManufacturingPlayback
        ?
zone state update
        ?
render zones
        ?
collision/process checks


======
Whereas PlannedShapePreview is only:

ManufacturingPlan
    ?
final composed shape preview
===So the long-term concept should be:
PlannedShapePreview
    answers: "What final pipe shape do I want?"

ManufacturingPlayback
    answers: "How was this shape physically made through the machine?"


    ===For helix/inserted forming later, we should not only preview it. We will need:

HelixProcessPlayback
or
ContinuousFormingPlayback

that updates the same four-zone/history model.

==============================================================
Phase 7Z — Add simulation mode name to HUD
HUD shows current mode:

CADPreview
PlannedShapePreview
ManufacturingPlayback
================================================================
Phase 8B — Add preview debug visibility flags

Control debug visuals separately:
    insertion marker
    insertion frame axes
    transformed insert overlay
This will let you switch off helper visuals when needed while keeping the planned preview clean.
Control planned-preview debug visuals separately:

show insertion marker
show insertion frame axes
show transformed insert overlay
=============================================================
Phase 8C — Add InsertAtNodeIndex metadata test
Goal:
Prepare second insertion mode:

PassPlacement::atNodeIndex(index)

This will later allow insertion at a selected sampled node index, not just arc length.
No real insertion yet.
Only verify metadata and node lookup.

This will let you later say:
PassPlacement::atNodeIndex(10)

 Start helix at sampled node 404
 instead of:
 Useful for manual selection / mouse picking later.

 Goal:

PassPlacement::atNodeIndex(index)
    stores selected sampled node index

No real insertion yet.
No rendering change yet.

===============================================================
Phase 8D — Implement InsertAtNodeIndex preview
Goal:
helixPass.placement =
    PassPlacement::atNodeIndex(404);
    produces
    before ---- HELIX ---- after
    using the same safe transformed-node preview path.


    ================================================================
    Phase 8F — Prepare ExplicitStartFrame insertion

    Phase 8F will add support scaffolding only.We’ll prepare ExplicitStartFrame so later a
    pass can start from a manually supplied frame, but we won’t yet make it the active path
    unless you choose to test it.

Goal:PassPlacement::atFrame(frame)
    can resolve to a start frame

No real insertion behavior change yet.
No manufacturing playback change.

current workflow
helixPass.placement = atNodeIndex(404)
        ?
find base curve nodes
        ?
node index 404 -> arc length 201.989
        ?
find frame at arc length
        ?
split base curve
        ?
before + transformed helix + after

Explicit frame workflow

helixPass.placement = atFrame(myFrame)
        ?
use supplied frame directly
        ?
transform helix to that frame
        ?
nodes = transformed helix only
        ?
print [PLAN PREVIEW EXPLICIT INSERT]
 data flow diagram :


PassPlacement
?
??? AppendToPrevious
?       ?
?   normal combined curve
?
??? InsertAtArcLength(s)
?       ?
?   base curve -> frame at s -> split at s
?       ?
?   before + transformed inserted pass + after
?
??? InsertAtNodeIndex(i)
?       ?
?   base curve nodes -> i -> arc length s
?       ?
?   same as InsertAtArcLength
?
??? ExplicitStartFrame(frame)
        ?
    use frame directly
        ?
    transformed inserted pass only

================================================================
Phase 8G — Add placement preset helper
Goal: switch between ArcLength, NodeIndex, and ExplicitFrame tests cleanly without
editing many code fragments.
Switch placement test mode in one place:

ArcLength
NodeIndex
ExplicitFrame

============================================================
Phase 8H — Add placement mode name to HUD

Goal:
P key cycles placement preset:

ArcLength
    ?
NodeIndex
    ?
ExplicitFrame
    ?
ArcLength

=========================================================================
For test frame

frame.P = { 200.0, 80.0, 0.0 };
frame.T = { 1.0, 0.0, 0.0 };
frame.N = { 0.0, 1.0, 0.0 };
frame.B = { 0.0, 0.0, 1.0 };

expected result:
helix starts at P = (200, 80, 0)
helix axis follows T = +X
helix radial orientation uses N/B


Diagram
world origin
(0,0,0)
   ?
   ?
   ?
   ??????????????????????????????? X
                    ^
                    |
              P=(200,80,0)
              helix starts here
              axis direction = T

    ExplicitFrame preview = helix only

    ===
    intentional on this stage:

    ArcLength:
    start on previous/base curve at distance s

NodeIndex:
    start on previous/base curve at sampled node i

ExplicitFrame:
    start at manually supplied independent Frame

    Later variants will allow:

    ExplicitFrameOnly
    show inserted pass only

ExplicitFrameAppendToBase
    base curve + inserted pass from explicit frame

ExplicitFrameWithAfter
    inserted pass + transformed continuation

    =======================================================
    Phase 8J — Clean placement toggle behavior
    Make placement toggle predictable and documented.
    We should clean/confirm:
    N = simulation mode toggle
M = LINE / MESH toggle
T = placement preset toggle
and add one console/HUD-safe preset name helper so logs are clearer:
[APP PLACEMENT PRESET] ArcLength
[APP PLACEMENT PRESET] NodeIndex
[APP PLACEMENT PRESET] ExplicitFrame

===============
==============================================================================
****************************************************************************************

Space = Play
P     = Pause
R     = Reset
S     = Step

M     = Toggle LINE / MESH
N     = Toggle simulation mode
T     = Toggle placement preset


## Simulation Modes

### CADPreview

Shows the ideal CAD pipe generated from the operation list.

Purpose:

- verify final geometric shape
- inspect clean operation-based pipe
- no manufacturing process simulation

Does not show:

- incoming stock
- active bending zone
- frozen geometry
- process history

---

### PlannedShapePreview

Shows the final planned multi-pass shape.

Example:

```text
rotary draw bending pass
    +
inserted helix pass

Purpose:

preview final intended shape
test pass placement
test helix / future forming passes
verify insertion point and insertion frame

This mode is CAD-like.
It is not manufacturing playback.


ManufacturingPlayback

Shows real process simulation through the machine.

This mode owns the four-zone model:

IncomingStock
    ?
machineEntryFrame
    ?
PositionedStraight
    ?
ActiveZone
    ?
FrozenGeometry

Purpose:

simulate manufacturing history
process FEED / ROTATE / BEND
show pipe movement through machine
later: collision checking and process validation

This is the real manufacturing simulator.


And this section:

```md
## Pass Placement Modes

### ArcLength

Starts inserted pass at a distance along the previous/base curve.

Example:

```cpp
PassPlacement::atArcLength(202.0);
Meaning:
Start inserted helix at s = 202 mm along the base pipe.
Use for precise distance-based insertion.

###NodeIndex

Starts inserted pass at a sampled pipe node.

Example:
PassPlacement::atNodeIndex(404);
Meaning:
Find sampled node 404.
Convert node index to arc length.
Insert pass at that position.

###ExplicitFrame

Starts inserted pass from a manually supplied frame.

Example:PassPlacement::atFrame(frame);

Meaning:

Use frame.P as start position.
Use frame.T as forward axis.
Use frame.N / frame.B as radial orientation.
Current behavior:

ExplicitFrame preview shows inserted pass only.
It does not attach before/after base curve yet.


****************************************************************************
============================================================================
=============================================================================


Phase 8M — Add placement debug visibility toggle

D toggles planned preview debug visuals:

marker
frame axes

==============================================================
Phase 8N — Add preview debug state to HUD

HUD shows whether planned preview debug visuals are ON/OFF.
DEBUG: ON
or
DEBUG: OFF
his helps when pressing D.
============================================================================
Phase 8R — Add active placement preset to AppController HUD data path

Goal:
HUD placement text should reflect AppController active preset directly:
ArcLength / NodeIndex / ExplicitFrame

=============================================================================
Phase 8T — Prepare explicit-frame attach modes
Prepare future behavior for ExplicitFrame:

1. InsertedOnly
   current behavior

2. AppendAfterFrame
   explicit frame + inserted pass only

3. AttachBaseAfterInsert
   before/base + inserted pass + after
