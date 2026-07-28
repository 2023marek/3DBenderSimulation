Segment 10 — predicted phases
Stretch bending with tension, strain, springback, 
and manufacturing playback

The likely roadmap is:

10A  Define pipe section, material, and process inputs
10B  Define evaluation result and feasibility statuses
10C  Implement section-property and strain calculations
10D  Evaluate anti-wrinkle and cracking limits
10E  Calculate axial tension and bending moment
10F  Build stretch-bending curvature/torsion profile
10G  Generate standalone stretch-bending geometry
10H  Add springback approximation
10I  Validate geometry and material results
10J  Define stretch-bending manufacturing state
10K  Define fixed active zone and moving material flow
10L  Implement incremental stretch-forming execution
10M  Build current stretch trace
10N  Commit completed output to frozen geometry
10O  Expose stretch zones in ManufacturingRenderData
10P  Render LINE-mode stretch-bending playback
10Q  Render MESH-mode stretch-bending playback
10R  Add HUD/debug diagnostics and visibility controls
10S  Test reset, replay, and stock exhaustion
10T  Final visual and architectural review
Block 1 — material feasibility
Phase 10B

Define:

StretchBendingEvaluationResult
StretchBendingEvaluationStatus

Possible statuses:

Valid
InvalidInput
GeometryNotFeasible
InnerWallCompressionRisk
OuterWallStrainExceeded
BelowYield
InvalidSection
NumericalFailure
Phase 10C

Calculate:

inner diameter
area A
second moment I
yield strain
bending strain ?b = ?D/2
inner strain
outer strain
Phase 10D

Evaluate:

?inner >= 0
?outer <= ?allow
?D <= ?allow

This establishes whether the requested geometry 
and stretch strain are physically plausible.

Phase 10E

Calculate machine/material values:

axial tension T = EA?0
elastic bending moment M = EI?
yield curvature
strain safety margins
Block 2 — geometry generation
Phase 10F

Convert the accepted process input into:

CurvatureTorsionProfile

Initially:

constant ?
constant ?
Phase 10G

Run the Segment 9 integrator and store a standalone stretch-bending result.

This gives the first geometry preview without touching manufacturing state.

Phase 10H

Apply a first springback model:

loaded curvature
        ? unloading
final curvature

Likely first implementation:

?load = ?target + compensation
Phase 10I

Compare:

target geometry
loaded geometry
springback-corrected geometry
material limits

Rendering of the standalone result should be 
the acceptance gate for this block.

Block 3 — manufacturing-state ownership
Phase 10J

Add process-specific state:

StretchActiveZone
CurrentStretchTrace

while preserving shared:

IncomingStock
PositionedStraight
FrozenGeometry
Phase 10K

Model your lathe-like concept:

PositionedStraight moves continuously
ActiveZone remains fixed at tangent contact
formed material exits downstream

ASCII:

moving straight stock ??????????????
                              [fixed active zone]
                                      \
                                       \ formed output
Phase 10L

Implement incremental execution:

feed increment
    ?
consume positioned straight
    ?
integrate one new formed segment
    ?
update active zone and trace
Phase 10M

Accumulate:

CurrentStretchTrace

Definition:

geometry already generated during the current operation,
outside the small fixed active contact zone,
but not yet committed as completed frozen geometry
Phase 10N

At operation completion:

CurrentStretchTrace
        ?
FrozenGeometry
Block 4 — rendering and validation
Phase 10O

Expose:

incoming stock
moving positioned straight
stretch active zone
current stretch trace
frozen geometry

through ManufacturingRenderData.

Phase 10P

LINE mode validation:

straight material visibly moves
active zone remains fixed
formed trace grows continuously
Phase 10Q

MESH mode validation:

round tube
stable frames
no flattening
no sudden frame flips
Phase 10R

Add:

HUD status
tension
inner/outer strain
curvature
springback compensation
debug toggles
Phase 10S

Test:

reset
second playback
pause/resume
stock exhaustion
invalid process input
operation completion
Phase 10T

Final review.

Acceptance should require:

? mathematical feasibility correct
? material values correct
? geometry matches target
? active zone remains fixed
? material visibly flows through it
? LINE rendering correct
? MESH rendering correct
? rotary-draw path unchanged
? reset and replay stable

The likely first major visual milestone is
around Phase 10G for standalone geometry, while real
four-zone manufacturing playback should appear around Phases 10P–10Q.


=======================================================
Phase 10A
Define stretch-bending pipe section, material, and process input types

The purpose of Phase 10A is to define the data needed by the
stretch-bending model before any machine state, integration,
or rendering is added.

The separation should be:

Pipe section
    describes geometry of the tube cross-section

Material
    describes elastic/plastic behavior and limits

Stretch-bending process input
    describes the target geometry and machine command

ASCII:

Pipe section + Material + Process input
                  ?
       Stretch-bending validation
                  ?
          ?(s), ?(s), tension
                  ?
       shared Segment 9 integrator

No manufacturing state changes yet.

The expected process flow is:

StretchBendingProcessInput
        ?
StretchBendingEvaluator
        ??? calculate section properties
        ??? calculate bending strain
        ??? calculate inner/outer strain
        ??? calculate required tension
        ??? determine feasibility
        ?
CurvatureTorsionProfile
        ?
SpatialCurveIntegrator
7. Add a test input in AppController

Inside the anonymous names

=========================================================
Phase 10B — Define evaluation result and feasibility statuses
Goal

Create a result type that records:

whether the stretch-bending request is valid
calculated strains
required tension
bending moment
risk conditions
final feasibility status

No geometry generation or manufacturing execution yet.

Flow:

StretchBendingProcessInput
        ?
future evaluator
        ?
StretchBendingEvaluationResult
        ??? calculated values
        ??? warnings
        ??? status

        4. Meaning of the main fields

The result will eventually contain:

bendingStrain = ?D/2

innerWallStrain =
    axialStretchStrain - bendingStrain

outerWallStrain =
    axialStretchStrain + bendingStrain

Allowed strain range:

minimumRequiredAxialStrain =
    bendingStrain

maximumAllowedAxialStrain =
    allowableStrain - bendingStrain

Geometry is feasible when:

minimumRequiredAxialStrain
    <= maximumAllowedAxialStrain

ASCII:

invalid range:

min ------------------->
          <------ max


valid range:

min |===================| max
5. Status priority

The future evaluator should return only one primary status.

Recommended order:

1. Disabled
2. InvalidPipeSection
3. InvalidMaterial
4. InvalidGeometry
5. InvalidInput
6. GeometryNotFeasible
7. OuterWallStrainExceeded
8. InnerWallCompressionRisk
9. BelowYield
10. Valid

Why priority matters:

A process could simultaneously have:

inner compression
outer strain exceeded
below yield

The status gives the main failure reason, while the boolean 
fields preserve all detailed conditions.
Architectural boundary
StretchBendingProcessInput
        ?
future StretchBendingEvaluator
        ?
StretchBendingEvaluationResult
        ??? status
        ??? strains
        ??? tension
        ??? bending moment
        ??? safety flags

This result remains separate from:

SpatialCurveIntegrationResult
ManufacturingState
ManufacturingRenderData

The evaluation result answers:

“Is the process physically feasible?”

The spatial integration result answers:

“What geometry does ?(s), ?(s) produce?”

==================================================
Phase 10C
Implement stretch-bending section, strain, force, 
and moment evaluation

Goal

Create a process-independent evaluator that converts:

StretchBendingProcessInput

into:

StretchBendingEvaluationResult

This phase calculates:

section properties
yield values
bending strain
inner/outer wall strain
allowed axial-strain range
axial tension
elastic bending moment
basic feasibility flags

It does not generate geometry or modify manufacturing state.

==============================================
Phase 10D — Add stretch-bending feasibility test cases
Goal

Verify that StretchBendingEvaluator distinguishes
four important states:

1. Valid
2. InnerWallCompressionRisk
3. OuterWallStrainExceeded
4. BelowYield

The tests use the same pipe and material data, changing only:

target curvature ?
axial stretch strain ??

No geometry generation or rendering changes.
1. Add a small test-case type

In the anonymous namespace of AppController.cpp:

struct StretchEvaluationTestCase
{
    const char* name =
        "";

    double targetCurvature =
        0.0;

    double axialStretchStrain =
        0.0;

    StretchBendingEvaluationStatus expectedStatus =
        StretchBendingEvaluationStatus::NotEvaluated;
};

This is only test infrastructure, so keeping it local to
AppController.cpp is appropriate.

[STRETCH CASE] name=Valid
expected=Valid actual=Valid pass=1
kappa=0.002 bending=0.02 axial=0.03
inner=0.01 outer=0.05 feasible=1 aboveYield=1

[STRETCH CASE] name=InnerCompression
expected=InnerWallCompressionRisk
actual=InnerWallCompressionRisk pass=1
inner=-0.01 outer=0.03

[STRETCH CASE] name=OuterLimit
expected=OuterWallStrainExceeded
actual=OuterWallStrainExceeded pass=1
inner=0.05 outer=0.09

[STRETCH CASE] name=BelowYield
expected=BelowYield actual=BelowYield pass=1
inner=0 outer=0.002 aboveYield=0

[STRETCH CASE SUMMARY] passed=4/4 result=PASS
=======================================================

Phase 10E
Calculate recommended axial-strain range
and machine tension commands

This phase will derive:

minimum axial strain
maximum axial strain
recommended working strain
minimum tension
maximum tension
recommended tension

from the section, material, allowable strain,
and target curvature.

Phase 10E — Recommended axial-strain range and machine tension commands
Cel

Dla zadanej geometrii chcemy obliczyæ:

minimalne wymagane odkszta³cenie osiowe
maksymalne dozwolone odkszta³cenie osiowe
zalecane robocze odkszta³cenie osiowe

oraz odpowiadaj¹ce im si³y rozci¹gaj¹ce:

minimal tension
maximum tension
recommended tension

Podstawowa zale¿noœæ:

T = E · A · ??

Zakres odkszta³cenia osiowego:

??,min = ?b
??,max = ?allow - ?b

gdzie:

?b = ?D/2

ASCII:

compression risk                              outer strain risk
        ?                                             ?
        ?                                             ?

--------|======================?======================|--------
      ?min                recommended               ?max

Na pocz¹tku przyjmiemy zalecenie jako œrodek poprawnego zakresu:

?recommended = (?min + ?max) / 2

========================================================
Phase 10F — Build accepted stretch-bending curvature/torsion profile
Goal

Convert only an accepted stretch-bending evaluation into:

CurvatureTorsionProfile

The profile will then be ready for the shared Segment 9 integrator.

Flow:

StretchBendingProcessInput
        ?
StretchBendingEvaluator
        ?
StretchBendingEvaluationResult
        ? only when status == Valid
StretchBendingProfileBuilder
        ?
CurvatureTorsionProfile

No geometry generation yet.

1. Create StretchBendingProfileBuilder.h

Suggested path:

Core/Forming/StretchBendingProfileBuilder.h
#pragma once

#include "Core/Forming/StretchBendingProcessInput.h"
#include "Core/Forming/StretchBendingEvaluationResult.h"
#include "Core/Geometry/CurvatureTorsionProfile.h"

// =====================================================
// STRETCH-BENDING PROFILE BUILDER
//
// Converts an accepted stretch-bending input/evaluation
// into a process-independent curvature/torsion profile.
//
// The builder:
//
//     does not generate PipeNodes
//     does not modify manufacturing state
//     does not perform material evaluation
//     does not render anything
//
// The first implementation produces a constant profile:
//
//     kappa(s) = targetCurvature
//     tau(s)   = targetTorsion
// =====================================================

class StretchBendingProfileBuilder
{
public:
    static CurvatureTorsionProfile build(
        const StretchBendingProcessInput& input,
        const StretchBendingEvaluationResult& evaluation
    );
};