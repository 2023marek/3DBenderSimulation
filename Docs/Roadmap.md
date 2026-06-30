? Architecture design

? Refactoring plans

? Pipe geometry mathematics

? Frenet / Parallel Transport Frames

? Curvature/torsion representations

? Helix modeling

? Variable-curvature pipe representation

? STL / CAD integration

? Machine simulation architecture

? Collision architecture

? Manufacturing pro
======================================================================
Looking at what we've done, I still have a pretty coherent picture:

AppController
    ?
SimulationController
    ?
PipeSystem
        ?? GeometricPipeModel
        ?? ManufacturingPipeSimulator

MachineSystem
        ?? MachineModel
        ?? MachineRuntimeState
        ?? MachineRenderData

GLView
        ?? PipeRenderer
        ?? MachineRenderer
        ?? HUD

and I understand the major architectural decisions we've made:

CADPreview
    uses GeometricPipeModel

ManufacturingPlayback
    uses ManufacturingPipeSimulator

Machine state separated from pipe state

STL machine assets now supported

PipeAxis3D archived

Rendering separated from simulation

MachineRenderer owns machine drawing

and now your new strategic direction:

operations
    ?
segment/curvature model
    ?
physics
    ?
sampling/discretization
    ?
render nodes
========================================================================

Create common segment representation:
    CAD and manufacturing can both use it later.

Do not refactor GeometricPipeModel yet.
Do not refactor ManufacturingPipeSimulator yet.
No rendering changes.

ARCHITECTURE
Space = Play
P     = Pause
R     = Reset
S     = Step

M     = Toggle LINE / MESH
N     = Toggle simulation mode
T     = Toggle placement preset


USER / CAD / CNC
        ?
        ?
????????????????????????????
? GeometricPipeModel       ?
? CAD / YBC / XYZ          ?
? deterministic geometry   ?
? editable history         ?
????????????????????????????
            ? ideal geometry
            ?
????????????????????????????
? PhysicalPipeSimulator    ?
? manufacturing behavior   ?
? springback               ?
? material state           ?
? tooling interaction      ?
????????????????????????????
            ? physical geometry
            ?
????????????????????????????
? CollisionSystem          ?
? pipe vs machine          ?
? pipe vs pipe             ?
? swept-volume checks      ?
????????????????????????????
            ? warnings / contacts
            ?
????????????????????????????
? Visualization            ?
? pipe, dimensions, alerts ?
? training overlays        ?
????????????????????????????

Core/
 ??? Geometry/
 ?    ??? GeometricPipeModel.h
 ?
 ??? Manufacturing/
 ?    ??? ManufacturingPipeSimulator.h
 ?    ??? ActiveZone.h
 ?    ??? SpringbackModel.h
 ?
 ??? Machine/
 ?    ??? MachineModel.h
 ?    ??? MachineState.h
 ?
 ??? Dimension/
 ?    ??? DimensionSystem.h
 ?    ??? DimensionData.h
 ?
 ??? Collision/
      ??? CollisionSystem.h
      ??? CollisionReport.h

      MachineModel
    static tooling geometry
    bend die, clamp, entry length, machine envelope

MachineState
    runtime state
    current status, time, rotation, current operation

GeometricPipeModel
    CAD logic
    operation history
    rebuildable ideal geometry
    YBC ? XYZ
    import/export

ManufacturingPipeSimulator
    physical process
    incoming stock
    positioned straight
    active bend zone
    frozen geometry
    rigid body pipe motion

SpringbackModel / SpringbackSolver
    material unloading behavior
    commanded angle ? final angle
    overbend calculation

CollisionSystem
    read-only safety check
    pipe vs machine
    pipe vs pipe
    pipe vs tooling

DimensionSystem
    read-only measurement
    lengths, angles, bend locations, labels

Visualization
    render geometry
    HUD
    dimension overlays
    collision warnings
    training markers

    AppController
    ?
    ??? Program / Operation input
    ?       FEED / ROTATE / BEND
    ?
    ??? SimulationController
    ?       playback, timing, mode switching
    ?
    ??? GeometricPipeModel
    ?       CAD preview
    ?       operation history
    ?       rebuildable ideal geometry
    ?       YBC ? XYZ
    ?       editing
    ?       import/export
    ?
    ??? ManufacturingPipeSimulator
    ?       incoming stock
    ?       positioned straight
    ?       active bend zone
    ?       current bend trace
    ?       frozen geometry
    ?       rigid body rotate/feed behavior
    ?
    ??? PhysicalPipeSimulator
    ?       springback
    ?       material model
    ?       strain/residual state
    ?       tooling/material interaction
    ?
    ??? CollisionSystem
    ?       pipe vs machine
    ?       pipe vs pipe
    ?       pipe vs tooling
    ?       warnings/contact reports
    ?
    ??? DimensionSystem
    ?       lengths
    ?       angles
    ?       bend locations
    ?       cut length
    ?       labels/measurement overlays
    ?
    ??? Visualization
            GLView
            PipeRenderer
            DimensionRenderer
            MachineRenderer
            DebugOverlayRenderer

            GeometricPipeModel
    Owns ideal CAD geometry.

    Responsibilities:
        addFeed()
        addBend()
        addRotate()
        edit operation history
        rebuild from scratch
        create ideal centerline
        create geometric segments
        support YBC ? XYZ
        provide data for dimensions/export

        ManufacturingPipeSimulator
    Owns process state.

    Responsibilities:
        processFeed()
        processRotate()
        processBend()
        incomingStock
        positionedStraight
        activeZone
        currentBendTrace
        frozenGeometry
        machineEntryFrame
        rotation kinematic mode:
            PipeRoll
            ToolHeadRotate

            PhysicalPipeSimulator
    Owns non-ideal material behavior.

    Responsibilities:
        springback
        overbend calculation
        material properties
        residual strain state
        plastic/elastic approximation

        CollisionSystem
    Owns safety/contact checks.

    Responsibilities:
        pipe vs machine
        pipe vs pipe
        pipe vs tooling
        collision report
        warning overlays

        DimensionSystem
    Owns read-only measurement.

    Responsibilities:
        total length
        arc length
        bend angle labels
        bend locations
        point distances
        cut length
        bounding box

        Visualization
    Owns drawing only.

    Responsibilities:
        draw CAD pipe
        draw manufacturing zones
        draw machine frame
        draw bend plane marker
        draw orientation stripe
        draw dimensions
        draw collision warnings

                         ??????????????????????
                 ?   AppController    ?
                 ??????????????????????
                           ?
                           ?
                 ??????????????????????
                 ? SimulationController?
                 ? mode / time / step ?
                 ??????????????????????
                         ?     ?
              CADPreview ?     ? ManufacturingPlayback
                         ?     ?
                         ?     ?
        ????????????????????   ??????????????????????????
        ? GeometricPipeModel?   ? ManufacturingSimulator ?
        ? ideal CAD pipe    ?   ? process simulation     ?
        ? rebuildable       ?   ? four-zone pipe model   ?
        ????????????????????   ??????????????????????????
                 ?                         ?
                 ? ideal geometry          ? manufacturing geometry
                 ?                         ?
                 ?              ????????????????????????
                 ?              ? PhysicalPipeSimulator ?
                 ?              ? springback/material   ?
                 ?              ????????????????????????
                 ?                          ?
                 ????????????????????????????
                                ?
                       ???????????????????
                       ? CollisionSystem ?
                       ???????????????????
                                ?
                       ???????????????????
                       ? DimensionSystem ?
                       ???????????????????
                                ?
                       ???????????????????
                       ? Visualization   ?
                       ???????????????????






                       =============================

                ??????????????????????
                ?   MainWindow       ?
                ??????????????????????
                          ?
                          ?
                ??????????????????????
                ?   AppController    ?
                ??????????????????????
                          ?
        ?????????????????????????????????????
        ?                 ?                 ?
        ?                 ?                 ?
????????????????  ????????????????  ????????????????
? Simulation   ?  ? MachineModel ?  ? DimensionSys ?
? Controller   ?  ? static model ?  ? measurements ?
????????????????  ????????????????  ????????????????
       ?                 ?                 ?
       ?                 ?                 ?
????????????????   MachineParts     DimensionData
? Pipe Systems ?
????????????????
       ?
       ??? GeometricPipeModel
       ?      CAD preview
       ?      operation history
       ?      rebuildable geometry
       ?      YBC ? XYZ
       ?
       ??? ManufacturingPipeSimulator
       ?      incoming stock
       ?      positioned straight
       ?      active bend zone
       ?      bend trace
       ?      frozen geometry
       ?      process FEED / ROTATE / BEND
       ?
       ??? PhysicalPipeSimulator
       ?      springback
       ?      material behavior
       ?      strain/residual state
       ?
       ??? CollisionSystem
              pipe vs machine
              pipe vs pipe
              warnings

       ?                 ?                 ?
       ?????????????????????????????????????
                  ?             ?
              GLView  — render orchestration
                       ?
        ?????????????????????????????????
        ?              ?                ?
 PipeRenderer     MachineRenderer   DimensionRenderer
        ?              ?                ?
        ?????????????????????????????????
                       ?
                    HUDPanel
                       ?
                    SCREEN

====================================================================

                    THE REAL MEANING OF ACTIVE ZONE

The active zone must behave like:
Frozen ======\\\\\\------ Incoming
               ^
          Active Zone



THE SIMULATOR CORE


                       PROCEDURAL HISTORY
                     ops
                      ?
                      ?
             buildSegments()
                      ?
                      ?
                 segments
                      ?
                      ?
            executeSegments()
         ???????????????????????
         ?                     ?
         ?                     ?
   frozenNodes         activeZone.localNodes
         ?                     ?
         ???????????????????????
                    ?
                  nodes
                    ?
                renderer


                         USER / CNC PROGRAM
                                ?
              ?????????????????????????????????????
              ?                                   ?
              ?                                   ?
        CAD GEOMETRY MODE              MANUFACTURING MODE
   full deterministic rebuild        persistent state evolution
              ?                                   ?
              ?                                   ?
     final designed pipe              incoming + active + frozen
              ?                                   ?
              ?????????????????????????????????????
                                ?
                             Renderer







                             AppController
     ?
     ?
sim.setMode(...)
     ?
     ?
SimulationController::mode changes
     ?
     ?
SimulationController::update()
     ?
     ??? CADPreview
     ?       ??? updatePipeGeometryCAD()
     ?
     ??? ManufacturingPlayback
             ??? updatePipeGeometryManufacturing()




             currentFrame
     ?
     ?
processBend()
     ?
     ??? start active zone from currentFrame
     ??? bend only by angleIncrement
     ??? update active zone
     ??? freeze if bend is complete
     ??? commit final frame

     ===============================

     Manufacturing Mode
     The target behavior is different from CADPreview.

CADPreview means:operations ? final pipe geometry

Manufacturing mode means:
operations ? material flow through machine

finally we used 4  zone model

ZONE 1                    ZONE 2                    ZONE 3
Incoming Stock            Active Bend Zone           Frozen Geometry

tail ====================> [ DIE / ENTRY ] ))))))) ????????????
                             ?                         ?
                             ?                         ?
                       machineEntryFrame           currentFrame

                       finally 
                       manufacturing zones become
ZONE 1 — Incoming Stock
raw material before machine entry

ZONE 2 — Positioned Straight Section
material already fed through the machine, still straight, not yet bent

ZONE 3 — Active Bend Zone
local deformation / bend window

ZONE 4 — Frozen Geometry
finished manufactured pipe

tail ==========> [ machineEntryFrame ] ----------> \\\\\\\\ )))))))
     incoming        machine entry       positioned    active   frozen
      stock                            straight       bend    geometry


what material is still waiting
what material has already been fed
what part is currently bending
what part is finished
========

machineEntryFrame
    owns active zone start position

activeZone
    owns temporary bending geometry

incomingStock
    owns remaining/consumed stock length

frozenNodes
    owns finished geometry, but M5 will improve transfer


    ======================

    Frozen Geometry Transfer.
    Active Bend Zone
      ?
old material leaves active zone
      ?
moves into Frozen Geometry
      ?
active zone stays small

pipeflow:
Incoming Stock       Active Bend Zone        Frozen Geometry
=======> [entry]     \\\\\\\\\\\\\\          )))))))))))))
                      ^        |
                      |        ??? oldest active node freezes
                      ??? newest deformation node enters

activeZone.localNodes
    owns only currently deforming material

frozenNodes
    owns material that already left the active bend window

============

What should happen during a bend
updateActiveZone(stepAngle)
    ?
new active node is added
    ?
if active window too long:
    oldest active node moves to frozenNodes
    ?
when bend target complete:
    remaining active nodes move to frozenNodes

    ====================
    BEND step:

previousActiveFrame
        ?
        ?
update active zone
        ?
        ?
newActiveFrame
        ?
        ?
transform frozen geometry
        ?
        ?
frozen body follows active bend


Before one bend step:

[entry] \\      )))))))
        active  frozen body


After one bend step:

[entry] \\\)))))))
        active + frozen body moved rigidly

activeLength = "how much pipe is inside the bending machine window"

it controls:
how local the deformation looks
how quickly nodes freeze
how much pipe is considered currently bending


New 4 zone pipeflow:
ZONE 1
Incoming Stock
raw material before machine entry

ZONE 2
Positioned Straight
material already fed through machine, still straight

ZONE 3
Active Bend Zone
local deformation window

ZONE 4
Frozen Geometry
finished manufactured shape

ASCII
tail ===================> [ machineEntryFrame ] -----------> \\\\\\\\ )))))))
       incoming stock              entry        positioned     active   frozen
                                                straight       zone     geometry

    ==============================
    Target renderer pipeflow

    PipeAxis3D
  ?
nodes = incoming + positioned + active + frozen
  ?
renderer draws one GL_LINE_STRIP

PipeAxis3D
  ?
ManufacturingRenderData
  ??? incomingStockNodes
  ??? positionedStraightNodes
  ??? activeZoneNodes
  ??? frozenNodes
        ?
renderer draws each group separately


================================
Correct visual pipeflow

Before bend:

tail ===================> [entry] ------------------------->
      incoming stock          positioned straight = 120


During bend:

tail ===================> [entry] ) ) ) ------>
                            bend trace  positionedStraight shrinking


After 180° bend:

tail ===================> [entry]
                            )
                           )
                          )
                         (
                          <---------------- positionedStraight ? 57.17



==================

During bebd we need:
machineEntryFrame fixed
        ?
        ?
[entry] ) ) ) ) ) ------>
        formed arc    positioned straight


We should separate:

activeZone.localNodes
    = small moving active window

currentBendTraceNodes
    = full visible arc formed during current bend



    ASCII
    [entry] ) ) ) ) ) ------>
        ?         ?
        ?         ??? positionedStraight follows arc end frame
        ?
        ??? currentBendTraceNodes, anchored at entry

                 \\\
                 activeZone.localNodes, small moving window


    Renderer draw order

In manufacturing mode, draw:

incoming stock
positioned straight
frozen geometry
current bend trace
active zone


ASCII
tail ===================> [entry] ) ) ) ) ) <---------
        incoming             trace arc     positioned straight
                              red/green       yellow

                              \\\
                              active window drawn last


Incoming stock          draw strip 1
Positioned straight     draw strip 2
Current bend trace      draw strip 3
Frozen geometry         draw strip 4
Active zone             draw strip 5


processBend()
    ??? calculate previous frozen attachment frame
    ??? consume positionedStraight.length
    ??? updateActiveZone()
    ??? calculate new frozen attachment frame
    ??? transform old frozen geometry
    ??? if complete: freezeActiveZone()

    updateActiveZone()
    ??? rotate active frame
    ??? integrate active frame position
    ??? add node to active window
    ??? add node to bend trace
    ??? maintain active window




    paintGL()
  ?
  ??? Manufacturing + LINE
  ?       ??? upload separated line strips
  ?
  ??? Manufacturing + MESH
  ?       ??? draw tube per zone
  ?
  ??? CADPreview
          ??? draw one continuous CAD pipe



          =========================================
          =========================================

          retReturns us to the real manufacturing concept. We’ll add a model placeholder for

          “already formed pipe + additional forming pass,” without changing current playback
          Prepare additional forming pass model
          Represent real manufacturing continuation:

already formed pipe
    ?
additional forming pass
    ?
updated manufacturing state:
Create AdditionalFormingPass.h
#pragma once

#include <string>

#include "Core/Forming/ManufacturingPass.h"
#include "Core/Geometry/Frame.h"

// =====================================================
// ADDITIONAL FORMING PASS
//
// Real manufacturing concept.
//
// Used when a pipe is already partially or fully formed,
// then another operation is performed on that same physical pipe.
//
// This is NOT planned-preview insertion.
// This belongs to manufacturing history / process simulation.
//
// Examples:
// - manually add one bend after previous bending
// - add helix after rotary draw bending
// - continue forming in another machine
// =====================================================

struct AdditionalFormingPass
{
    std::string name;

    // The forming operation/pass to apply next.
    ManufacturingPass pass;

    // Frame where the already formed pipe enters the next process.
    Frame entryFrame;

    // Future:
    // - selected deformable region
    // - clamp points
    // - machine type
    // - process constraints
    // - collision validation
    bool enabled = true;
};

Create ManufacturingHistory.h

#pragma once

#include <vector>

#include "Core/Forming/ManufacturingPass.h"
#include "Core/Forming/AdditionalFormingPass.h"

// =====================================================
// MANUFACTURING HISTORY
//
// Real process-history container.
//
// This is different from PlannedShapePreview.
//
// PlannedShapePreview:
//     What final shape do I want?
//
// ManufacturingHistory:
//     How was this pipe physically made?
//
// Future flow:
//
//     primary pass
//         ?
//     additional forming pass
//         ?
//     additional forming pass
//         ?
//     final manufactured state
// =====================================================

struct ManufacturingHistory
{
    std::vector<ManufacturingPass> primaryPasses;
    std::vector<AdditionalFormingPass> additionalPasses;

    void clear()
    {
        primaryPasses.clear();
        additionalPasses.clear();
    }

    bool empty() const
    {
        return primaryPasses.empty()
            && additionalPasses.empty();
    }
};

Add temporary compile test in AppController
#include "Core/Forming/ManufacturingHistory.h"
ManufacturingHistory history;

history.primaryPasses.push_back(
    rotaryPass
);

AdditionalFormingPass extra;

extra.name =
    "Additional helix forming pass";

extra.pass =
    helixPass;

extra.entryFrame.P = { 0.0, 0.0, 0.0 };
extra.entryFrame.T = { 1.0, 0.0, 0.0 };
extra.entryFrame.N = { 0.0, 1.0, 0.0 };
extra.entryFrame.B = { 0.0, 0.0, 1.0 };

history.additionalPasses.push_back(
    extra
);

std::cout << "[MFG HISTORY TEST] primary="
          << history.primaryPasses.size()
          << " additional="
          << history.additionalPasses.size()
          << std::endl;



====================================================================
          NEW CHAT ROAD MAP

=====================================================================
==========================
ROADMAP
==========================

? Phase 1A
ManufacturingHistory

? Phase 1B
ManufacturingPass execution

? Phase 2A
ManufacturingRenderData cleanup

? Phase 2B
CurrentBendTrace ownership

? Phase 3A
PipeAxis3D becomes CAD only

? Phase 4A
Springback placeholder

? Phase 5A
Collision framework




...

========================================================================

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




    ===========================
     const char* processTypeToString(
        TubeFormingProcessType type
    )
    {
        switch (type)
        {
        case TubeFormingProcessType::RotaryDrawBending:
            return "RotaryDraw";

        case TubeFormingProcessType::HelixForming:
            return "HelixForming";

        case TubeFormingProcessType::TwoRollerContinuous:
            return "TwoRollerContinuous";

        case TubeFormingProcessType::StretchBending:
            return "StretchBending";

        default:
            return "Unknown";
        }
    }