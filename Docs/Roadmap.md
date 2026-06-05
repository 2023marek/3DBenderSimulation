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