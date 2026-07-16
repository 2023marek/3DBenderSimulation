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

   ====================================================
   Phase 8U — Add attach mode name to HUD
HUD shows explicit-frame attach mode:

ATTACH: InsertedOnly

===================================================
Phase 8V — Add explicit attach mode preset toggle
When placement preset is ExplicitFrame,
cycle attach mode:

InsertedOnly
AppendAfterFrame
AttachBaseAfterInsert

Suggested key:

A = attach mode toggle
======================================================
Phase 8W — Implement AppendAfterFrame behavior
Goal:
ExplicitFrame + AppendAfterFrame
    uses its own code path
========================================================
Phase 8X — Implement AttachBaseAfterInsert fallback as base + helix
Goal:
ExplicitFrame + AttachBaseAfterInsert
currently shows:

base curve + explicit-frame helix
No transformed after-curve yet.
expected:
rotary base pipe
+
separate helix starting at explicit frame
This helps prepare real base/insert attachment later.
ExplicitFrame + AttachBaseAfterInsert
    shows:
        base rotary curve
        +
        explicit-frame helix
No real attachment yet.
No after-curve transformation yet.

expected visual:
rotary base pipe

and separately:

helix starting at explicit frame P

==========================================================
Phase 8Y — Clean node count for preview strips
Goal:
HUD node count should be correct for both preview styles.
Current normal preview:
previewNodes:
    before ---- HELIX ---- after

HUD node count = previewNodes.size()
Disconnected preview:
previewNodeStrips:
    strip 0 = base pipe
    strip 1 = explicit helix

HUD should count:
    base nodes + helix nodes

Diagram:
Normal connected preview:

[ nodes ]
0 ---- 1 ---- 2 ---- 3 ---- 4 ---- 5

count = nodes.size()


Disconnected preview:

[ strip 0 ]        [ strip 1 ]
0---1---2---3      0---1---2

count = strip0.size + strip1.size

========================================================
Phase 8Z — Document preview node models
Goal:document the difference between

connected preview nodes
disconnected preview strips

Diagram:
Connected preview:

nodes:
before ---- HELIX ---- after

=====
Disconnected preview:

previewNodeStrips:
strip 0: base pipe

strip 1: explicit-frame helix


====
basic terms:
nodes
    one continuous drawable pipe path

previewNodeStrips
    multiple independent drawable pipe paths

strip
    one independent pipe path

bridge artifact
    unwanted connection between end of one shape and start of another
=============================================================================
Phase 9A — Prepare real AttachBaseAfterInsert behavior
Goal:
Move from preview-only fallback:

base strip
helix strip

toward real connected logic:

base before
    +
explicit-frame inserted pass
    +
after curve

====
First small step:
Phase 9A:
    add comments + placeholder helper only
    no behavior change

    Planed pipeflow:
Base curve
0 -------------------------------- end
          ^
          explicit frame target

Future:
base before ---- inserted pass ---- transformed base after
For Phase 9A, add helper stub:
bool applyExplicitAttachBaseAfterInsert(
    const ManufacturingPass& pass,
    const PipeCurve& baseCurve,
    const Frame& resolvedFrame)
{
    // TODO Phase 9B+
    // Real behavior:
    //
    // 1. Decide insertion position on base curve.
    // 2. Split base curve into before/after.
    // 3. Transform inserted pass to resolvedFrame.
    // 4. Transform after curve to inserted end frame.
    // 5. Rebuild preview nodes.
    //
    // Current behavior remains handled by disconnected strips.
    return false;
}
====================================================
Frame
    Local coordinate system attached to pipe.

Frame = P + T/N/B

P = position
T = tangent / forward direction of pipe
N = normal direction
B = binormal direction

Diagram:
                 N
                 ?
                 |
                 |
                 P ----? T
                /
               /
              B
P = where the inserted shape starts
T = direction the inserted shape grows forward
N/B = radial orientation around the pipe
======================================================================  

Meaning of resolvedFrame
resolvedFrame means:

the final concrete frame where insertion should start

It can come from different placement modes:
ArcLength
    s = 202 mm
    ?
    find frame on base curve at this distance
    ?
    resolvedFrame

NodeIndex
    nodeIndex = 404
    ?
    find node/frame at index
    ?
    resolvedFrame

ExplicitFrame
    user gives Frame directly
    ?
    resolvedFrame = user frame
=======================================================
Pipeflow:
    PassPlacement
      ?
      ?
resolvePlacementStartFrame(...)
      ?
      ?
resolvedFrame = P,T,N,B
      ?
      ?
inserted pass starts here

====================================================
Meaning of “transform”
In this project, transform means:
take geometry created in local/default coordinates
and place it into another frame in world coordinates

Local helix before transform:

local coordinates

P=(0,0,0)
T=+X

HELIX starts at origin:

0 ---- coil ---->

After transform to resolvedFrame:

world coordinates

resolvedFrame.P = insertion point
resolvedFrame.T = pipe tangent direction

HELIX starts at resolvedFrame.P:

base pipe ---- P ---- coil ---->

So this:
PipeCurveTransform::transformNodesToFrame(
    localInsertedNodes,
    resolvedFrame
);

means:

local inserted helix
    ?
move + rotate
    ?
helix starts at resolvedFrame

======================
Point 3 explained
// 3. Transform inserted pass to resolvedFrame.
//
// The inserted pass, for example a helix, is usually generated
// in its own local coordinate system:
//
//     local start position = (0, 0, 0)
//     local forward axis   = +X
//
// But the selected insertion location may be somewhere on an
// already existing pipe:
//
//     resolvedFrame.P = insertion point
//     resolvedFrame.T = pipe tangent direction
//     resolvedFrame.N = local normal direction
//     resolvedFrame.B = local binormal direction
//
// Transform means:
//     local helix coordinates
//         ->
//     world coordinates defined by resolvedFrame.
//
// Result:
//     first helix node starts at resolvedFrame.P
//     helix forward direction follows resolvedFrame.T
//     helix radial orientation uses resolvedFrame.N/B.

Diagram:

Before transform:

local helix:

(0,0,0) ---- HELIX ---->
   T=+X


Target insertion frame:

base pipe ---- P
               \
                T direction


After transform:

base pipe ---- P ---- HELIX ---->
               ^
               resolvedFrame.P
    =======================================

               Point 4 explained
Expanded comment:
// 4. Transform after curve to inserted end frame.
//
// After inserting a new pass, the remaining base curve cannot
// simply stay in its old position.
//
// Why?
//
// Because the inserted pass changes where the continuation
// should start.
//
// Correct flow:
//
//     base before
//         +
//     transformed inserted pass
//         +
//     after curve transformed to inserted end frame
//
// The end frame of the inserted pass becomes the new start frame
// for the remaining pipe geometry.
//
// This keeps the pipe continuous:
//
//     before ---- inserted ---- after
//
// instead of:
//
//     before ---- inserted
//     old after curve still somewhere else

Diagram:

Wrong:

before ---- HELIX

after curve still starts at old split frame
---- after


Correct:

before ---- HELIX ---- after
                  ^
                  inserted end frame
                  after curve starts here

More detailed pipeflow:
Base curve before insertion:

A ---------------- P ---------------- Z
                  split
                  
A -> P = before curve
P -> Z = after curve


Insert helix at P:

A ---------------- P ---- HELIX_END

Now after curve must be moved:

old after:
P ---------------- Z

transformed after:
HELIX_END -------- Z'

Final:
A ---- P ---- HELIX ---- HELIX_END ---- transformed after

====================================
Why this matters

Without step 4, you get disconnected or wrong geometry:

before ---- HELIX
P ---- old after

With step 4:

before ---- HELIX ---- after

This is exactly the same problem we already fixed earlier for:
ArcLength / NodeIndex insertion
where the after-curve had to be transformed to the helix end frame.

==============================

Again
Phase 9A — Prepare real AttachBaseAfterInsert behavior
Goal:
Move from preview-only fallback:

base strip
helix strip

toward real connected logic:

base before
    +
explicit-frame inserted pass
    +
after curve

===
First small step:
Phase 9A:
    add comments + placeholder helper only
    no behavior change

    Planned pipeflow:
    Base curve
0 -------------------------------- end
          ^
          explicit frame target

Future:
base before ---- inserted pass ---- transformed base after

========================================================
Phase 9B — Route AttachBaseAfterInsert into helper
Goal:
No behavior change yet.

ExplicitFrame + AttachBaseAfterInsert
    calls applyExplicitAttachBaseAfterInsert(...)


    =====================================================
    Phase 9C — Formalize AttachBaseAfterInsert behavior

Goal:
Make this behavior intentional:
ExplicitFrame + AttachBaseAfterInsert
    split base at s=0
    before empty
    inserted helix
    after full base curve transformed to helix end frame

    =========================================================
    Phase 9D — Reclassify insertion placement as planned-preview only
    Goal:
    ArcLength / NodeIndex / ExplicitFrame placement
belongs to PlannedShapePreview.

It is not real manufacturing playback
=====
## Planned placement vs real manufacturing

PassPlacement is a planned-preview concept.

It is used by `PlannedShapePreview` to compose and inspect intended geometry:

```text
base planned curve
    +
inserted planned pass

This is useful for CAD-like design and planning.
It does not represent the real process of bending an already formed pipe.


=========
Real manufacturing must be represented by manufacturing history

formed pipe state
    ?
additional forming operation
    ?
updated manufacturing state
===
The real simulator keeps the four-zone model:
IncomingStock
    ?
PositionedStraight
    ?
ActiveZone
    ?
FrozenGeometry

Threfore:

ArcLength / NodeIndex / ExplicitFrame
    = planned preview placement

FEED / ROTATE / BEND / HELIX / MANUAL
    = manufacturing process history

===
---

## 3. Add comment in `ManufacturingPlanPreviewModel.h`

Near class header:

```cpp
// IMPORTANT:
// This model is allowed to use CAD-like placement tools.
// It may splice, preview, insert, or transform planned curves.
//
// It must not be treated as proof that the machine can
// physically manufacture the same shape.
//```

---

# Phase 9D complete when

```text
1. Comments added.
2. Architecture.md updated.
3. Build succeeds.
4. No behavior change.


================================================================
Phase 9E — Prepare additional forming pass model
for the real case:

already formed pipe
    +
additional bend / helix / manual operation



===================================================================
3D forming system
1.Three-roller forming (section rolling / pyramid rolling

Three rollers arranged in a triangle (two bottom, one top — or vice versa)



2.Two-roller continuous bending
How it works:

Pipe is fed continuously between rollers
One roller changes position dynamically
The pipe exits already curved

What it does:

Produces continuous arcs or spirals
Radius can be adjusted during motion ? variable curvature




3. Rotary draw bending (precision bending)

This is one of the most important systems for 3D pipe forming.

How it works:

Pipe is clamped to a rotating die
It’s drawn around a fixed radius
Often uses a mandrel inside the pipe to prevent collapse

4.Roll bending with multiple axes (3D roll bending)

How it works:

Similar to 3-roller system but:
Rollers can move in multiple directions
Pipe can be rotated during forming
What it does:
Creates true 3D curves (not just in one plane)

5.Stretch bending
How it works:
Pipe is clamped at both ends
Pulled (stretched) while being wrapped over a form

6.Press bending
A die presses the pipe into a shape in one motion


 =================================================================
 ======================================================================

 Practical formula set for numeric curvature calculation, process control, and
 simulation of 3-roller pipe bending.
1. Basic curvature
where
?=curvature
R=bend radius
Units
R[mm],?[mm?1]
2. Curvature from three measured points
For simulation or machine feedback, if you know three pipe centerline points:

P1?(x1?,y1?),P2?(x2?,y2?),P3?(x3?,y3?)
then
a=?P2??P1??
a=?P2??P1??
c=?P3??P1??
Traingle area:
A=21??(x2??x1?)(y3??y1?)?(y2??y1?)(x3??x1?)?
Curvature:
?=4A?/abc
Radius:
R=abc/4A?

3.Cfrom centerline function
if centerline is :
y=y(x)
then for small slopes
??y''

Roller displacement to curvature approximation
let
?=vertical displacement of center roller
L=distance between support rollers
For small deflection:
R?L*L/8?

Therefore:
??8??/L*L
This is one of the simplest control formulas.
More accurate circular-arc geometry
If the roller system creates sagitta ?
R=L*L/8*?+?/2?
therefore:
?=1/L*L/8*?+?/2
For small ?, the ?/2 term is small
Feedback control equation
For CNC bending:
e??=?target???measured?
Roller displacement update:
?new?=?old?+Kp?e??+Ki??e??dt+K*d?de?/??dt
?new?=?old?+Kp?(?target???measured?)
Discrete simulation update
Divide pipe into small segments:
si?=i?s
Curvature field:
?i?=?(si?)
Angle update:
?i+1?=?i?+?i??s
position update:
xi+1?=xi?+?scos?i?
ti+1?=ti?+?s?i?


For a helix, curvature alone is not enough. A circle is fully described by curvature, but a helix requires:

Curvature (?)
Torsion (?)

Curvature tells how strongly the centerline bends.

Torsion tells how strongly the curve twists out of its bending plane.
Helix parameterization

A circular helix can be written as:
x=rcost
y=rsint
z=bt

Where:

r = helix radius
b = vertical rise parameter


Pitch

Usually we use pitch P:

P=2?b

Thus:

b=P/2?

Arc length

One revolution:
Lrev?=sqrt((2?r)^2+P^2)
?
Very important for CNC feed calculation.
Helix curvature

The exact curvature is:

Using pitch:
?=r?/r^2+(P/?2?)^2
Radius of curvature
Rc?=1/??
Thus:
Rc?=r^2+b^2?/r
Notice:

Curvature radius ? helix radius.

This is a common mistake.

Helix torsion

The exact torsion is:
?=b/r^2+b^2
Using pitch:
?=P/(2?)?/r^2+(P/?2?)^2
Relationship between curvature and torsion

For a circular helix:
??/?=b?/r
or
?/??=P/?2?r

Helix angle
?=tan^?1*(P/2?r?)

Then:

?=cos^2??/r

?=sin?cos??/r
Useful for machine control.

Curvature vector
For simulation:
?=?n
where
n
is the principal normal.

Magnitude:
???=?
Frenet-Serret equations

These are the fundamental equations for helix simulation.

dT?/ds=?N
dN/ds=??T+?B

dB/?ds=??N

Where:

T = tangent
N = normal
B = binormal

Discrete simulation

For numerical integration:

?i+1?=?i?+?i??s
Twist:
?i+1?=?i?+?i??s
xi+1?=xi?+?sTx?
yi+1?=yi?+?sTy?
zi+1?=zi?+?sTz?

This is how many tube-forming simulations are implemented.

Roller-bending machine interpretation

For a 3-roll machine producing a helix:

You control two independent quantities:

Curvature

Created by roller penetration:
??8??/L^2
where

L = roller spacing
? = roller displacement
Torsion

Created by controlled pipe rotation:
??d??/ds
where:

? = pipe rotation angle
s = feed length

This is one of the most important formulas for 3D roll bending.

Machine control equations

Feed speed:
v=ds/dt?

Rotation speed:
?=d??/dt
Then:?=??/v
This is the practical CNC formula.

Inverse problem (most useful)
Suppose you want:

helix radius r
pitch P

Compute:
b=P?/2?
Then:
?=r/r^2+b^2?
?=b/r^2+b^2?

Then machine commands become:
?=?L^2?/8
?=?v
This is essentially the mathematical core of generating a helical pipe on a CNC 3-roll bending machine:

Desired helix geometry ? (?,?) ? roller position ? and pipe rotation speed ?.
==============================================================================================
Two rollers
“two rollers” system as a continuous-feed bending process: one roller/guide drives or supports the pipe, 
another adjustable roller creates curvature while the pipe moves. I’ll build the 
math from geometry ? curvature ? force/moment ? control.

Mathematically, a two-roller continuous pipe bending system can be modeled as a moving beam/pipe where 
one adjustable roller imposes a lateral displacement or contact force while the pipe
is continuously fed.

Basic variables

s=pipe feed length
v=ds/dt?=feed speed

?=adjustable roller displacement
L=effective distance between contact/support points
R=bend radius
?=1?/R
Geometry approximation

If the pipe is forced into an arc between two effective contact points, the simplest sagitta relation is:
R=L^2?/8?+2??
So curvature is:
?=?1?/L^2?/8?+?/2

For small displacement:
??8?/?L^2
This is the first useful control equation.

Continuous feed equation

Because the pipe moves continuously:
s=vt
So curvature becomes a function of feed length:
?(s)
or time:
?(t)
If roller displacement changes during feeding:
?=?(t)
then:
?(t)?8?(t)/?L^2
and since s=vt:
?(s)?8?(s)?/L^2


Pipe centerline calculation

For 2D bending, the pipe shape is computed from curvature.

Angle update:
d?/ds?=?(s)
Position update:
dx/ds?=cos?
dy/ds?=sin?

So numerically:
?i+1?=?i?+?i??s
xi+1?=xi?+?scos?i
yi+1?=yi?+?ssin?i?
Control law

If you measure actual curvature:
e??=?target???measured?
then roller correction:
?new?=?old?+Kp?e??
More complete PID:
?new?=?old?+Kp?e??+Ki??e??dt+Kd?de???/dt

For variable-radius bending

If desired radius changes along the pipe:
R=R(s)
then:
?(s)=1/R(s)
Required roller displacement:
?(s)??(s)L^2?/8
or:
?(s)?L^2/8R(s)

================================================================

Strech bending
compact math foundation for stretch bending a pipe into a helix, like a coil/heating element.
target helix geometry
Let:
r=helix radius
P=pitch per revolution
b=P?/2?


Parametric centerline:
x(u)=rcosu
y(u)=rsinu
z(u)=bu
where u is the angular parameter.

Arc length per radian:
ds/du?=sqrt(r^2+b^2)
Arc length per revolution:
Lrev?=2?sqrt(r^2+b^2)
Number of turns:
N=?Lpipe??/Lrev

Helix curvature and torsion

For a circular helix:
?=r/r^2+b^2?
?=b/r^2+b^2
Radius of curvature:
Rc?=1/??=r^2+b^2/?r
Important:
Rc !?=r 
The pipe does not bend with radius equal to helix radius unless pitch is zero.

Helix angle

Helix angle:

?=tan^?1*(P/2?r?)
?
Also:
?=cos^2??/r
?=sin?cos??/r

Stretch bending idea

In stretch bending, the pipe is pulled with axial tension while bent around a form.

Total axial strain at a fiberHelix simulation update

Given target:

integrate centerline using Frenet frame:

dT/ds?=?N
dN?/ds=??T+?B

dB/ds?=??N

Discrete form
Ti+1?=Ti?+?s?Ni?
Ni+1?=Ni?+?s(??Ti?+?Bi?)
Bi+1?=Bi???s?Ni?
ri+1?=ri?+?sTi?
Normalize frame vectors each step

Machine-control meaning

For stretch bending into a helix, control variables are usually:
T=axial tension
?=bending curvature
?=twist / spatial rotation rate

Feed speed:

v=ds?/dt
Rotation speed:
?=d?/dt

Torsion control approximation:

?=??/v

So:

?=?v

================================================================
input D, t, E, sigma_y, eps_allow

input helix radius r, pitch P, pipe length Lpipe
input feed speed v

d = D - 2*t
A = pi/4 * (D^2 - d^2)
I = pi/64 * (D^4 - d^4)

b = P / (2*pi)

kappa = r / (r^2 + b^2)
tau   = b / (r^2 + b^2)

Rc = 1 / kappa

eps_b = kappa * D/2

eps0_min = eps_b
eps0_max = eps_allow - eps_b

if eps0_min > eps0_max:
    geometry is not feasible

choose eps0 between eps0_min and eps0_max

T = E * A * eps0

eps_outer = eps0 + eps_b
eps_inner = eps0 - eps_b

omega = tau * v

simulate Frenet frame:
    Tvec, Nvec, Bvec
    for each ds:
        Tvec += ds*kappa*Nvec
        Nvec += ds*(-kappa*Tvec + tau*Bvec)
        Bvec += ds*(-tau*Nvec)
        normalize
        position += ds*Tvec
	?
Core final result:

(r,P)?(?,?)?(?b,?0,T)?(?,v)?helical pipe

That is the mathematical base for a stretch-bending helix simulator.

=================================================
===================================================


	ManufacturingPass
    should know the forming process type

RotaryDrawPass
TwoRollerContinuousPass
StretchBendingPass
HelixFormingPass

Core/Forming/
    machine/process description

Core/Control/
    CNC control formulas
    feed speed
    rotation speed
    roller displacement
    curvature target

Core/Geometry/
    generated centerline
    Frenet frame
    helix integration

Core/Physical/
    later:
    springback
    material parameters
    strain

    curvature:
kappa = 1 / R

two-roller small displacement:
kappa ? 8 * delta / L^2

helix:
b = P / (2*pi)

kappa = r / (r^2 + b^2)
tau   = b / (r^2 + b^2)

rotation speed:
omega = tau * v

=========================================================
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
Phase 1C
Move temporary manufacturing-history test into clean debug helper
Keep AppController constructor clean.
Keep ManufacturingHistory compile test available.
No playback/rendering behavior change.

==============================================
Phase 2A — Manufacturing History becomes part of the simulator
Goal:
Right now:
ManufacturingHistory
        ?
temporary compile test only

ASCII diagram:
AppController
      ?
      ?
SimulationController
      ?
      ??? ManufacturingPipeSimulator
      ??? ManufacturingPlanPreview
      ??? ManufacturingHistory

      Only architecture:

Add a ManufacturingHistory member to SimulationController.

User program
      ?
      ?
ManufacturingHistory
      ?
      ?
ManufacturingPipeSimulator
      ?
      ?
Renderer

=======================================================
Phase 2B.
Goal:
Stop using dummy ManufacturingPass objects.
Store the real rotary + helix passes in simulator-owned
ManufacturingHistory.
ASCII
AppController
   ?
   ??? build rotaryPass
   ??? build helixPass
   ?
   ?
ManufacturingPlanPreview
   ?
   ??? visual planned shape preview

SimulationController
   ?
   ?
ManufacturingHistory
   ??? primaryPasses[0]    = real rotaryPass
   ??? additionalPasses[0] = real helixPass

   Pipeflow meaning

   raw heater tube
      ?
      ?
primary rotary draw pass
      ?
      ?
already formed pipe
      ?
      ?
additional helix forming pass
      ?
      ?
future updated manufactured pipe

meanig now:
ManufacturingPlan
   ?
   ??? pass[0] real rotary pass
   ??? pass[1] real helix pass
          ?
          ?
ManufacturingHistory
   ??? primaryPasses[0]
   ??? additionalPasses[0]

   Pipeflow:

   raw heater tube
      ?
      ?
rotary draw pass
      ?
      ?
formed pipe state
      ?
      ?
additional helix pass
      ?
      ?
future continued manufacturing

===================================================
Phase 2C
Add ManufacturingHistory debug summary
Goal:
Print what kind of passes are stored,
not only how many.

Expected future console
[MFG HISTORY]
primary passes: 1
  [0] RotaryDraw

additional passes: 1
  [0] Additional helix forming pass
      process: Helix
Phase 2C as a pure debug/visibility phase: add a reusable summary 
printer for ManufacturingHistory, call it after real passes are 
stored, and leave playback/rendering untouched.
Goal:
Print ManufacturingHistory content clearly,
not only counts.
SimulationController
      ?
      ?
ManufacturingHistory
      ?
      ??? primaryPasses
      ?       ??? [0] Rotary / primary forming pass
      ?
      ??? additionalPasses
              ??? [0] Additional helix forming pass

raw pipe
   ?
   ?
primaryPasses[0]
   ?
   ?
already formed pipe
   ?
   ?
additionalPasses[0]
   ?
   ?
future continued pipe state


===============================================
Anonymous space
This is called an anonymous namespace.

It means:

"Everything inside this namespace is private to this .cpp file."

Without namespace
Suppose you simply write

void debugTestManufacturingHistory()
{
    ...
}

This function has external linkage.

The linker sees

debugTestManufacturingHistory()

as a global symbol.

Imagine later you create another file

SimulationController.cpp

and accidentally write

void debugTestManufacturingHistory()
{
    ...
}

Now the linker finds

debugTestManufacturingHistory()

twice.

Result:

multiple definition

Link error.

Anonymous namespace

Now instead

namespace
{
    void debugTestManufacturingHistory()
    {
    }
}

means

This function exists ONLY inside AppController.cpp

No other cpp file can see it.

It cannot collide.
==
Think of it like "private"
Classes have
private:
Anonymous namespaces are almost
private:
for an entire cpp file.
==
Small example
Suppose you have
AppController.cpp
SimulationController.cpp
PipeRenderer.cpp
Each can contain
namespace
{
    int counter = 0;
}
Each one has its own
counter
They are completely independent.

Why not put it in the class?
We could have
class AppController
{
private:
    void debugTestManufacturingHistory();
};

That would work.

But ask:
Does this helper belong to AppController?
Not really.
It is only temporary debug code.
It doesn't use

this
It doesn't modify AppController.
So making it a class member would unnecessarily enlarge 
the public design of the class.
================================


AppController.cpp
??????????????????????????????????????
? namespace                          ?
? {                                  ?
?   manufacturingProcessTypeToString ?
? }                                  ?
??????????????????????????????????????
        ?
        ?
visible only inside AppController.cpp

Other .cpp files cannot call it.
Other .cpp files cannot conflict with it.
Without namespace:
AppController.cpp                  OtherFile.cpp
manufacturingProcessTypeToString   manufacturingProcessTypeToString
        ?                                  ?
        ????????????? conflict ?????????????
AppController.cpp                  OtherFile.cpp
local helper                       local helper
        ?                                  ?
        ????? no conflict, separate ????????

About public:

If we move this to a reusable helper file in Phase 2D, then it 
becomes intentionally shared:

ManufacturingHistoryDebug.h
??????????????????????????????????????
? public declaration                 ?
?                                    ?
? void debugPrintManufacturingHistory?
??????????????????????????????????????
        ?
        ?
AppController.cpp can call it
SimulationController.cpp can call it
future test files can call it
================================
Phase 2D
Move ManufacturingHistory debug summary into its own helper file
Goal;
AppController should not own history-printing logic.
Create reusable debug utility.
AppController
    ?
    ?
debugPrintManufacturingHistory(history)
    ?
    ?
Core/Forming/ManufacturingHistoryDebug

Move debug printing out of AppController.cpp

Before:

AppController.cpp
 ??? builds history
 ??? owns debug printer
 ??? prints summary


After:

AppController.cpp
 ??? builds history
 ??? calls helper
          ?
          ?
Core/Forming/ManufacturingHistoryDebug
 ??? prints summary
=======================================================

Phase 2E
Add ManufacturingHistory reset/build helper
Goal:
Move this logic out of AppController:

ManufacturingPlan
   ?
ManufacturingHistory
  Pipeflow;

  plan.passes[0] ? primaryPasses
plan.passes[1] ? additionalPasses
=======================================================
Phase 2E another cleanup/refactor phase: move “plan ? history” 
construction into a reusable helper so AppController only requests 
the conversion.

Goal:
Move ManufacturingPlan ? ManufacturingHistory logic out of AppController.
Before:

AppController.cpp
   ??? creates ManufacturingPlan
   ??? fills ManufacturingHistory
   ??? prints debug


After:

AppController.cpp
   ??? creates ManufacturingPlan
   ??? calls buildManufacturingHistoryFromPlan()
              ?
              ?
Core/Forming/ManufacturingHistoryBuilder

pipeflow:
ManufacturingPlan
   ??? passes[0] ???????????????? history.primaryPasses[0]
   ?                                primary forming
   ?
   ??? passes[1] ???????????????? history.additionalPasses[0].pass
                                    additional forming

=====================================================


Phase 2F
Name additional forming pass from process type

Goal:
Stop hardcoding:
"Additional helix forming pass"

Generate name from actual pass.processType.

pipeflow unchanged:
plan.passes[0] ? primaryPasses
plan.passes[1] ? additionalPasses[0]

Stop hardcoding:
"Additional helix forming pass"

Use process type:
Helix ? "Additional helix forming pass"
RotaryDraw ? "Additional rotary draw forming pass"
TwoRoller ? "Additional two-roller forming pass"

ASCII:

ManufacturingPass
      ?
      ?
processType
      ?
      ?
additional pass name
      ?
      ?
ManufacturingHistory
pipeflow unchanged
ManufacturingPlan
   ??? passes[0] ??? primaryPasses[0]
   ??? passes[1] ??? additionalPasses[0]
                         ?
                         ??? generated name

======================================================
Phase 2G
Support all extra passes, not only pass[1]
Goal:
Current:
plan.passes[0] ? primary
plan.passes[1] ? one additional

Next:
plan.passes[0] ? primary
plan.passes[1..N] ? additional passes


ASCII:
ManufacturingPlan
   ??? passes[0] ?????????? primaryPasses[0]
   ??? passes[1] ?????????? additionalPasses[0]
   ??? passes[2] ?????????? additionalPasses[1]
   ??? passes[N] ?????????? additionalPasses[N-1]
pipeflow
raw pipe
   ?
primary pass
   ?
additional pass 1
   ?
additional pass 2
   ?
additional pass N
   ?
future manufactured pipe

Phase 2G a small generalization: replace the fixed passes[1] logic 
with a loop over all passes after the primary pass.

Goal:
Support all extra passes, not only plan.passes[1].
ManufacturingPlan
   ?
   ??? passes[0] ?????????? history.primaryPasses[0]
   ?
   ??? passes[1] ?????????? history.additionalPasses[0]
   ??? passes[2] ?????????? history.additionalPasses[1]
   ??? passes[3] ?????????? history.additionalPasses[2]
   ?
   ??? passes[N] ?????????? history.additionalPasses[N - 1]
==================================================================
   Phase 2H
   Make additional pass names readable when there are many.
   plan.passes[1] ?? additionalPasses[0] ?? "Additional pass 1: helix forming pass"
plan.passes[2] ?? additionalPasses[1] ?? "Additional pass 2: helix forming pass"
plan.passes[3] ?? additionalPasses[2] ?? "Additional pass 3: stretch-bending forming pass"
pipeflow unchanged
raw pipe
   ?
primary pass
   ?
additional pass 1
   ?
additional pass 2
   ?
future final pipe

===================================================
Phase 2I
Add entry-frame debug print for additional passes

Goal:
Show where each additional forming pass starts.

additionalPasses[0]
   ??? name: Additional pass 1: helix forming pass
   ??? process: Helix
   ??? entryFrame
        ??? P position
        ??? T tangent
        ??? N normal
        ??? B binormal

 formed pipe
    ?
entryFrame
    ?
additional forming pass

only improve debug output: print entryFrame vectors for each
additional pass, without changing history construction or
simulation behavior.
====================================Phase 2J


Prepare real entryFrame calculation placeholder

Phase 2J:
PassPlacement / ManufacturingPlanPreviewModel
        = CAD / planned preview placement

AdditionalFormingPass.entryFrame
        = real manufacturing continuation start frame

Do not let ManufacturingHistory depend directly on PreviewModel.
But we can reuse the same placement-resolution idea.

Current preview flow
ManufacturingPlan
      ?
      ?
ManufacturingPlanPreviewModel
      ?
      ??? PassPlacement::InsertAtArcLength
      ??? PassPlacement::InsertAtNodeIndex
      ??? PassPlacement::ExplicitStartFrame
              ?
              ?
        resolved Frame
But better architecture for Phase 2J:

Core/Forming/PassPlacementResolver
        ?
        ??? resolve arc length
        ??? resolve node index
        ??? resolve explicit frame
        ??? return Frame

Then both systems can use it:
ManufacturingPlanPreviewModel
        ?
        ?
PassPlacementResolver
        ?
        ?
ManufacturingHistoryBuilder

Pipeflow
primary pass output curve
        ?
        ?
sample / query curve
        ?
        ?
resolved entryFrame
        ?
        ?
additional forming pass starts here

So Phase 2J should not yet calculate perfect real
manufacturing frames.It should prepare reusable frame-resolution logic.
Before:
ManufacturingPlanPreviewModel
    ??? private resolvePlacementStartFrame()

After:
ManufacturingPlanPreviewModel
    ??? PassPlacementResolver
              ?
              ? future reuse
ManufacturingHistoryBuilder

========================================
Current 

                    ManufacturingPlan
                            ?
                            ?
              ManufacturingPlanPreviewModel
                            ?
                    resolvePlacement()
                            ?
                            ?
                          Frame


Only Preview can do it.

Target architecture
After Phase 2J:



                    ManufacturingPlan
                            ?
                            ?
                 PassPlacementResolver
                            ?
                placement ? Frame
                            ?
      ?????????????????????????????????????????????
      ?                     ?                     ?
 PreviewModel       ManufacturingHistory     Future Playback


 Notice something important:
 The resolver does not know anything about Preview.
It only knows geometry.
That makes it reusable.

pipeflow
CAD Curve
     ?
     ?
PassPlacement
     ?
     ?
PreviewModel
     ?
     ?
Frame



Future pipeflow:
CAD Curve
     ?
     ?
PassPlacement
     ?
     ?
PassPlacementResolver
     ?
     ?
Frame
     ?
     ?????????? Preview
     ?
     ?????????? ManufacturingHistory
     ?
     ?????????? Playback
     ?
     ?????????? Collision


   finally after improvements.
   1111111111111111111111111111111111111111111
   111111111111111111111111111111111111111111111111111111111

What we achieved:
PassPlacement
      ?
PassPlacementResolver
      ?
Frame + arcLength

Pipeflow:

base curve
   ?
placement rule
   ?
resolved frame
   ?
preview now
   ?
future additional-pass entryFrame
=============================================================================
Phase 2K
Use PassPlacementResolver for AdditionalFormingPass.entryFrame

Goal:
AdditionalFormingPass.entryFrame
should come from pass.placement,
not from hardcoded identity frame.
Before:
additional pass
    ??? entryFrame = identity
        P=(0,0,0)
        T=(1,0,0)
        N=(0,1,0)
        B=(0,0,1)
After:
base curve before this pass
        ?
pass.placement
        ?
PassPlacementResolver
        ?
AdditionalFormingPass.entryFrame
Pipeflow:
primary pass output curve
        ?
base curve for additional pass
        ?
placement rule
        ?
resolved entryFrame
        ?
additional forming pass


====
So now both systems agree:

PassPlacementResolver
      ??? PreviewModel frame
      ??? ManufacturingHistory entryFrame
primary rotary pass
      ?
base curve at NodeIndex
      ?
resolved entryFrame
      ?
additional helix pass

===========================================================
===========================================================

Phase 2L
Print placement resolution info in ManufacturingHistory debug

Goal:
Debug should show not only entryFrame,
but also how this entryFrame was resolved.
ASCII:
AdditionalFormingPass
   ??? pass
   ??? entryFrame
   ??? placement resolution info
        ??? mode
        ??? nodeIndex / requested arcLength
        ??? resolvedArcLength

Pipeflow:
PassPlacement
      ?
PassPlacementResolver
      ?
Frame + resolvedArcLength
      ?
AdditionalFormingPass debug info

===========================================================
===========================================================

Phase 2M
Implement AppendToPrevious in PassPlacementResolver

Pipeflow:
previous pass curve
      ?
end frame
      ?
AppendToPrevious
      ?
next pass entryFrame

PassPlacementMode::AppendToPrevious
should resolve to the end frame of the already-built base curve.

ASCII:
previous pass / base curve
????????????????????????????? end frame
                                ?
                                ?
                         AppendToPrevious
                                ?
                                ?
                         next pass starts here

Pipeflow:
raw pipe
   ?
primary pass
   ?
base curve end frame
   ?
additional pass

==============================================================
Phase 2N
Add debug test for AppendToPrevious placement

Goal:
Temporarily switch/add one pass using AppendToPrevious
and confirm resolver returns end frame of previous curve.

pipeflow:

primary pass
      ?
end frame
      ?
AppendToPrevious
      ?
additional pass entryFrame

base curve
????????????????????????????? last node
                                ?
                                ?
                         AppendToPrevious
                                ?
                                ?
                         resolved entryFrame

==================================================================
Phase 2P
Clean ManufacturingHistory debug output label names
Goal:
Make debug text clearer:
requestedArcLength only meaningful for ArcLength mode.
For NodeIndex, print nodeIndex as main value.


current output:
placementMode=InsertAtNodeIndex resolved=1
nodeIndex=404 requestedArcLength=0 resolvedArcLength=201.989

better output:
placementMode=InsertAtNodeIndex resolved=1
requestedNodeIndex=404
resolvedArcLength=201.989

=======================================================
Phase 2Q
Add resolved placement info to HUD/debug overlay later-ready data

Goal:
Prepare history placement information so it can later be shown in HUD,
not only console.


Prepare placement-resolution info for future HUD/debug overlay.
Do not draw it yet.

ASCII:
ManufacturingHistory
      ?
      ?
PlacementDebugSummary
      ?
      ??? modeName
      ??? resolved
      ??? requestedValueText
      ??? resolvedArcLength
      ??? entryFrame
              ?
              ? later
            HUDPanel
AdditionalFormingPass
      ?
summary data
      ?
console now
      ?
HUD later

==============================================
Phase 2R: use history summary data in debug printer

Goal:
Make console debug and future HUD use the same summary data.

Pipeflow:

ManufacturingHistory
      ?
ManufacturingHistorySummaryBuilder
      ?
console debug now
      ?
HUD later
====================================================================


Phase 2T
Move shared process-name conversion into one helper

Goal:

Avoid duplicate TubeFormingProcessType ? string functions
in Debug, SummaryBuilder, Builder, etc.

One shared helper for:
TubeFormingProcessType ? readable label

ASCII
TubeFormingProcessType
        ?
        ?
FormingProcessLabels
        ?
        ??? ManufacturingHistoryDebug
        ??? ManufacturingHistorySummaryBuilder
        ??? ManufacturingHistoryBuilder

========================================================
Phase 2U
Add shared placement-mode label helper
Goal:
Avoid duplicate PassPlacementMode ? string functions.
Use one helper for console now and HUD later.

ASCCII:
PassPlacementMode
        ?
        ?
PassPlacementLabels
        ?
        ??? ManufacturingHistoryDebug
        ??? ManufacturingHistorySummaryBuilder
        ??? future HUD/debug overlay

=========================================================
Phase 2V
Use shared process labels in additional pass naming

Goal:
Remove remaining hardcoded names like:
"helix forming pass"
"rotary draw forming pass"

Use shared forming process labels as base.

We want:
TubeFormingProcessType
      ?
formingProcessTypeToLabel()
      ?
Additional pass name


ASCII:

ManufacturingPass.processType
        ?
        ?
formingProcessTypeToLabel()
        ?
        ?
"Additional pass 1: HelixForming"

===================================================
Phase 2W
Add primary pass summaries
Goal:
SummaryBuilder should describe primary passes too,
not only additional passes.
Pipeflow:

ManufacturingHistory
   ??? primaryPasses
   ?       ?
   ?   PrimaryPassSummary
   ?
   ??? additionalPasses
           ?
       AdditionalPassPlacementSummary

       ManufacturingHistorySummaryBuilder should summarize:
- primary passes
- additional passes

ASCII:
ManufacturingHistory
   ?
   ??? primaryPasses
   ?       ?
   ?       ?
   ?   PrimaryPassSummary
   ?
   ??? additionalPasses
           ?
           ?
       AdditionalPassPlacementSummary

       =====================================================
       Phase 2X
Add full ManufacturingHistorySummary object
Goal:
Instead of returning primary and additional summaries separately,
create one combined summary object for future HUD.

Pipeflow:
ManufacturingHistory
      ?
ManufacturingHistorySummary
      ??? primaryPasses
      ??? additionalPasses
      ?
console now / HUD later



==============================
Phase 2Y
Add summary counts helper

Goal:
Make HUD/debug able to quickly show:
primary count
additional count
total pass count

ManufacturingHistorySummary
   ??? primaryPasses
   ??? additionalPasses
   ?
   ??? primaryPassCount
   ??? additionalPassCount
   ??? totalPassCount

   ===========================================
   Phase 2Z
Guard manufacturing history debug output
Goal:
Allow enabling/disabling this console block cleanly.

Goal;
Keep ManufacturingHistory debug printing available,
but make it easy to turn on/off.

ASCII:
AppController
   ?
   ??? build ManufacturingHistory
   ?
   ??? if debug flag enabled
           ?
        print history summary

===============================================================
Phase 3A
Review ManufacturingPipeSimulator 4-zone render data

Goal:
Return from history/preview cleanup
to the real manufacturing pipeflow core.

Pipefloat target:
incoming stock
   ?
positioned straight
   ?
active bend zone
   ?
current bend trace
   ?
frozen geometry

=========================================================
Phase 3B review.
main rule:
ManufacturingPipeSimulator owns process state.
Renderer owns drawing only.
Correct ownership:
ManufacturingPipeSimulator
      ?
      ?
ManufacturingRenderData
      ??? incomingStockNodes
      ??? positionedStraightNodes
      ??? currentBendTraceNodes
      ??? activeZoneNodes
      ??? frozenNodes
legacy path:
ManufacturingRenderData
      ?
flattenManufacturingRenderData()
      ?
renderNodes
      ?
old single-strip drawing
Separated zones preserve manufacturing meaning.
Flattened renderNodes loses zone meaning.

ASCII:
Correct:

incoming | positioned | trace | active | frozen
   ?           ?          ?       ?        ?
   ?           ?          ?       ?        ?
draw separately with zone colors / styles


Legacy:

incoming + positioned + trace + active + frozen
              ?
              ?
       one flat node list
              ?
              ?
       zone meaning partly lost

       Phase 3C
Add comments marking flattenManufacturingRenderData as legacy compatibility


=====================================================
Phase 3F
Goal:
Avoid repeating manufacturing zone order manually
in LINE and MESH paths.

Current duplication:
uploadPipeGeometry() LINE order
paintGL() MESH order

Target:
one clear helper/comment defines order:

incoming
positioned
frozen
trace
active

ASCII:
ManufacturingRenderData
      ?
      ?
zone order helper
      ?
      ??? LINE upload
      ??? MESH draw
============================================================
Phase 3G.
Create one helper for LINE zone strip order.
ASCII:
ManufacturingRenderData
      ?
      ?
buildManufacturingLineStrips()
      ?
      ?
pipeRenderer.uploadLineStrips()
==================================================================
=================================================================
===================================================================
====================================================================
Option 1
Anonymous namespace
namespace
{
    helper(...)
}
belongs to GLView.cpp
not to GLView.
ASCII:

GLView.cpp

helper()

GLView

paintGL()
uploadPipeGeometry()

No ownership relation.

Option 2

Private member

class GLView
{
private:

    drawManufacturingMeshZones();
};

ASCII:
GLView

+----------------------------+
| uploadPipeGeometry()       |
| paintGL()                  |
| drawTubeZone()             |
| drawManufacturingMeshZones()|
+----------------------------+

Now it is clearly

part of GLView.

Which one is better?

Ask one question:

Does this function need GLView?

If NO
namespace helper
is usually better.

Example

double degToRad(double);

No GLView needed.

Perfect namespace helper.

If YES

private member
is usually better.

Example
drawManufacturingMeshZones()
Why?

Because inside it we call
drawTubeZone(...)
and
drawTubeZone()
already belongs to
GLView.
So conceptually
drawManufacturingMeshZones()

belongs to GLView too.
Compare our two helpers

buildManufacturingLineStrips()

Only builds a vector.

No OpenGL.

No camera.

No renderer.

Just data conversion.

That naturally fits a namespace helper.

ASCII:
ManufacturingRenderData
        ?
        ?
buildManufacturingLineStrips()
        ?
        ?
std::vector<std::vector<float>>
Phase 3H
drawManufacturingMeshZones()
Calls
drawTubeZone(...)
which calls
tubeMesh.generate(...)
pipeRenderer.uploadMesh(...)
pipeRenderer.draw()
These are all GLView responsibilities.
ASCII:
GLView
 ?
 ??? drawTubeZone()
 ??? uploadPipeGeometry()
 ??? paintGL()
 ??? drawManufacturingMeshZones()
 It makes sense that all rendering actions live together 
 inside GLView.

 A rule I personally follow
 Pure calculation?
        ?
namespace helper

Uses object state or other member functions?
        ?
private member function

This keeps the code easier to understand and maintain.


Another possibility is a forward declaration.
void world();

void hello()
{
    world();
}

void world()
{
}

This tells the compiler

Trust me.

There will be a function called world() later.

==================
The anonymous namespace means

"This function belongs only to this .cpp file."

It is not a member of any class.

Think of it like this:

GLView.cpp

+--------------------------------------------+
| namespace                                  |
| {                                          |
|     helper A                               |
|     helper B                               |
| }                                          |
|                                            |
| GLView::paintGL()                          |
| GLView::uploadPipeGeometry()               |
+--------------------------------------------+

These helpers are simply "local workers".

They don't belong to GLView.

Why did it fail?
Because the helper tried to call
nodesToFloatLine(...)

the file look like here
namespace
{
    helper()
    {
        nodesToFloatLine();   // compiler: What is this??
    }
}

// 300 lines later...

nodesToFloatLine(...)

The compiler reads a file from top to bottom.

It does not know what comes later.


==============================================

Phase 3H.
goal
Extract MESH manufacturing zone draw order into one helper.

ASCII:
ManufacturingRenderData
      ?
      ?
drawManufacturingMeshZones()
      ?
      ??? incoming
      ??? positioned
      ??? frozen
      ??? trace
      ??? active
      ===================================================

Phase 3N review.

Main rule:
ManufacturingPipeSimulator should NOT own colors.
Why:
Simulator = process state
Renderer/GLView = visual style
Correct ownership:

ManufacturingPipeSimulator
   ??? zone nodes only

GLView / Renderer
   ??? zone colors / line width / mesh radius

   Target future colors:

   incoming stock       = raw material color
positioned straight  = fed straight color
frozen geometry      = finished pipe color
current bend trace   = forming trace color
active zone          = highlight color
ASCII:
data:
incoming ? positioned ? frozen ? trace ? active

style:
color 1  ? color 2    ? color 3 ? color 4 ? color 5

coclusion:
? colors belong to rendering layer
? ManufacturingRenderData should stay geometry-only
? next step should add GLView-level zone color constants/placeholders

goal:
Add named colors for manufacturing zones.


=============================================
Phase 3P
Add helper for drawing colored manufacturing mesh zone

Goal:
Reduce repeated pattern:

shader->setVec3(...)
drawTubeZone(...)

==================

Phase 3R
Add CAD and plan preview color constants
ASCII idea
Before

paintGL()
   ?
   ??? (0.2,0.9,0.3)
   ??? (0.2,0.9,0.3)
   ??? (0.2,0.9,0.3)
   ??? (0.2,0.9,0.3)


After

DEFAULT_PIPE_COLOR
        ?
        ???????????? paintGL()
        ???????????? drawMachineReference()
        ???????????? debug rendering
        ???????????? future render code

        Goal:
        Separate colors for:
- CADPreview
- PlannedShapePreview
- ManufacturingPlayback

Phase 3R.

This phase follows the same architecture we've been building since 3J:

Every rendering mode should own its own visual constants.

Even if today they all use the same green color, tomorrow they may differ.

Why are we doing this?

Right now we have something like:

DEFAULT_PIPE_COLOR
        ?
        ??? CAD Preview
        ??? Plan Preview
        ??? Manufacturing
But these are three different visualization modes.

Later we might want:

CAD Preview        ? Gray
Plan Preview       ? Blue
Manufacturing      ? Green + zone colors
Architecture
                    Rendering Modes
                           ?
      ???????????????????????????????????????????
      ?                    ?                    ?
      ?                    ?                    ?
 CAD Preview        Plan Preview        Manufacturing
      ?                    ?                    ?
      ?                    ?                    ?
CAD_PIPE_COLOR   PLAN_PREVIEW_PIPE_COLOR   DEFAULT_PIPE_COLOR
Notice something important:
These are NOT three copies.

They are three different meanings.

=======================================================================
Think of it as the conductor of an orchestra.? calls every frame
 ?
paintGL()
 ?
 ??? setup camera
 ??? setup shader
 ??? choose simulation mode
 ??? draw CAD
 ??? draw Plan Preview
 ??? draw Manufacturing

 So if there is a place where shader->setVec3(...) should appear, 
 paintGL() is one of the first places to check.

 paintGL()

    ?
    ?
DEFAULT_PIPE_COLOR

    ?
    ?
drawManufacturingMeshZones()

        ?
        ?
drawColoredManufacturingTubeZone()

        ?
        ??? set gray
        ??? draw
        ?
        ??? set yellow
        ??? draw
        ?
        ??? set green
        ??? draw
        ?
        ??? ...

        ==================================================

        Zaczynam Phase 3X 
        jako kolejny refactor czytelnoœci: wyci¹gniemy g³ówne rysowanie plan-preview pipe z 
        drawPlannedShapePreview(), a overlay helpers zostan¹ osobno.

 Goal:
drawPlannedShapePreview()
should orchestrate only:

1. draw plan pipe
2. draw insertion marker
3. draw insertion frame


===============================================================
Phase 3Y
Extract CAD preview pipe drawing helper

Goal:

Make drawCadPreview() match the same pattern:

drawCadPreview()
      ?
drawCadPreviewPipe()

Before:

drawManufacturingPlayback()
   ??? get mfg pipe
   ??? get render data
   ??? set renderer mode
   ??? LINE drawing
   ??? MESH drawing


After:

drawManufacturingPlayback()
   ??? get mfg pipe
   ??? get render data
   ??? drawManufacturingPlaybackPipe()
          ??? LINE drawing
          ??? MESH drawing
===============================================================
Phase 4A
Review ManufacturingPipeSimulator bend trace vs active zone
Goal:
Return from GLView cleanup to manufacturing behavior.

Check:
- activeZone = small moving deformation window
- currentBendTrace = full visible arc formed during current bend

[entry] ) ) ) ) ) ------>
        trace arc     positioned straight

            \\\
            active window

=============================================================
Phase 4C
Add shared manufacturing zone append helper

Goal:
Avoid repeating:

for (const auto& node : zone)
    renderNodes.push_back(node);
=========================================================
Phase 4D
Add shared manufacturing zone order comment in simulator

Make simulator flatten order match renderer order intentionally.

=================================================

Phase 4S
add compact manufacturing state snapshot

[MFG SNAPSHOT] incoming=102 positioned=166.64 trace=50 active=10 frozen=0
======================================================
Phase 5J is intentionally a documentation phase, 
not a behavior change.
he goal is to make the ownership and intent of positionedStraight.nodes obvious to

struct ManufacturingPositionedStraight
{
    // =====================================================
    // SOURCE OF TRUTH
    //
    // Remaining straight pipe currently positioned in the
    // machine before entering the active bend.
    // =====================================================
    double length = 0.0;

    // =====================================================
    // GENERATED CACHE
    //
    // Rebuilt from:
    //   - length
    //   - current attachment frame
    //
    // This is render/generated geometry.
    // Do not treat this as simulation state.
    // =====================================================
    std::vector<PipeNode> nodes;
};

cleary separate
Simulation state
        vs
Generated geometry

Several phases ago we discussed ownership.

This is another example.

Think of it like this:

Simulation

length
frame
radius
angle
        ?
        ?
        ?
Geometry Builder
        ?
        ?
nodes
The simulation owns:
length
frame
angle
The geometry builder produces:
nodes
So nodes are derived data, not the authoritative state.
Future benefit

Later, when optimizing performance, you'll immediately know:
Can I clear nodes?
        YES

Can I regenerate nodes?
        YES

Can I reconstruct simulation from nodes?
        NO
That distinction is extremely valuable in large simulation codebases


Now we're gradually building a simulation architecture:
simulation state vs generated geometry,
ownership of data,
rendering vs simulation,
debug infrastructure,
manufacturing history.

Those are the kinds of architectural boundaries
that make a project much easier to extend

later—for example, when you introduce helix forming or 
true multi-pass manufacturing.
=========================================================
Phase 5K
Review currentFrame ownership
Goal:
Clarify whether currentFrame is still needed as real state,
or only legacy compatibility.
Question:
Does ManufacturingPipeSimulator need to own currentFrame
as real state?
currentFrame
    = end of manufactured pipe?
    = legacy PipeAxis3D frame?
    = duplicate of frozenNodes.back()?

    Review target:
    currentFrame should not become a hidden second source of truth.
    Zaczynam Phase 5K jako review w³asnoœci stanu. Sprawdzimy,
    czy currentFrame jest Ÿród³em prawdy, 
    czy tylko pomocnicz¹/legacy kopi¹ koñca frozenNodes.

    Current currentFrame role:
    currentFrame = legacy/current output frame

    It is updated in places like:
    resetFrames()
freezeActiveZone()
rotatePipeBodyAroundMachineAxis()
syncCurrentFrameFromFrozen()
But the stronger source of truth is usually:
But the stronger source of truth is usually:
state.frozenNodes.back()

ASCII:
frozenNodes
   ??? node 0
   ??? node 1
   ??? last node
          ?
          ?
     output/end frame
     Risk:
currentFrame can become a duplicate source of truth
if it disagrees with frozenNodes.back()

============================================
Phase 5M
Add helper to get frozen end frame
Goal:
Avoid directly accessing frozenNodes.back()
everywhere.

frozen end frame logic becomes explicit
and currentFrame remains a cache/compatibility value.

=========================================

Phase 5N
Use frozen end frame helper in freezeActiveZone

Goal:

After frozenNodes is rebuilt,
sync currentFrame through one helper path.
===============================================
Phase 5P
Review beginBendFromFrame start frame ownership

Goal:
Clarify whether beginBendFromFrame should own its own
start frame,
Prepare future support for:

- normal rotary draw from machineEntryFrame
- additional forming pass from AdditionalFormingPass.entryFrame
 Current:
 beginBendFromFrame(
    machineEntryFrame,
    radius,
    targetAngle,
    bendDirection
);

Future:
startFrame =
    normal manufacturing     ? machineEntryFrame
    additional forming pass  ? additionalPass.entryFrame
    ==========================
    Yes, we can reuse concepts, but not the whole
    ManufacturingPlanPreviewModel

    Reusable:
    PassPlacement
PassPlacementResolver
resolved Frame
arc length / node index / explicit frame logic

Do not reuse directly:
preview node rebuilding
curve splicing
transformed preview overlays
preview-only strips
Why:
ManufacturingPlanPreviewModel = CAD-like planned preview
ManufacturingPipeSimulator    = real process playback

Good future ovnership
bend start frame
    ??? normal rotary playback
    ?       ??? machineEntryFrame
    ?
    ??? additional forming pass
            ??? AdditionalFormingPass.entryFrame

    ASCII:
    PassPlacement
      ?
PassPlacementResolver
      ?
Frame
      ?
beginBendFromFrame(...)
============

Phase 5Q.
Goal:
Centralize bend start frame selection.
For now it still returns machineEntryFrame.
Later it can return AdditionalFormingPass.entryFrame.
=======================================================
Phase 5S
Review active bend start vs positioned straight start

Goal:
Check relationship between:

activeZone.frame
positionedStraight start frame
machineEntryFrame

Current logic:

Frame getPositionedStraightStartFrame() const

{
    if (state.activeZone.active)
        return state.activeZone.frame;

    return machineEntryFrame;
}

Meaning:

Before bend:
positioned straight starts at machineEntryFrame

During bend:
positioned straight starts at activeZone.frame

Pipeflow:
before bend:
[entry] ------------------------>
        positioned straight

during bend:
[entry] ) ) ) ------------------>
        trace  positioned straight
               starts at activeZone.frame
               Phase 5S: review positioned straight start
               frame ownership
Phase 5T is another clarity phase. The behavior stays
exactly the same; we improve the intent of the code.
Goal
When someone reads:
getPositionedStraightStartFrame()
they might ask:
"Start of what?"
A clearer mental model is:
This returns the frame where the remaining straight section
begins after the current bend.

Option 1 (recommended): Keep the name, improve the comment

// =====================================================
// POSITIONED STRAIGHT START FRAME
//
// Returns the frame from which the remaining straight
// pipe should be generated.
//
// Before bending:
//     machineEntryFrame
//
// During bending:
//     activeZone.frame
//     (the current end of the growing bend)
//
// This keeps the straight section attached to the
// current bend as the bend grows.
// =====================================================
Frame getPositionedStraightStartFrame() const;

// The remaining straight section always starts where the
// current manufacturing process ends:
//
// no bend      -> machine entry
// active bend  -> end of current bend (activeZone.frame)

Option 2 (possible later)

If, after the project grows, you feel the name is still
too generic, you could rename it to
getRemainingStraightStartFrame()

or:
getStraightSectionStartFrame()
I would not do that now because:
it requires changing every call site,
the current name is already used consistently,
there is no ambiguity inside ManufacturingPipeSimulator.

Why this matters
Notice how we've been evolving the architecture

machineEntryFrame
        ?
        ?
getBendStartFrame()

activeZone.frame
        ?
        ?
getPositionedStraightStartFrame()

frozenNodes
        ?
        ?
getFrozenEndFrame()

ach helper now answers one very specific question:
Where does the bend start?

Where does the remaining straight start?

Where does the manufactured pipe end?

That consistency is much more valuable than simply renaming functions.

==========================================
Phase 5U
Review helper family for frame ownership
Goal:
Confirm we now have clear frame helpers:

getBendStartFrame()
getPositionedStraightStartFrame()
getFrozenEndFrame()
frameFromNode()
Meaning:
bend start frame          ? where bending begins
positioned straight frame ? where remaining straight begins
frozen end frame          ? where manufactured body ends
node-to-frame helper      ? conversion utility
ASCII
machineEntryFrame
      ?
getBendStartFrame()
      ?
activeZone.frame
      ?
getPositionedStraightStartFrame()
      ?
frozenNodes.back()
      ?
getFrozenEndFrame()
=========================================
Phase 5V
Review first bend lifecycle summary
Phase 5V
Review first bend lifecycle summary

Goal:
Close the first FEED ? BEND analysis with a 
clear lifecycle diagram.

Summary target:
1. feed stock
2. positionedStraight grows
3. bend starts at bendStartFrame
4. trace grows
5. active window moves
6. positionedStraight shrinks
7. freeze creates frozen geometry
ASCII:
FEED:
incoming ======> [entry] ---------------->
                     positionedStraight

BEND:
incoming ======> [entry] ) ) ) ---------->
                     trace   positioned

FREEZE:
incoming ======> [entry] ) ) ) ---------->

                     frozen geometry
===========
                     Reuse !!!!!!!!!!!!:
? PassPlacement
? PassPlacementResolver
? resolved Frame
? placement algorithms
? shared helper functions

never reuse directly:
? preview node rebuilding
? preview rendering
? preview overlays
? preview strips

Because:
ManufacturingPlanPreviewModel
    = planning / CAD preview

ManufacturingPipeSimulator
    = real manufacturing state and playback

So we reuse the algorithms and concepts, not the preview implementation.
Thisseparation will become especially valuable when implementing HelixForming 
and future multi-pass processes.


==============================================================
Phase 5X
this mismatch
actual consumed stock = 300
machine feed counter  = 302.32

this means two different concepts are mixed:
commanded feed
    = what CNC/controller tried to execute

actual consumed feed
    = what material simulator really allowed
ASCII:
CNC command:
FEED 110
   ?
controller counter keeps moving

Material stock:
only 102 mm left
   ?
simulator clamps actual feed to 102

Current ownership:
MachineController / runtime state
    owns commanded/progress feed display

ManufacturingPipeSimulator
    owns actual material consumption

Conclusion:

? ManufacturingPipeSimulator is correct to clamp stock
? machine feed counter should not silently exceed 
available stock

Best target model:
commandedFeedProgress = CNC progress
actualMaterialFeed    = consumed stock
feedLimitReached      = true/false

Let controller know how much material really moved.
Phase 5X goal:
Review whether machine feed display should show:
1. commanded feed
2. actual consumed feed
3. both
Best future answer:
commandedFeed = CNC target/progress
actualFeed    = real material consumed
CNC command:
FEED 110
   ?
machine counter may try to advance

material stock:
only 102 left
   ?
actual consumed = 102
==============================================
Phase 5Y
Return actualFeed from ManufacturingPipeSimulator::processFeed
goal:
ManufacturingPipeSimulator::processFeed()
returns actual material feed distance.
==========================================
Phase 5Z
Use actualFeed in controller feed progress

Goal:
Stop machine/controller feed progress from exceeding
available stock.

Current issue:
machine feed counter can show 302.32
while actual consumed stock is 300

Goal:
machine feed progress should use actualFeed,
not requested stepFeed.


PROJECT STRUCTURE
AppController
      ?
      ?
SimulationController
      ?
      ??? OperationQueue
      ??? MachineSystem / MachineRuntimeState
      ??? PipeSystem
             ?
             ??? GeometricPipeModel        CAD preview
             ??? ManufacturingPipeSimulator
                    ?
                    ??? ManufacturingState
                           ??? incomingStock
                           ??? positionedStraight
                           ??? activeZone
                           ??? currentBendTraceNodes
                           ??? frozenNodes
==================================================




ManufacturingPipeSimulator should explicitly know:
incoming stock is exhausted.

AppController
      ?
      ?
SimulationController
      ?
      ??? OperationQueue
      ??? MachineSystem / MachineRuntimeState
      ??? PipeSystem
             ?
             ??? GeometricPipeModel        CAD preview
             ??? ManufacturingPipeSimulator
                    ?
                    ??? ManufacturingState
                           ??? incomingStock
                           ??? positionedStraight
                           ??? activeZone
                           ??? currentBendTraceNodes
                           ??? frozenNodes

resposibility:
SimulationController
    = controls program execution
    = FEED / ROTATE / BEND timing
    = operation progress

ManufacturingPipeSimulator
    = controls physical material state
    = actual stock consumption
    = zones / frozen / trace / active window
=================================================
Phase 6B
Add stock exhaustion flag/status 
ASCII:
incoming stock
300 ? ... ? 0
              ?
        exhausted = true


New stock loaded
        ?
        ?
exhausted = false
        ?
        ?
FEED operations
        ?
        ?
remainingLength reaches 0
        ?
        ?
exhausted = true
======================================================
Phase 6C
Propagate stock exhaustion status to SimulationController

Goal:
Keep the simulator responsible for the material state, and let the 
controller decide what to do:
Flow:
ManufacturingPipeSimulator
        ?
        ? isIncomingStockExhausted()
        ?
SimulationController
        ?
        ??? stop operation
        ??? update UI
        ??? record manufacturing history
        ??? request new stock
        ??? continue with multi-pass workflow
=========
For multi-pass manufacturing, stock exhaustion is not an error. 
It becomes an event:

Stock exhausted
        ?
        ?
Pause manufacturing
        ?
        ?
Load new stock
        ?
        ?
Resume from saved manufacturing state

Phase 6C: propagate stock exhaustion state to controller

I recommend this approach because it keeps responsibilities clean:
the simulator reports
what happened, while the controller decides what to do next. 
This will scale naturally when you add multi-pass and 
helix-forming support.

=======================================
Phase 6D
Add feed stop reason enum placeholder
Goal:
Prepare controller for more than one stop reason.
Future reason may be:

incoming stock exhausted
collision detected
machine limit reached
operator pause
material constraint exceeded

processFeed()
      ?
actualFeed = 0
      ?
check simulator state
      ?
FeedStopReason::IncomingStockExhausted
      ?
controller decides next action


SimulationController
?
??? program data
??? PipeSystem
??? MachineSystem
??? runtime execution
?      ??? playing
?      ??? accumulatedDistance
?      ??? accumulatedAngle
?      ??? accumulatedRotation
?      ??? lastFeedStopReason
?
??? helper methods
FeedStopReason is runtime execution state,
just like:

playing
paused
accumulatedDistance
accumulatedAngle


accumulatedRotation
DEbugging

AppController
?
??? configureManufacturingDebug()
?      ?
?      ??? ManufacturingPipeSimulator
?
??? configureControllerDebug()
       ?
       ??? SimulationController

       This keep the debug configuration cleanly separated
       between the two main components.
       ManufacturingPipeSimulator
    owns manufacturing debug

SimulationController
    owns execution/controller debug

AppController
    configures both

    ==========================================
Phase 6G
Separate controller debug configuration helper

Goal:
Keep AppController clean and separate:

manufacturing debug
controller/debug execution

===============================
AppController
?
??? configureManufacturingDebug()
?      ?
?      ??? ManufacturingPipeSimulator
?             ??? Snapshot
?             ??? ActiveWindow
?             ??? BendStep
?             ??? ...
?
??? configureControllerDebug()
       ?
       ??? SimulationController
              ??? OperationStopReason
              ??? controller diagnostics

 This is exactly the kind of separation that will pay off

 ================================================
 Phase 6H
 Review manufacturing responsibilities after refactoring
 Goal:
 Make sure every responsibility has exactly one owner.

 AppController
    ?
    ?
SimulationController
    ?
    ??? operation execution
    ??? timing
    ??? progress
    ??? stop reasons
    ?
    ?
ManufacturingPipeSimulator
    ?
    ??? incoming stock
    ??? positioned straight
    ??? active bend
    ??? bend trace
    ??? frozen geometry
    ??? visible reconstruction
    ??? material state

    Geometry ownership
    ? ManufacturingPipeSimulator

Material ownership
    ? ManufacturingPipeSimulator

Operation execution
    ? SimulationController

Program sequencing
    ? OperationQueue

Machine runtime state
    ? MachineSystem

Rendering
    ? GLView

    This is the last architectural checkpoint before we
    start introducing more advanced manufacturing capabilities.

    The next major features you mentioned are:

    • Multi-pass manufacturing
• Helix forming
• Two-roller forming
• Additional machine kinematics

All of them should fit into the architecture we've been building,
not force us to redesign it.


=========================================================
=========================================================

what pass to run
where it enters
whether it is enabled
future constraints / selected region

It should not directly:
modify frozenNodes
rebuild render data
splice preview curves
draw overlays

Execution boundary:

AdditionalFormingPass
        ? data
        ?
ManufacturingPipeSimulator
        ? execution
        ?
ManufacturingState updated
ASCII:
already formed pipe
      ?
      ?
AdditionalFormingPass
      ??? pass
      ??? entryFrame
      ??? enabled
              ?
              ?
ManufacturingPipeSimulator executes
              ?
              ?
updated frozen geometry

Minimum current boundary:
AdditionalFormingPass provides entryFrame.
ManufacturingPipeSimulator decides how to use it.

Phase 7C.
Add execution placeholder for AdditionalFormingPass.
No geometry change yet.

========================
Phase 7E
Review ManufacturingHistory ownership in SimulationController

Commit topic
Chcemy ustaliæ, kto trzyma ManufacturingHistory, kto j¹ buduje,
i kto powinien wywo³aæ real execution dla  AdditionalFormingPass
Current ownership should be:


SimulationController
    ??? owns ManufacturingHistory

Because SimulationController already owns:

operation execution
program sequencing
manufacturing playback control

ManufacturingHistory should not be owned by:
GLView              no, rendering only
ManufacturingPipeSimulator  no, physical execution only
AppController       no, setup/config only

Correct execution flow:

SimulationController
      ?
      ??? has ManufacturingHistory
      ?
      ??? selects primary/additional pass
      ?
      ?
ManufacturingPipeSimulator
      ?
      ??? executes selected pass
      ASCII:
ManufacturingHistory
   ??? primaryPasses
   ??? additionalPasses
            ?
            ?
SimulationController chooses pass
            ?
            ?
ManufacturingPipeSimulator executes
Conclusion:
? ManufacturingHistory ownership in SimulationController is correct
? ManufacturingPipeSimulator should not own the whole history
? it should only execute one pass given to it
==========================================================

Phase 7F
Add debug helper to execute first additional pass placeholder

Zaczynam Phase 7F jako debug-only integration test. 
Wywo³amy placeholder dla s, ale nadal bez zmiany geometrii i
bez realnego multi-pass  execution.
Goal:
Call executeAdditionalFormingPass()
for the first additional pass,
only as a debug placeholder test.
===================================================
Phase 7H
Add additional pass execution result enum

Goal:
Replace simple bool return from:

executeAdditionalFormingPass(...)

with a clearer result:
Executed
Disabled
UnsupportedProcess
InvalidEntryFrame

Why:
bool only says success/failure.
Enum says why.


=====
Replace bool result with meaningful execution result.
pipeflow:

AdditionalFormingPass
        ?
        ?
executeAdditionalFormingPass()
        ?
        ??? disabled ----------> Disabled
        ??? bad entry frame ---> InvalidEntryFrame
        ??? unsupported type --> UnsupportedProcess
        ??? accepted ----------> Executed

==================================================
Private Public
Rule I personally follow

I use one simple question:

Who needs this function?
If the answer is:

Only ManufacturingPipeSimulator

Then it is a private member function.

If the answer is:

SimulationController

AppController

GLView

other systems
Then it is a public member function.
If the answer is:

SimulationController

AppController

GLView

other systems
          PUBLIC
             -
             ¡
      -¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¬
      - ManufacturingPipeSim -
      -                      -
      -  processFeed()       -
      -  processBend()       -
      -  executePass()       -
      -                      -
      - -------------------- -
      -                      -
      - isValidFrame()       -
      - rebuildRenderData()  -
      - freezeActiveZone()   -
      - buildTrace()         -
      -                      -
      L¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦¦-
             ^
             -
          PRIVATE

Everything below the line is the internal mechanism.
The outside world should only
press the "buttons" at the top.


My recommendation (best architecture)

I would not make it a member of ManufacturingPipeSimulator at all.

Instead, put it next to the enum as a free utility function.

For example:

enum class AdditionalPassExecutionResult
{
    Validated,
    Executed,
    Disabled,
    UnsupportedProcess,
    InvalidEntryFrame
};

const char* additionalPassExecutionResultToString(
    AdditionalPassExecutionResult result
);

Then implement it in the .cpp.

Now everyone can use it:

SimulationController
        -
        +¦¦ debug
        -
HUD ----+¦¦ toString()
        -
Logger -+
        -
Tests ---

Notice:

It belongs to the RESULT TYPE,
not to ManufacturingPipeSimulator.

That's exactly the same idea as:

testPlacementPresetToString(...)
renderModeToString(...)
bendDirectionToString(...)

Those functions aren't methods of a class—they're 
utilities associated with an enum.

Rule of thumb
Does the function operate on the simulator itself?



exampleExample:

processFeed()
processBend()
freezeActiveZone()

it's member of ManufacturingPipeSimulator because it 
operates on the simulator state.



Does the function simply convert an enum to text?


Free helper function

recomended:
AdditionalPassExecutionResult
            -
            ¡
additionalPassExecutionResultToString()
            ^
            -
Used by:
    ? SimulationController
    ? HUD
    ? Logger
    ? Unit tests

    This keeps ManufacturingPipeSimulator focused on simulation,
    while string conversion stays with the type it describes. 
    I think this is the cleanest architecture, 
    especially as your project grows and more
    systems (HUD, logs, diagnostics, history export) 
    need to display the same result consistently.

    ==============================

    Phase 7L — Option 3: shared free helper

    Goal:

AdditionalPassExecutionResult
        ¡
shared label helper
        ¡
HUD / logger / controller / tests


====================================
NO visible in console output for AdditionalPassExecutionResult
========================================
This is actually the most important point.

Playback mode has nothing to do with it.

The message is not printed during playback. 
It is printed only when:

SimulationController::debugExecuteFirstAdditionalPassPlaceholder()
is executed.

Let's trace it

Current call chain
AppController
      ?
      ?
rebuildTestManufacturingPlan()
      ?
      ?  (only if DEBUG_EXECUTE_ADDITIONAL_PASS_PLACEHOLDER == true)
      ?
SimulationController::debugExecuteFirstAdditionalPassPlaceholder()
      ?
      ?
executeAdditionalFormingPass()
      ?
      ?
std::cout << "[ADDITIONAL PASS RESULT]"

Notice:

Playback
      ?
      ??????? NO CONNECTION


      The important order is:
      build plan
    ?
set preview plan
    ?
build ManufacturingHistory
    ?
execute additional-pass placeholder
    ?
print/debug history

========================================
Phase 7M
Store last additional-pass execution result
Goal:

SimulationController should keep the latest result
for HUD, diagnostics, or future execution flow.

==========================================================
Phase 7N
Add getter for last additional-pass execution result

Goal:
Allow HUD or diagnostics to read the stored result
without exposing the private member directly.

SimulationController
    ??? private stored result
              ?
             getter
              ?
        HUD / diagnostics
=========================================================
Phase 7O
Expose additional-pass result in HUD data
Goal:
Move the stored result one step closer to the UI
without letting HUD access SimulationController internals directly.
Target flow:
ManufacturingPipeSimulator
        ?
AdditionalPassExecutionResult
        ?
SimulationController stores result
        ?
AppController builds HUDData
        ?
HUD displays label


===================================================
Phase 7Q.
We're not designing a new HUD element—we're 
just exposing information
that already exists.

Goal:   
ManufacturingPipeSimulator
        ?
        ?
AdditionalPassExecutionResult
        ?
        ?
SimulationController
        ?
        ?
HUDData

After this phase:
ManufacturingPipeSimulator
        ?
        ?
SimulationController
        ?
        ?
HUDData
        ?
        ?
HUD Renderer
        ?
        ?
Additional pass: Validated

PREVIEW SIDE                     REAL EXECUTION SIDE

plan preview nodes               AdditionalFormingPass
curve splicing                           ?
overlays                                 ?
      ?                         ManufacturingPipeSimulator
      ?                                   ?
      ????? stay separate                 ?
                                  ManufacturingState

=======================================
Segment 8
First real additional-pass geometry execution

Starting phase:
Phase 8A
Define deformable region selection

Define which part of already manufactured geometry
may be modified by an AdditionalFormingPass.

Current state:
frozenNodes
    = complete manufactured pipe geometry

For multi-pass forming, we must not automatically deform all nodes.

We need a selected region:

before region
deformable region
after region
ASCII pipeflow:

frozen pipe:

[0] ======== [start] ~~~~~~~~~~~~~ [end] ======== [last]
              ?                         ?
              ??? deformable region ?????

During additional pass:

before region
    stays unchanged

selected region
    is transformed/deformed

after region
    may move rigidly or remain fixed,
    depending on the machine process


struct DeformableRegion
{
    double startArcLength = 0.0;
    double endArcLength = 0.0;
};

Why arc length first:

? independent from sampling density
? works if node count changes
? natural for manufacturing dimensions
? compatible with PassPlacementResolver concepts

Do not use only node indices as permanent process data:

nodeIndex 404

because after geometry rebuilding:

node 404 may represent a different physical location

Better:

startArcLength = 202.0 mm
endArcLength   = 402.0 mm
Relation to entryFrame

The additional pass already has:

entryFrame

That tells us where the pass begins spatially.

The deformable region tells us:

how much pipe after that point belongs to the pass

ASCII:

AdditionalFormingPass.entryFrame
              ?
              ?
pipe =========|~~~~~~~~~~~~~~~~~~~~~~~~=========
              <--- deformable length --->

uture structure

Eventually AdditionalFormingPass may contain:

Frame entryFrame;

double deformableStartArcLength = 0.0;
double deformableLength = 0.0;

or a separate object:

DeformableRegion deformableRegion;
I recommend the separate object because other 
processes may need more selection modes later:
ArcLengthRange
NodeRange
WholePipe
RegionAroundFrame
Phase 8A conclusion

For the first real implementation:

source geometry:
    ManufacturingState::frozenNodes

selection coordinates:
    arc-length range

first process target:
    nodes between startArcLength and endArcLength
No geometry changes yet.