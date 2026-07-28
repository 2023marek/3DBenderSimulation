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

The output is correct. Phase 10F is successful.

The accepted case produced the expected constant profile:

s=0      ?=0.002 ?=0
s=200    ?=0.002 ?=0

That confirms:

? profile length matches 200 mm
? curvature is constant
? torsion is zero
? two-sample representation is correct

The rejection test is especially important:

evaluationStatus=GeometryNotFeasible
profileValid=0
samples=0

This proves the safety boundary works:

invalid stretch-bending evaluation
        ?
no ?/? profile
        ?
shared integrator cannot be called accidentally

====================================================
Phase 10G
Generate and store standalone stretch-bending geometry

Phase 10G — Generate and store standalone stretch-bending geometry
Goal

Use the accepted stretch-bending profile from Phase 10F with the shared Segment 9 integrator, then store the result separately for later debug rendering.

Flow:

valid StretchBendingProcessInput
        ?
StretchBendingEvaluator
        ?
valid StretchBendingEvaluationResult
        ?
StretchBendingProfileBuilder
        ?
CurvatureTorsionProfile
        ?
SpatialCurveIntegrator
        ?
SpatialCurveIntegrationResult

Still no manufacturing state changes.

1. Add stored result in AppController.h

Add in private::

SpatialCurveIntegrationResult
    debugStretchBendingIntegrationResult;

Optional public getter:

const SpatialCurveIntegrationResult&
getDebugStretchBendingIntegrationResult() const
{
    return debugStretchBendingIntegrationResult;
}
2. Add debug helper declaration

In AppController.h, private section:

void debugTestStretchBendingGeometry();

This method is not const because it stores the result.

3. Implement the geometry test

Add to AppController.cpp:

void AppController::debugTestStretchBendingGeometry()
{
    if (!DEBUG_TEST_STRETCH_BENDING_INPUT)
        return;

    // =====================================================
    // 1. BUILD KNOWN VALID STRETCH-BENDING INPUT
    // =====================================================

    StretchBendingProcessInput input =
        buildTestStretchBendingProcessInput();

    input.geometry.targetCurvature =
        0.002;

    input.geometry.targetTorsion =
        0.0;

    input.geometry.targetArcLength =
        200.0;

    input.axialStretchStrain =
        0.03;

    input.sampleStep =
        0.25;

    // =====================================================
    // 2. EVALUATE MATERIAL / PROCESS FEASIBILITY
    // =====================================================

    StretchBendingEvaluator evaluator;

    const StretchBendingEvaluationResult evaluation =
        evaluator.evaluate(
            input
        );

    if (!evaluation.valid)
    {
        debugStretchBendingIntegrationResult.clear();

        std::cout
            << "[STRETCH GEOMETRY]"
            << " evaluationStatus="
            << stretchBendingEvaluationStatusToString(
                evaluation.status
            )
            << " result=REJECTED"
            << std::endl;

        return;
    }

    // =====================================================
    // 3. BUILD ACCEPTED KAPPA / TAU PROFILE
    // =====================================================

    const CurvatureTorsionProfile profile =
        StretchBendingProfileBuilder::build(
            input,
            evaluation
        );

    if (!profile.valid)
    {
        debugStretchBendingIntegrationResult.clear();

        std::cout
            << "[STRETCH GEOMETRY]"
            << " profileValid=0"
            << " result=REJECTED"
            << std::endl;

        return;
    }

    // =====================================================
    // 4. DEFINE A SEPARATE DEBUG START FRAME
    //
    // Keep the standalone stretch result away from:
    //
    //     normal manufacturing pipe
    //     planar integrator test
    //     helix integrator test
    //
    // Start tangent:
    //     +X
    //
    // Bending normal:
    //     +Y
    //
    // Therefore the generated curve lies initially in
    // the XY plane when torsion is zero.
    // =====================================================

    Frame startFrame;

    startFrame.P =
        Vec3D{
            0.0,
            -220.0,
            0.0
        };

    startFrame.T =
        Vec3D{
            1.0,
            0.0,
            0.0
        };

    startFrame.N =
        Vec3D{
            0.0,
            1.0,
            0.0
        };

    startFrame.B =
        Vec3D{
            0.0,
            0.0,
            1.0
        };

    // =====================================================
    // 5. GENERATE TEMPORARY STRETCH-BENDING GEOMETRY
    // =====================================================

    SpatialCurveIntegrator integrator;

    debugStretchBendingIntegrationResult =
        integrator.integrate(
            startFrame,
            profile,
            input.sampleStep
        );

    const SpatialCurveIntegrationResult& result =
        debugStretchBendingIntegrationResult;

    // =====================================================
    // 6. BASIC DIAGNOSTICS
    // =====================================================

    std::cout
        << "[STRETCH GEOMETRY]"
        << " evaluationStatus="
        << stretchBendingEvaluationStatusToString(
            evaluation.status
        )
        << " profileValid="
        << profile.valid
        << " resultValid="
        << result.valid
        << " complete="
        << result.isComplete()
        << " nodes="
        << result.nodes.size()
        << " requestedLength="
        << result.requestedArcLength
        << " integratedLength="
        << result.integratedArcLength
        << " curvature="
        << input.geometry.targetCurvature
        << " torsion="
        << input.geometry.targetTorsion
        << std::endl;

    if (!result.nodes.empty())
    {
        const PipeNode& first =
            result.nodes.front();

        const PipeNode& last =
            result.nodes.back();

        std::cout
            << "[STRETCH GEOMETRY ENDPOINT]"
            << " firstP=("
            << first.pos.x << ", "
            << first.pos.y << ", "
            << first.pos.z << ")"
            << " lastP=("
            << last.pos.x << ", "
            << last.pos.y << ", "
            << last.pos.z << ")"
            << std::endl;
    }
}
4. Add includes if missing

In AppController.cpp:

#include "Core/Forming/StretchBendingEvaluator.h"
#include "Core/Forming/StretchBendingProfileBuilder.h"
#include "Core/Geometry/SpatialCurveIntegrator.h"

You likely already have them.

5. Call the test once

In the constructor:

debugTestStretchBendingEvaluation();
debugTestStretchBendingFeasibilityCases();
debugTestStretchBendingProfileBuilder();
debugTestStretchBendingGeometry();
6. Expected geometry

Input:

? = 0.002 /mm
? = 0
length = 200 mm

Radius:

R = 1 / ? = 500 mm

Total bend angle:

angle = ? × length
      = 0.002 × 200
      = 0.4 rad

Approximately:

22.918°

Expected local endpoint relative to the start frame:

x = R sin(angle)
y = R(1 - cos(angle))
z = 0

Approximately:

x ? 194.709 mm
y ? 39.4709 mm
z = 0

Since the start point is:

(0, -220, 0)

expected world endpoint is approximately:

(194.709, -180.529, 0)
7. Add analytical endpoint check

Inside the debug method, after successful integration:

if (result.isComplete()
    && !result.nodes.empty())
{
    const double curvature =
        input.geometry.targetCurvature;

    const double length =
        input.geometry.targetArcLength;

    const double angle =
        curvature * length;

    Vec3D expectedEnd;

    if (curvature > 1e-12)
    {
        const double radius =
            1.0 / curvature;

        expectedEnd =
            startFrame.P
            + Vec3D{
                radius * std::sin(angle),
                radius
                    * (
                        1.0
                        - std::cos(angle)
                    ),
                0.0
            };
    }
    else
    {
        expectedEnd =
            startFrame.P
            + startFrame.T * length;
    }

    const Vec3D error =
        result.nodes.back().pos
        - expectedEnd;

    std::cout
        << "[STRETCH GEOMETRY ACCURACY]"
        << " expectedEnd=("
        << expectedEnd.x << ", "
        << expectedEnd.y << ", "
        << expectedEnd.z << ")"
        << " positionError="
        << error.length()
        << std::endl;
}

With sampleStep=0.25, the error should be small.

8. Optional standalone rendering

You already have:

spatialDebugRenderer
drawSpatialIntegratorResult(...)

Extend drawSpatialIntegratorDebugPreviews():

drawSpatialIntegratorResult(
    app->getDebugStretchBendingIntegrationResult(),
    glm::vec3(
        0.9f,
        0.2f,
        0.8f
    ),
    SPATIAL_INTEGRATOR_MESH_RADIUS
);

Suggested meaning:

orange      = general planar integrator test
blue        = general helix integrator test
pink/purple = accepted stretch-bending geometry

Place it after the existing two previews:

drawSpatialIntegratorResult(
    planar,
    SPATIAL_PLANAR_PREVIEW_COLOR,
    SPATIAL_INTEGRATOR_MESH_RADIUS
);

drawSpatialIntegratorResult(
    helix,
    SPATIAL_HELIX_PREVIEW_COLOR,
    SPATIAL_INTEGRATOR_MESH_RADIUS
);

drawSpatialIntegratorResult(
    app->getDebugStretchBendingIntegrationResult(),
    SPATIAL_STRETCH_PREVIEW_COLOR,
    SPATIAL_INTEGRATOR_MESH_RADIUS
);

Add color constant:

constexpr glm::vec3 SPATIAL_STRETCH_PREVIEW_COLOR =
    glm::vec3(
        0.9f,
        0.2f,
        0.8f
    );

This remains under the same spatial-preview visibility toggle.

Expected Phase 10G result

Console:

[STRETCH GEOMETRY]
evaluationStatus=Valid
profileValid=1
resultValid=1
complete=1
nodes=801
requestedLength=200
integratedLength=200
curvature=0.002
torsion=0

Why 801 nodes:

200 / 0.25 = 800 steps
+ initial node = 801

Expected endpoint:

approximately:
(194.709, -180.529, 0)

Visual result:

a smooth, shallow purple planar arc
separate from normal manufacturing geometry

Acceptance:

? valid evaluation produces geometry
? profile and integration complete
? analytical endpoint error small
? standalone curve visible in LINE mode
? standalone tube round in MESH mode
? invalid stretch input still produces no geometry
? rotary-draw path unchanged

============================================================
Phase 10H — Add first springback approximation and loaded-curvature command
Goal

Until now the profile uses:

targetCurvature

directly.

Real material elastically recovers after unloading:
loaded curvature
        ? unload
final curvature < loaded curvature

Therefore the machine must overbend:

loaded curvature > target final curvature

ASCII:

machine-loaded shape:

       )))
     )))
   )))

after unloading:

        )
      )
    )

This first model will be deliberately simple and isolated. It will not yet modify manufacturing playback.

1. First springback model

Use a curvature-recovery ratio:

springbackRatio ? [0,1)

Definition:

finalCurvature =
    loadedCurvature × (1 - springbackRatio)

To achieve a target final curvature:

loadedCurvature =
    targetCurvature
    / (1 - springbackRatio)

Example:

target ? = 0.002
springback ratio = 0.10

loaded ? =
    0.002 / 0.90
    = 0.00222222

After unloading:

0.00222222 × 0.90
    = 0.002

This is a control approximation, not yet a full elastic-plastic unloading solution.

2. Extend StretchBendingProcessInput

Add:

// Fraction of loaded curvature expected to recover
// during unloading.
//
// Example:
//
//     0.10 = 10% curvature recovery
//
// Valid range:
//
//     0 <= springbackRatio < 1
double springbackRatio =
    0.0;

// Enables machine overbend compensation.
bool compensateSpringback =
    true;

Update isValid():

if (!std::isfinite(springbackRatio))
    return false;

if (springbackRatio < 0.0
    || springbackRatio >= 1.0)
{
    return false;
}

Do not reject:

..................
11. Store final-curvature preview separately later

In this phase, debugStretchBendingIntegrationResult should represent:

loaded geometry

The existing purple curve will therefore become slightly more curved.

We will later create a separate unloaded/final result.

Current flow:

target final ?
    ? compensation
loaded ? profile
    ? integrator
loaded geometry preview
12. Add diagnostics

In:

debugTestStretchBendingEvaluation()

add:

std::cout
    << "[STRETCH SPRINGBACK]"
    << " targetFinalKappa="
    << result.finalTargetCurvature
    << " ratio="
    << result.springbackRatio
    << " compensationApplied="
    << result.springbackCompensationApplied
    << " loadedKappa="
    << result.loadedCurvatureCommand
    << " predictedFinalKappa="
    << result.predictedFinalCurvature
    << " finalError="
    << result.finalCurvatureError
    << " loadedBendingStrain="
    << result.loadedBendingStrain
    << " predictionValid="
    << result.springbackPredictionValid
    << std::endl;
13. Expected values for the valid test

For:

target final ? = 0.002
springback ratio = 0.10

Expected:

loaded ? =
    0.002 / 0.9
    = 0.00222222

Predicted final:

0.00222222 × 0.9
    = 0.002

Error:

approximately 0

Loaded bending strain:

?b,load =
    0.00222222 × 20 / 2
    = 0.0222222

The feasible strain interval changes from:

old:
0.0200 to 0.0600

to:

new:
0.0222222 to 0.0577778

The recommended midpoint remains:

0.04

because the simple symmetric limits still center around:

allowableStrain / 2
14. Important effect on existing test cases

Your previous test cases were designed without springback compensation.

If the global test input now has:

springbackRatio = 0.10;

the loaded strain becomes larger, so expected values change slightly.

To keep Phase 10D tests focused only on status logic, disable springback inside those tests:

input.springbackRatio =
    0.0;

input.compensateSpringback =
    false;

Add those lines inside the loop before evaluation.

This preserves the previous expected outputs exactly.

Then create a separate Phase 10H springback test.

15. Add dedicated springback test

In AppController.h:

void debugTestStretchBendingSpringback() const;

Implementation:

void AppController::debugTestStretchBendingSpringback() const
{
    if (!DEBUG_TEST_STRETCH_BENDING_INPUT)
        return;

    StretchBendingProcessInput input =
        buildTestStretchBendingProcessInput();

    input.geometry.targetCurvature =
        0.002;

    input.axialStretchStrain =
        0.04;

    input.springbackRatio =
        0.10;

    input.compensateSpringback =
        true;

    StretchBendingEvaluator evaluator;

    const StretchBendingEvaluationResult result =
        evaluator.evaluate(
            input
        );

    const double tolerance =
        1e-12;

    const bool finalCurvatureAccepted =
        std::abs(
            result.predictedFinalCurvature
            - input.geometry.targetCurvature
        )
        <= tolerance;

    std::cout
        << "[STRETCH SPRINGBACK TEST]"
        << " status="
        << stretchBendingEvaluationStatusToString(
            result.status
        )
        << " targetKappa="
        << input.geometry.targetCurvature
        << " loadedKappa="
        << result.loadedCurvatureCommand
        << " predictedFinalKappa="
        << result.predictedFinalCurvature
        << " ratio="
        << result.springbackRatio
        << " error="
        << result.finalCurvatureError
        << " accepted="
        << finalCurvatureAccepted
        << std::endl;
}

Call:

debugTestStretchBendingSpringback();

after the feasibility cases.

16. Expected standalone geometry change

Before compensation:

? = 0.002
R = 500 mm
angle over 200 mm = 0.4 rad

After compensation:

?load = 0.00222222
Rload = 450 mm
loaded angle over 200 mm ? 0.444444 rad

So the purple loaded preview should bend slightly more strongly.

ASCII:

target final:
??????????????)

loaded machine preview:
?????????????))

Do not yet visually compare loaded and unloaded curves in the same scene. That comes next.

Phase 10H acceptance
? build complete
? existing feasibility tests still PASS with springback disabled
? springback test reports accepted=1
? loaded curvature exceeds target curvature
? predicted final curvature matches target
? loaded strain uses loaded curvature
? loaded moment uses loaded curvature
? standalone purple geometry becomes slightly more curved
? rotary-draw path unchanged

Next phase: