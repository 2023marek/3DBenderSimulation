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

=====================================================
Phase 10I
Generate unloaded final geometry and compare 
loaded versus final shape

Phase 10I — Generate unloaded final geometry and compare with loaded shape
Goal

Create two separate standalone results:

Loaded geometry
    uses loadedCurvatureCommand

Final unloaded geometry
    uses predictedFinalCurvature

Then render both together.

ASCII:

loaded machine shape:
?????????????))

after unloading:
??????????????)

This phase remains outside real manufacturing playback.

1. Rename the existing stored result

Your current:

SpatialCurveIntegrationResult
    debugStretchBendingIntegrationResult;

represents the loaded machine geometry.

Rename it to:

SpatialCurveIntegrationResult
    debugStretchLoadedIntegrationResult;

Add a second result:

SpatialCurveIntegrationResult
    debugStretchFinalIntegrationResult;

Public getters:

const SpatialCurveIntegrationResult&
getDebugStretchLoadedIntegrationResult() const
{
    return debugStretchLoadedIntegrationResult;
}

const SpatialCurveIntegrationResult&
getDebugStretchFinalIntegrationResult() const
{
    return debugStretchFinalIntegrationResult;
}

Remove or update the old getter:

getDebugStretchBendingIntegrationResult()

to prevent ambiguity.

2. Keep the loaded profile builder unchanged

StretchBendingProfileBuilder should continue using:

evaluation.loadedCurvatureCommand

That builder represents the machine-loaded state.

Do not change it back to target curvature.

3. Add a final-profile builder

Create:

Core/Forming/StretchBendingFinalProfileBuild
.......................................

9. Render both results

In:

drawSpatialIntegratorDebugPreviews()

replace the old single stretch preview with:

drawSpatialIntegratorResult(
    app->getDebugStretchLoadedIntegrationResult(),
    SPATIAL_STRETCH_LOADED_COLOR,
    SPATIAL_INTEGRATOR_MESH_RADIUS
);

drawSpatialIntegratorResult(
    app->getDebugStretchFinalIntegrationResult(),
    SPATIAL_STRETCH_FINAL_COLOR,
    SPATIAL_INTEGRATOR_MESH_RADIUS
);

Render final after loaded:

loaded first
final second

so the green final curve remains visible where 
they nearly overlap.

10. Expected visual result
LINE mode
purple:
    slightly stronger bend

green:
    slightly more open bend

ASCII:

loaded purple:
????????????))

final green:
?????????????)

Both begin at exactly the same point and tangent.

MESH mode

You should see two thin round tubes:

purple tube = loaded
green tube  = unloaded final

They overlap near the start and gradually
separate toward the endpoint.

11. Expected diagnostics

Approximately:

[STRETCH SHAPE COMPARISON]
loadedValid=1
loadedComplete=1
finalValid=1
finalComplete=1
loadedKappa=0.00222222
finalKappa=0.002
targetKappa=0.002

Endpoints approximately:

loaded end:
(193.48, -176.282, 0)

final end:
(194.709, -180.529, 0)

Endpoint recovery length should be approximately:

sqrt(
    (194.709 - 193.480)^2
    + (-180.529 + 176.282)^2
)

? 4.42 mm

That earlier 4.42229 mm “error” now becomes a 
meaningful physical quantity:

springback endpoint displacement
Important interpretation

The current springback model assumes:

arc length unchanged
torsion unchanged
curvature uniformly reduced

This is a first approximation.

Later models may change:

arc length through elastic strain recovery
torsion during unloading
nonuniform curvature
end constraints

Do not add those yet.

Phase 10I acceptance
? build complete
? loaded geometry remains valid
? final unloaded geometry is generated
? final curvature matches target curvature
? loaded and final curves share start frame
? curves separate gradually
? endpoint recovery is approximately 4.42 mm
? LINE rendering clearly shows both shapes
? MESH rendering shows two round tubes
? rotary-draw path unchanged

Next phase:

Phase 10J
Define stretch-bending manufacturing state 

and fixed active-zone data


==
Why?

You now have three completely independent debug tests:

Segment 9

debugTestSpatialCurveIntegrator()
        ?
        ??? profile = constant curvature
        ??? stores:
            debugPlanarIntegrationResult
debugTestSpatialHelixIntegrator()
        ?
        ??? profile = constant ? + ?
        ??? stores:
            debugHelixIntegrationResult
Segment 10

debugTestStretchBendingGeometry()
        ?
        ??? loadedProfile
        ??? finalProfile
        ??? stores:
            debugStretchLoadedIntegrationResult
            debugStretchFinalIntegrationResult

They are separate demonstrations of the same shared geometry engine.

The correct place

The new code belongs inside:

void AppController::debugTestStretchBendingGeometry()

where you currently have something like:

StretchBendingEvaluator evaluator;

StretchBendingEvaluationResult evaluation =
    evaluator.evaluate(input);

CurvatureTorsionProfile profile =
    StretchBendingProfileBuilder::build(
        input,
        evaluation
    );

Frame startFrame;
...

Currently you probably have:

SpatialCurveIntegrator integrator;

debugStretchLoadedIntegrationResult =
    integrator.integrate(
        startFrame,
        profile,
        input.sampleStep
    );

Replace that section with:

SpatialCurveIntegrator integrator;

debugStretchLoadedIntegrationResult =
    integrator.integrate(
        startFrame,
        loadedProfile,
        input.sampleStep
    );

debugStretchFinalIntegrationResult =
    integrator.integrate(
        startFrame,
        finalProfile,
        input.sampleStep
    );


Architecturally

Think of your project like this:

                     SpatialCurveIntegrator
                              ?
                              ?
         ???????????????????????????????????????????
         ?                    ?                    ?
         ?                    ?                    ?
 Planar test            Helix test          Stretch test
 (Segment 9)           (Segment 9)         (Segment 10)

debugPlanar...     debugHelix...      debugStretchLoaded...
                                      debugStretchFinal...

Every test owns its own integration result.

None of them should overwrite another.

======================================================



Phase 10J — Stretch-bending manufacturing state and fixed active zone
Goal

Introduce a manufacturing-state model for stretch bending.

This phase does not animate anything yet.

It defines:

what the machine is doing
where forming occurs
which part of the pipe is active
which part is already formed
which part remains undeformed

The geometry generated in Phase 10I remains independent and unchanged.

Phase 10I
loaded/final reference geometry

Phase 10J
manufacturing-state description
1. Manufacturing model

For the first stretch-bending prototype, use one fixed active zone.

pipe start                                         pipe end

|--------------------|===============|--------------------|
     formed region       active zone       undeformed region

0                 activeStart       activeEnd             L

The active zone is defined in pipe arc-length coordinates.

activeStartS
activeEndS

For Phase 10J, these values remain fixed during the process.

Later phases can move or resize the active zone.

2. Add process-stage enum

Create:

Core/Forming/StretchBendingManufacturingStage.h
#pragma once

// =====================================================
// STRETCH-BENDING MANUFACTURING STAGE
//
// Describes the high-level state of the stretch-bending
// manufacturing process.
//
// This is intentionally independent from the existing
// rotary-draw manufacturing state.
//
// Phase 10J only defines the states.
// Later phases will use them during playback.
// =====================================================

enum class StretchBendingManufacturingStage
{
    // -------------------------------------------------
    // No valid manufacturing process has been prepared.
    // -------------------------------------------------

    Invalid,

    // -------------------------------------------------
    // Pipe and machine are ready, but no forming load
    // has yet been applied.
    // -------------------------------------------------

    Ready,

    // -------------------------------------------------
    // Axial tension is being established.
    //
    // The pipe may be stretched, but bending has not
    // yet reached the commanded loaded curvature.
    // -------------------------------------------------

    ApplyingTension,

    // -------------------------------------------------
    // Bending is actively occurring inside the fixed
    // active zone.
    // -------------------------------------------------

    Forming,

    // -------------------------------------------------
    // Loaded machine geometry has been reached.
    //
    // The forming load is still considered present.
    // -------------------------------------------------

    LoadedHold,

    // -------------------------------------------------
    // Bending load is being removed and elastic
    // springback is occurring.
    // -------------------------------------------------

    Unloading,

    // -------------------------------------------------
    // Final unloaded geometry has been reached.
    // -------------------------------------------------

    Complete
};
3. Add stage-to-string helper

Create:

Core/Forming/StretchBendingManufacturingStage.cpp

or place this helper next to the enum if your project commonly uses header-only helpers.

#include "Core/Forming/StretchBendingManufacturingStage.h"

const char*
stretchBendingManufacturingStageToString(
    StretchBendingManufacturingStage stage
)
{
    switch (stage)
    {
        case StretchBendingManufacturingStage::Invalid:
            return "Invalid";

        case StretchBendingManufacturingStage::Ready:
            return "Ready";

        case StretchBendingManufacturingStage::ApplyingTension:
            return "ApplyingTension";

        case StretchBendingManufacturingStage::Forming:
            return "Forming";

        case StretchBendingManufacturingStage::LoadedHold:
            return "LoadedHold";

        case StretchBendingManufacturingStage::Unloading:
            return "Unloading";

        case StretchBendingManufacturingStage::Complete:
            return "Complete";
    }

    return "Unknown";
}
...........................................
Add fixed active-zone data

Create:

Core/Forming/StretchBendingActiveZone.h
#pragma once

#include <cmath>

// =====================================================
// STRETCH-BENDING ACTIVE ZONE
//
// Defines the part of the pipe where bending deformation
// is currently allowed to occur.
//
// Coordinates use pipe arc length:
//
//     s = 0              pipe start
//     s = totalLength    pipe end
//
// Phase 10J uses a fixed zone.
//
// Later phases may introduce:
//
//     moving active zones
//     growing active zones
//     machine-relative active zones
// =====================================================

struct StretchBendingActiveZone
{
    // -------------------------------------------------
    // Arc-length coordinate where the active zone begins.
    // -------------------------------------------------

    double startS = 0.0;

    // -------------------------------------------------
    // Arc-length coordinate where the active zone ends.
    // -------------------------------------------------

    double endS = 0.0;

    // -------------------------------------------------
    // Returns the zone length.
    // -------------------------------------------------

    double length() const
    {
        return endS - startS;
    }

    // -------------------------------------------------
    // Basic mathematical validity.
    //
    // This function does not know the pipe length.
    // Full validation against the pipe belongs in
    // isValidForLength().
    // -------------------------------------------------

    bool isValid() const
    {
        return
            std::isfinite(startS)
            && std::isfinite(endS)
            && startS >= 0.0
            && endS > startS;
    }

    // -------------------------------------------------
    // Checks whether this zone fits inside a pipe with
    // the supplied total arc length.
    // -------------------------------------------------

    bool isValidForLength(
        double totalLength
    ) const
    {
        return
            isValid()
            && std::isfinite(totalLength)
            && totalLength > 0.0
            && endS <= totalLength;
    }

    // -------------------------------------------------
    // Returns true when the supplied pipe coordinate lies
    // inside the active zone.
    //
    // Inclusive boundaries are useful for rendering and
    // sample classification.
    // -------------------------------------------------

    bool contains(
        double s
    ) const
    {
        return
            isValid()
            && std::isfinite(s)
            && s >= startS
            && s <= endS;
    }
};
5. Add manufacturing-state structure

Create:

Core/Forming/StretchBendingManufacturingState.h
#pragma once

#include <cmath>

#include "Core/Forming/StretchBendingManufacturingStage.h"
#include "Core/Forming/StretchBendingActiveZone.h"

// =====================================================
// STRETCH-BENDING MANUFACTURING STATE
//
// Represents one snapshot of the stretch-bending process.
//
// It does not own geometry.
//
// It describes:
//
//     process stage
//     overall progress
//     active forming zone
//     applied tension fraction
//     applied bending fraction
//     unloading fraction
//
// Geometry generation and rendering remain separate.
// =====================================================

struct StretchBendingManufacturingState
{
    // -------------------------------------------------
    // Current high-level manufacturing stage.
    // -------------------------------------------------

    StretchBendingManufacturingStage stage =
        StretchBendingManufacturingStage::Invalid;

    // -------------------------------------------------
    // Overall normalized playback progress.
    //
    // Expected range:
    //
    //     0.0 = process beginning
    //     1.0 = process complete
    // -------------------------------------------------

    double processProgress = 0.0;

    // -------------------------------------------------
    // Fixed pipe region where bending deformation occurs.
    // -------------------------------------------------

    StretchBendingActiveZone activeZone;

    // -------------------------------------------------
    // Fraction of commanded axial tension currently
    // applied.
    //
    //     0.0 = no tension
    //     1.0 = full commanded tension
    // -------------------------------------------------

    double tensionFraction = 0.0;

    // -------------------------------------------------
    // Fraction of loaded bending command currently
    // applied.
    //
    //     0.0 = straight/unbent state
    //     1.0 = full loaded curvature
    // -------------------------------------------------

    double bendingFraction = 0.0;

    // -------------------------------------------------
    // Fraction of unloading completed.
    //
    //     0.0 = loaded shape
    //     1.0 = final unloaded shape
    //
    // This remains zero before the Unloading stage.
    // -------------------------------------------------

    double unloadingFraction = 0.0;

    // -------------------------------------------------
    // Returns true if all scalar values are finite and
    // inside their normalized ranges.
    // -------------------------------------------------

    bool isValidForLength(
        double totalLength
    ) const
    {
        if (stage
            == StretchBendingManufacturingStage::Invalid)
        {
            return false;
        }

        if (!std::isfinite(processProgress)
            || !std::isfinite(tensionFraction)
            || !std::isfinite(bendingFraction)
            || !std::isfinite(unloadingFraction))
        {
            return false;
        }

        if (processProgress < 0.0
            || processProgress > 1.0)
        {
            return false;
        }

        if (tensionFraction < 0.0
            || tensionFraction > 1.0)
        {
            return false;
        }

        if (bendingFraction < 0.0
            || bendingFraction > 1.0)
        {
            return false;
        }

        if (unloadingFraction < 0.0
            || unloadingFraction > 1.0)
        {
            return false;
        }

        return activeZone.isValidForLength(
            totalLength
        );
    }
};
6. State meaning

The three fractions have different meanings.

tensionFraction
    controls axial stretching load

bendingFraction
    controls progress toward loaded curvature

unloadingFraction
    interpolates loaded shape toward final shape

Conceptually:

Ready
    tension = 0
    bending = 0
    unloading = 0

ApplyingTension
    tension = 0 ? 1
    bending = 0
    unloading = 0

Forming
    tension = 1
    bending = 0 ? 1
    unloading = 0

LoadedHold
    tension = 1
    bending = 1
    unloading = 0

Unloading
    tension may reduce
    bending = 1
    unloading = 0 ? 1

Complete
    final unloaded geometry
    unloading = 1
7. Add a fixed-state builder

Do not manually construct manufacturing states throughout the application.

Create one builder responsible for initial Phase 10J state creation.

Create:

Core/Forming/StretchBendingManufacturingStateBuilder.h
#pragma once

#include "Core/Forming/StretchBendingProcessInput.h"
#include "Core/Forming/StretchBendingEvaluationResult.h"
#include "Core/Forming/StretchBendingManufacturingState.h"

// =====================================================
// STRETCH-BENDING MANUFACTURING-STATE BUILDER
//
// Creates the initial fixed-zone manufacturing state.
//
// Phase 10J does not calculate time-dependent playback.
// It only prepares a valid Ready state.
// =====================================================

class StretchBendingManufacturingStateBuilder
{
public:
    static StretchBendingManufacturingState buildReadyState(
        const StretchBendingProcessInput& input,
        const StretchBendingEvaluationResult& evaluation,
        const StretchBendingActiveZone& activeZone
    );
};

Create:

Core/Forming/StretchBendingManufacturingStateBuilder.cpp
#include "Core/Forming/StretchBendingManufacturingStateBuilder.h"

StretchBendingManufacturingState
StretchBendingManufacturingStateBuilder::buildReadyState(
    const StretchBendingProcessInput& input,
    const StretchBendingEvaluationResult& evaluation,
    const StretchBendingActiveZone& activeZone
)
{
    StretchBendingManufacturingState state;

    // -------------------------------------------------
    // A manufacturing state can only be prepared from
    // accepted stretch-bending input.
    // -------------------------------------------------

    if (!input.isValid())
        return state;

    if (!evaluation.valid)
        return state;

    if (evaluation.status
        != StretchBendingEvaluationStatus::Valid)
    {
        return state;
    }

    const double totalLength =
        input.geometry.targetArcLength;

    if (!activeZone.isValidForLength(
            totalLength
        ))
    {
        return state;
    }

    // -------------------------------------------------
    // Initial ready state.
    //
    // No tension or bending load has been applied yet.
    // -------------------------------------------------

    state.stage =
        StretchBendingManufacturingStage::Ready;

    state.processProgress =
        0.0;

    state.activeZone =
        activeZone;

    state.tensionFraction =
        0.0;

    state.bendingFraction =
        0.0;

    state.unloadingFraction =
        0.0;

    return state;
}
8. Choose the Phase 10J fixed zone

For the existing test length:

total pipe length = 200 mm

Use:

active zone = 40 mm to 160 mm

ASCII:

s=0          s=40                         s=160       s=200
|-------------|=============================|-----------|
  inactive             active zone             inactive

This gives:

active-zone length = 120 mm

In your stretch debug test:

StretchBendingActiveZone activeZone;

activeZone.startS =
    40.0;

activeZone.endS =
    160.0;

Avoid making the whole pipe active for this test.

A smaller region makes later visual classification easier.

9. Store the debug manufacturing state

In the class that currently stores:

debugStretchLoadedIntegrationResult;
debugStretchFinalIntegrationResult;

add:

StretchBendingManufacturingState
    debugStretchManufacturingState;

Add getter:

const StretchBendingManufacturingState&
getDebugStretchManufacturingState() const
{
    return debugStretchManufacturingState;
}

Initialize it using the builder:

debugStretchManufacturingState =
    StretchBendingManufacturingStateBuilder::buildReadyState(
        input,
        evaluation,
        activeZone
    );

Place this after evaluation and active-zone creation, but before future manufacturing rendering.

Suggested order:

evaluate process
    ?
build loaded profile
    ?
build final profile
    ?
integrate reference geometry
    ?
build fixed active zone
    ?
build manufacturing state
    ?
print diagnostics
10. Add active-zone diagnostics
const StretchBendingManufacturingState& manufacturingState =
    debugStretchManufacturingState;

std::cout
    << "[STRETCH MANUFACTURING STATE]"
    << " stage="
    << stretchBendingManufacturingStageToString(
        manufacturingState.stage
    )
    << " progress="
    << manufacturingState.processProgress
    << " tensionFraction="
    << manufacturingState.tensionFraction
    << " bendingFraction="
    << manufacturingState.bendingFraction
    << " unloadingFraction="
    << manufacturingState.unloadingFraction
    << " valid="
    << manufacturingState.isValidForLength(
        input.geometry.targetArcLength
    )
    << std::endl;

Add zone diagnostics:

std::cout
    << "[STRETCH ACTIVE ZONE]"
    << " startS="
    << manufacturingState.activeZone.startS
    << " endS="
    << manufacturingState.activeZone.endS
    << " length="
    << manufacturingState.activeZone.length()
    << " totalLength="
    << input.geometry.targetArcLength
    << " valid="
    << manufacturingState.activeZone.isValidForLength(
        input.geometry.targetArcLength
    )
    << std::endl;
11. Add classification diagnostics

Check representative coordinates:

const double beforeZoneS =
    20.0;

const double insideZoneS =
    100.0;

const double afterZoneS =
    180.0;

std::cout
    << "[STRETCH ACTIVE ZONE CLASSIFICATION]"
    << " beforeS="
    << beforeZoneS
    << " beforeActive="
    << manufacturingState.activeZone.contains(
        beforeZoneS
    )
    << " insideS="
    << insideZoneS
    << " insideActive="
    << manufacturingState.activeZone.contains(
        insideZoneS
    )
    << " afterS="
    << afterZoneS
    << " afterActive="
    << manufacturingState.activeZone.contains(
        afterZoneS
    )
    << std::endl;

Expected:

beforeActive=0
insideActive=1
afterActive=0
12. Add invalid-zone test

Test one invalid zone separately.

StretchBendingActiveZone invalidZone;

invalidZone.startS =
    170.0;

invalidZone.endS =
    230.0;

Build:

const StretchBendingManufacturingState invalidState =
    StretchBendingManufacturingStateBuilder::buildReadyState(
        input,
        evaluation,
        invalidZone
    );

Diagnostic:

std::cout
    << "[STRETCH ACTIVE ZONE REJECTION]"
    << " zoneStart="
    << invalidZone.startS
    << " zoneEnd="
    << invalidZone.endS
    << " pipeLength="
    << input.geometry.targetArcLength
    << " stateStage="
    << stretchBendingManufacturingStageToString(
        invalidState.stage
    )
    << " stateValid="
    << invalidState.isValidForLength(
        input.geometry.targetArcLength
    )
    << std::endl;

Expected:

stateStage=Invalid
stateValid=0
13. Expected console output

Approximately:

[STRETCH MANUFACTURING STATE]
stage=Ready
progress=0
tensionFraction=0
bendingFraction=0
unloadingFraction=0
valid=1
[STRETCH ACTIVE ZONE]
startS=40
endS=160
length=120
totalLength=200
valid=1
[STRETCH ACTIVE ZONE CLASSIFICATION]
beforeS=20
beforeActive=0
insideS=100
insideActive=1
afterS=180
afterActive=0
[STRETCH ACTIVE ZONE REJECTION]
zoneStart=170
zoneEnd=230
pipeLength=200
stateStage=Invalid
stateValid=0
14. Do not render manufacturing geometry yet

Phase 10J should not deform or recolor the pipe.

The current display should remain:

purple = complete loaded reference shape
green  = complete final unloaded reference shape

The new state is data only.

manufacturing state
        |
        X no rendering yet
        |
future Phase 10K renderer

This separation prevents rendering decisions from
contaminating the process model.

15. Ownership

Recommended ownership:

AppController
    |
    +-- debugStretchLoadedIntegrationResult
    |
    +-- debugStretchFinalIntegrationResult
    |
    +-- debugStretchManufacturingState

Core types remain independent:

StretchBendingProcessInput
StretchBendingEvaluationResult
StretchBendingActiveZone
StretchBendingManufacturingState

The renderer may read state later, but it should not modify it.

Phase 10J acceptance
? manufacturing-stage enum exists
? fixed active-zone type exists
? active-zone validation works
? manufacturing state has normalized fractions
? Ready state builds from valid input
? invalid process input produces Invalid state
? out-of-range active zone is rejected
? active-zone classification works
? loaded/final geometry remains unchanged
? no rotary-draw code is modified
? no manufacturing animation is introduced yet

After the diagnostics pass, the next phase is:

Phase 10K

Generate a manufacturing preview centerline with:

straight region
active transition region
formed region

This will be the first phase where the fixed active-zone 
state affects visible geometry.


=======================================================
Phase 10K — Stretch-Bending Manufacturing State Progression

Phase 10J created a valid initial state:

Ready
progress = 0
tensionFraction = 0
bendingFraction = 0
unloadingFraction = 0

Phase 10K advances that state through the manufacturing sequence:

Ready
  ?
ApplyingTension
  ?
Forming
  ?
LoadedHold
  ?
Unloading
  ?
Complete

This phase changes only process-state data. It does not yet modify geometry or rendering.

10K.1 — Timeline model

We give every process stage a duration:

Applying tension   1.0 s
Forming            2.0 s
Loaded hold        0.5 s
Unloading          1.0 s
                  -----
Total              4.5 s

ASCII timeline:

time
0        1.0             3.0      3.5             4.5
?---------?---------------?--------?----------------?
 TENSION       FORMING       HOLD       UNLOADING

The fractions mean:

tensionFraction
    0.0 = no axial tension
    1.0 = full commanded tension

bendingFraction
    0.0 = no loaded bending command
    1.0 = full loaded bending command

unloadingFraction
    0.0 = unloading has not started
    1.0 = unloading complete

    10K.7 — Expected console output








10K.7 — Expected console output

Approximately:

[STRETCH STATE TRANSITION] stage=Ready time=0 progress=0

[STRETCH STATE TRANSITION]
stage=ApplyingTension
time=0.25
progress=0.0555556
tensionFraction=0.25
bendingFraction=0
unloadingFraction=0

[STRETCH STATE TRANSITION]
stage=Forming
time=1
progress=0.222222
tensionFraction=1
bendingFraction=0
unloadingFraction=0

[STRETCH STATE TRANSITION]
stage=LoadedHold
time=3
progress=0.666667
tensionFraction=1
bendingFraction=1
unloadingFraction=0

[STRETCH STATE TRANSITION]
stage=Unloading
time=3.5
progress=0.777778
tensionFraction=1
bendingFraction=1
unloadingFraction=0

[STRETCH STATE TRANSITION]
stage=Complete
time=4.5
progress=1
tensionFraction=0
bendingFraction=1
unloadingFraction=1

[STRETCH STATE PROGRESSION SUMMARY]
stage=Complete
elapsedTime=4.5
totalDuration=4.5
progress=1
tensionFraction=0
bendingFraction=1
unloadingFraction=1
accepted=1

At exactly the start of unloading, unloadingFraction=0 is correct. The process has entered the unloading stage, but no unloading time has yet elapsed.

Phase 10K flow
Ready state from Phase 10J
            ?
            ?
 StateAdvancer.advance(dt)
            ?
            ??? elapsed time
            ??? current stage
            ??? tension fraction
            ??? bending fraction
            ??? unloading fraction
            ?
            ?
 Updated manufacturing state

No geometry connection yet:

manufacturing state ??X??? rendered nodes

That separation is intentional.

The next phase remains logically:

Phase 10L
Manufacturing-state-driven loaded/final
geometry interpolation

But Phase 10K should first compile and produce accepted=1.


=======================================================
============================================================

Phase 10L
This is an excellent checkpoint. Looking at the 
complete output, I would approve moving to Phase 10L.
More importantly, I don't see any architectural 
issues that should be corrected before proceeding.


Let's review it as if this were a design review.

1. Geometry integration ?
loadedProfileValid=1
resultValid=1
complete=1
nodes=801
integratedLength=200

This tells us:

Profile
      ?
Integrator
      ?
SpatialCurveIntegrationResult

works exactly as intended.

The integrated length equals the requested length:

L
requested
	?

=L
integrated
	?

=200

which is one of the most important checks.

2. Geometry accuracy ?

Loaded:

positionError=5.37e-06

Final:

positionError=4.02e-06

These are essentially numerical round-off errors.

For engineering software these are effectively zero.

So our numerical integrator is behaving very well.

3. Manufacturing state ?
stage=Ready
...
valid=1

Builder works.

4. Active zone ?
40 ... 160

Classification:

20 -> outside

100 -> inside

180 -> outside

Exactly correct.

5. Invalid zone ?
170 ... 230

Correctly rejected.

That proves the validation path also works.

6. State machine ?

This is probably the nicest part.

Ready
?

ApplyingTension

?

Forming

?

LoadedHold

?

Unloading

?

Complete

Every transition occurred exactly once.

No skipped stages.

No repeated stages.

No impossible transitions.

7. Final summary ?
accepted=1

Exactly what we wanted.

8. Springback ?
target=0.002

loaded=0.00222222

predicted=0.002

error=0

Perfect.

What has been built so far?

If we redraw the architecture:

Process Input
      ?
      ?
Evaluator
      ?
      ?
EvaluationResult
      ?
      ???????????????
      ?             ?
Loaded Builder   Final Builder
      ?             ?
      ?             ?
Loaded ??      Final ??
      ?             ?
      ???????????????
             ?
SpatialCurveIntegrator
             ?
             ?
Loaded Geometry

Final Geometry

Manufacturing State
             ?
             ?
State Advancer

Notice something important.

Everything currently exists independently.

Nothing is coupled incorrectly.

That is exactly what we wanted from the architecture.

This is the perfect moment to connect them

Until now we've proven each subsystem independently.

The next phase is not to create a new subsystem.

It is to make these existing subsystems communicate.

Phase 10L

This is where the architecture becomes much more elegant.

Instead of having

Loaded geometry

Final geometry

as two completely separate objects,

the renderer will ask

"Given the current manufacturing state,
what curvature profile should I use right now?"

For example

Ready

? = 0

?

ApplyingTension

? = 0

?

Forming

? =
bendingFraction × loaded?

?

LoadedHold

? =
loaded?

?

Unloading

? =
interpolate(
loaded?,
final?,
unloadingFraction)

?

Complete

? =
final?

The important architectural change is that 
the renderer will no longer choose between
a "loaded geometry" and a "final geometry." 
Instead, it will integrate a single, 
instantaneous curvature profile derived from
the current manufacturing state. That keeps one
integration pipeline and makes the animation
physically meaningful.

My recommendation before Phase 10L

I would make one small architectural refinement
before writing any code.

Currently, the state contains:

tensionFraction

bendingFraction

unloadingFraction

These are machine state variables.

Phase 10L will need material state:

currentCurvature

currentTorsion

Those are not the same thing.

For example:

bendingFraction = 0.50

does not automatically define:

currentCurvature

because later you may introduce nonlinear bending laws, 
material hardening, or machine calibration.

So instead of embedding curvature calculations
inside StretchBendingManufacturingStateAdvancer, 
I recommend introducing a separate class, for example:

StretchBendingCurrentProfileBuilder

with a responsibility like:

ManufacturingState
      +
EvaluationResult
      +
LoadedProfile
      +
FinalProfile
             ?
             ?
Current Curvature/Torsion Profile

That preserves the separation we've maintained throughout 
the project:
==============
Advancer ? "Where is the process in time?"
CurrentProfileBuilder ? "Given that process state,
what is the instantaneous ?(s), ?(s)?"
SpatialCurveIntegrator ? "Given ?(s), ?(s),
compute the geometry."
================= 
I think that separation will make future additions 
like nonlinear springback or varying curvature 
distributions much easier without modifying the
state machine itself. This fits very well
with the architecture

you've been building.

==========================================================

Phase 10K.5 — Separate Process State from Material Curvature

Before Phase 10L, add one small architectural layer:

StretchBendingManufacturingState
        ?
        ? process fractions and current stage
        ?
StretchBendingCurrentProfileParameters
        ?
        ? current curvature/torsion commands
        ?
Phase 10L:
StretchBendingCurrentProfileBuilder

This refinement prevents the state advancer from becoming responsible for material mechanics.

The distinction is:

ManufacturingState
    describes what the machine is doing

CurrentProfileParameters
    describes what curvature and torsion
    the material should currently have

For example:

bendingFraction = 0.5

is a process value.

It does not itself store:

currentCurvature = 0.00111111

The conversion will be handled separately.

10K.5.1 — Do not add curvature to the state

Keep StretchBendingManufacturingState as it currently is:

double tensionFraction;
double bendingFraction;
double unloadingFraction;

Do not add:

double currentCurvature;
double currentTorsion;

to the manufacturing state.

That would mix two responsibilities:

process timeline
+
material geometry

===================================================

================================

Phase 10L — Duplicate-boundary sampling check

Your six samples are correct. Now we must verify the
function inside SpatialCurveIntegrator that reads ?
and ? for a requested arc length.

I could not locate that interpolation function in the 
uploaded excerpts, so use the following check directly in 
SpatialCurveIntegrator.cpp.
Do not insert the profile-sampling diagnostic into:

debugTestStretchBendingCurrentProfileParameters(...)

That function tests only this transformation:

manufacturing state
        +
evaluation
        ?
current curvature / torsion parameters

It does not build a CurvatureTorsionProfile, so there is no currentProfile available to sample.

Correct place

Insert it inside:

void AppController::
debugTestStretchBendingCurrentProfileBuilder(
    const StretchBendingEvaluationResult& evaluation)

Place it after:

const CurvatureTorsionProfile currentProfile =
    StretchBendingCurrentProfileBuilder::build(
        state,
        evaluation
    );

and preferably after the six-sample acceptance check has passed.

The structure should be:

void AppController::
debugTestStretchBendingCurrentProfileBuilder(
    const StretchBendingEvaluationResult& evaluation)
{
    if (!DEBUG_TEST_STRETCH_BENDING_INPUT)
        return;

    StretchBendingManufacturingState state =
        debugStretchManufacturingState;

    if (!state.isValid())
    {
        return;
    }

    if (!evaluation.valid)
    {
        return;
    }

    // =================================================
    // 1. CONFIGURE TEST STATE
    // =================================================

    state.stage =
        StretchBendingManufacturingStage::LoadedHold;

    state.bendingFraction =
        1.0;

    state.unloadingFraction =
        0.0;

    // =================================================
    // 2. BUILD CURRENT SPATIAL PROFILE
    // =================================================

    const CurvatureTorsionProfile currentProfile =
        StretchBendingCurrentProfileBuilder::build(
            state,
            evaluation
        );

    // =================================================
    // 3. VERIFY PROFILE CONSTRUCTION
    //
    // Your existing six-sample acceptance code remains
    // here.
    // =================================================

    // sampleCountAccepted
    // sampleOrderAccepted
    // sampleValuesAccepted
    // profileAcceptance

    // =================================================
    // 4. VERIFY PROFILE SAMPLING
    //
    // ADD SpatialCurveIntegrator and
    // ProfileSamplingCase HERE.
    // =================================================

    SpatialCurveIntegrator integrator;

    struct ProfileSamplingCase
    {
        const char* name;

        double arcLength;

        double expectedCurvature;

        double expectedTorsion;
    };

    // Sampling cases continue here...
}
Why this is the correct place

There are three separate layers.

Layer 1 — Parameter resolver

Your function:

debugTestStretchBendingCurrentProfileParameters(...)

tests:

state
  ?
single current ? and ? pair

For example:

LoadedHold
    ?
? = 0.00222222
? = 0

It knows nothing about spatial positions such as:

s = 20
s = 40
s = 100
s = 160
s = 180

Therefore, sampling a profile there would mix responsibilities.

Layer 2 — Profile builder

The function:

debugTestStretchBendingCurrentProfileBuilder(...)

tests:

current ? and ?
        +
active zone
        ?
?(s), ?(s) profile

It produces:

const CurvatureTorsionProfile currentProfile

Now we have both:

profile data

and:

arc-length positions

This is exactly what the sampling test requires.

Layer 3 — Spatial integrator sampler

The call:

integrator.sampleProfileForDebug(
    currentProfile,
    testCase.arcLength,
    actualCurvature,
    actualTorsion
);

tests:

?(s), ?(s) profile
        +
requested s
        ?
actual ? and ? at that position

So the correct learning sequence is:

CurrentProfileParameters test
    asks:
    "What ? and ? are active now?"

CurrentProfileBuilder test
    asks:
    "Where along the pipe do they apply?"

Profile-sampling test
    asks:
    "What values does the integrator actually read at s?"
Exact insertion point

Use this pattern:

const bool profileAcceptance =
    currentProfile.valid
    && sampleCountAccepted
    && sampleOrderAccepted
    && sampleValuesAccepted;

std::cout
    << "[STRETCH CURRENT PROFILE ACCEPTANCE]"
    // ...
    << std::endl;

// =====================================================
// STOP IF PROFILE CONSTRUCTION FAILED
//
// Sampling an invalid profile would hide the real cause
// of the failure.
// =====================================================

if (!profileAcceptance)
{
    std::cout
        << "[STRETCH CURRENT PROFILE SAMPLING SUMMARY]"
        << " accepted=0"
        << " reason=ProfileConstructionFailed"
        << std::endl;

    return;
}

// =====================================================
// SAMPLE THE SAME PROFILE FUNCTION USED BY INTEGRATION
// =====================================================

SpatialCurveIntegrator integrator;

struct ProfileSamplingCase
{
    const char* name;

    double arcLength;

    double expectedCurvature;

    double expectedTorsion;
};

// Define samplingCases...

This placement is best because the test follows the real data flow:

build profile
    ?
validate profile structure
    ?
sample profile values
    ?
later integrate geometry

Do not put it in the parameter test, because that function ends one architectural layer too early.

Yes. Thanks .Right you mentioned about this before. Check now end of void AppController::
debugTestStretchBendingCurrentProfileBuilder(............... Now( before building) I have one errror :Severity	Code	Description	Project	File	Line	Suppression State	Details
Error (active)	E0265	function "SpatialCurveIntegrator::sampleProfileForDebug" (declared at line 44 of "C:\Users\marek\source\repos\pipe_bender_repo\Core\Geometry\SpatialCurveIntegrator.h") is inaccessible	pipe_bender_repo.exe - Qt-Debug	C:\Users\marek\source\repos\pipe_bender_repo\App\AppController.cpp	3185		

The error means sampleProfileForDebug() is declared in a private section of SpatialCurveIntegrator.

AppController cannot call private methods.

Fix

Open:

SpatialCurveIntegrator.h

You likely have something like:

class SpatialCurveIntegrator
{
public:
    SpatialCurveIntegrationResult integrate(
        const Frame& startFrame,
        const CurvatureTorsionProfile& profile,
        double sampleStep
    ) const;

private:
    bool sampleProfileForDebug(
        const CurvatureTorsionProfile& profile,
        double arcLength,
        double& outCurvature,
        double& outTorsion
    ) const;
};

Move sampleProfileForDebug() into the public: section:

class SpatialCurveIntegrator
{
public:
    SpatialCurveIntegrationResult integrate(
        const Frame& startFrame,
        const CurvatureTorsionProfile& profile,
        double sampleStep
    ) const;

    // Debug/test access to the same profile sampling logic
    // used internally by integrate().
    bool sampleProfileForDebug(
        const CurvatureTorsionProfile& profile,
        double arcLength,
        double& outCurvature,
        double& outTorsion
    ) const;

private:
    bool sampleProfileAtArcLength(
        const CurvatureTorsionProfile& profile,
        double arcLength,
        double& outCurvature,
        double& outTorsion
    ) const;
};

Then in SpatialCurveIntegrator.cpp:

bool SpatialCurveIntegrator::sampleProfileForDebug(
    const CurvatureTorsionProfile& profile,
    double arcLength,
    double& outCurvature,
    double& outTorsion) const
{
    return sampleProfileAtArcLength(
        profile,
        arcLength,
        outCurvature,
        outTorsion
    );
}
Why this arrangement is correct

Keep the real worker private:

sampleProfileAtArcLength(...)

Expose only a thin public test wrapper:

sampleProfileForDebug(...)

So production integration still owns the real sampling logic, while diagnostics can verify the exact same implementation.

Do not make both functions public.

building complete.Analize console output:InsideRange=1 recommendedTension=244102 kappa=0.002 bending=0.02 axial=0.03 inner=0.01 outer=0.05 feasible=1 aboveYield=1
[STRETCH CASE] name=InnerCompression expected=InnerWallCompressionRisk actual=InnerWallCompressionRisk pass=1 minAxial=0.02 recommendedAxial=0.04 maxAxial=0.06 commandedInsideRange=0 recommendedTension=244102 kappa=0.002 bending=0.02 axial=0.01 inner=-0.01 outer=0.03 feasible=1 aboveYield=1
[STRETCH CASE] name=OuterLimit expected=OuterWallStrainExceeded actual=OuterWallStrainExceeded pass=1 minAxial=0.02 recommendedAxial=0.04 maxAxial=0.06 commandedInsideRange=0 recommendedTension=244102 kappa=0.002 bending=0.02 axial=0.07 inner=0.05 outer=0.09 feasible=1 aboveYield=1
[STRETCH CASE] name=BelowYield expected=BelowYield actual=BelowYield pass=1 minAxial=0.001 recommendedAxial=0.04 maxAxial=0.079 commandedInsideRange=1 recommendedTension=244102 kappa=0.0001 bending=0.001 axial=0.001 inner=0 outer=0.002 feasible=1 aboveYield=0
[STRETCH CASE SUMMARY] passed=4/4 result=PASS
[STRETCH PROFILE] evaluationStatus=Valid valid=1 samples=2 totalLength=200
[STRETCH PROFILE VALUES] firstS=0 firstKappa=0.00222222 firstTau=0 lastS=200 lastKappa=0.00222222 lastTau=0
[STRETCH PROFILE REJECTION] evaluationStatus=GeometryNotFeasible profileValid=0 samples=0
[STRETCH CURRENT PARAMETERS] test=Ready stage=Ready bendingFraction=0 unloadingFraction=0 curvature=0 torsion=0 valid=1
[STRETCH CURRENT PARAMETERS] test=HalfForming stage=Forming bendingFraction=0.5 unloadingFraction=0 curvature=0.00111111 torsion=0 valid=1
[STRETCH CURRENT PARAMETERS] test=LoadedHold stage=LoadedHold bendingFraction=1 unloadingFraction=0 curvature=0.00222222 torsion=0 valid=1
[STRETCH CURRENT PARAMETERS] test=HalfUnloading stage=Unloading bendingFraction=1 unloadingFraction=0.5 curvature=0.00211111 torsion=0 valid=1
[STRETCH CURRENT PARAMETERS] test=Complete stage=Complete bendingFraction=1 unloadingFraction=1 curvature=0.002 torsion=0 valid=1
[STRETCH CURRENT PROFILE BUILDER SUCCESS] samples=6 totalArcLength=200 valid=1
[STRETCH CURRENT PROFILE SAMPLE] index=0 s=0 curvature=0 torsion=0
[STRETCH CURRENT PROFILE SAMPLE] index=1 s=40 curvature=0 torsion=0
[STRETCH CURRENT PROFILE SAMPLE] index=2 s=40 curvature=0.00222222 torsion=0
[STRETCH CURRENT PROFILE SAMPLE] index=3 s=160 curvature=0.00222222 torsion=0
[STRETCH CURRENT PROFILE SAMPLE] index=4 s=160 curvature=0 torsion=0
[STRETCH CURRENT PROFILE SAMPLE] index=5 s=200 curvature=0 torsion=0
[STRETCH CURRENT PROFILE ACCEPTANCE] profileValid=1 sampleCount=6 sampleCountAccepted=1 sampleOrderAccepted=1 sampleValuesAccepted=1 accepted=1
[STRETCH CURRENT PROFILE TEST SUMMARY] samples=6 totalArcLength=200 valid=1 accepted=1
[STRETCH CURRENT PROFILE SAMPLING] case=BeforeStart s=20 sampled=1 curvature=0 expectedCurvature=0 torsion=0 expectedTorsion=0 finite=1 accepted=1
[STRETCH CURRENT PROFILE SAMPLING] case=StartLeft s=40 sampled=1 curvature=0 expectedCurvature=0 torsion=0 expectedTorsion=0 finite=1 accepted=1
[STRETCH CURRENT PROFILE SAMPLING] case=StartExact s=40 sampled=1 curvature=0.00222222 expectedCurvature=0.00222222 torsion=0 expectedTorsion=0 finite=1 accepted=1
[STRETCH CURRENT PROFILE SAMPLING] case=StartRight s=40 sampled=1 curvature=0.00222222 expectedCurvature=0.00222222 torsion=0 expectedTorsion=0 finite=1 accepted=1
[STRETCH CURRENT PROFILE SAMPLING] case=Inside s=100 sampled=1 curvature=0.00222222 expectedCurvature=0.00222222 torsion=0 expectedTorsion=0 finite=1 accepted=1
[STRETCH CURRENT PROFILE SAMPLING] case=EndLeft s=160 sampled=1 curvature=0.00222222 expectedCurvature=0.00222222 torsion=0 expectedTorsion=0 finite=1 accepted=1
[STRETCH CURRENT PROFILE SAMPLING] case=EndExact s=160 sampled=1 curvature=0 expectedCurvature=0 torsion=0 expectedTorsion=0 finite=1 accepted=1
[STRETCH CURRENT PROFILE SAMPLING] case=EndRight s=160 sampled=1 curvature=0 expectedCurvature=0 torsion=0 expectedTorsion=0 finite=1 accepted=1
[STRETCH CURRENT PROFILE SAMPLING] case=AfterEnd s=180 sampled=1 curvature=0 expectedCurvature=0 torsion=0 expectedTorsion=0 finite=1 accepted=1
[STRETCH CURRENT PROFILE SAMPLING SUMMARY] passed=9/9 accepted=1
[STRETCH SHAPE COMPARISON] loadedValid=1 loadedComplete=1 finalValid=1 finalComplete=1 loadedKappa=0.00222222 finalKappa=0.002 targetKappa=0.002
[STRETCH SPRINGBACK DISPLACEMENT] loadedEnd=(193.48, -176.282, 0) finalEnd=(194.709, -180.53, 0) endpointRecoveryLength=4.4223
[STRETCH GEOMETRY] evaluationStatus=Valid loadedProfileValid=1 resultValid=1 complete=1 nodes=801 requestedLength=200 integratedLength=200 loadedCurvature=0.00222222 torsion=0
[STRETCH GEOMETRY ENDPOINT] firstP=(0, -220, 0) lastP=(193.48, -176.282, 0)
[STRETCH LOADED GEOMETRY ACCURACY] loadedKappa=0.00222222 expectedEnd=(193.48, -176.282, 0) positionError=5.37282e-06
[STRETCH FINAL GEOMETRY ACCURACY] finalKappa=0.002 expectedEnd=(194.709, -180.53, 0) positionError=4.0238e-06
[STRETCH MANUFACTURING STATE] stage=Ready progress=0 tensionFraction=0 bendingFraction=0 unloadingFraction=0 valid=1
[STRETCH ACTIVE ZONE] startS=40 endS=160 length=120 totalLength=200 valid=1
[STRETCH ACTIVE ZONE CLASSIFICATION] beforeS=20 beforeActive=0 insideS=100 insideActive=1 afterS=180 afterActive=0
[STRETCH ACTIVE ZONE REJECTION] zoneStart=170 zoneEnd=230 pipeLength=200 stateStage=Invalid stateValid=0
[STRETCH STATE TRANSITION] stage=Ready time=0 progress=0
[STRETCH STATE TRANSITION] stage=ApplyingTension time=0.25 progress=0.0555556 tensionFraction=0.25 bendingFraction=0 unloadingFraction=0
[STRETCH STATE TRANSITION] stage=Forming time=1 progress=0.222222 tensionFraction=1 bendingFraction=0 unloadingFraction=0
[STRETCH STATE TRANSITION] stage=LoadedHold time=3 progress=0.666667 tensionFraction=1 bendingFraction=1 unloadingFraction=0
[STRETCH STATE TRANSITION] stage=Unloading time=3.5 progress=0.777778 tensionFraction=1 bendingFraction=1 unloadingFraction=0
[STRETCH STATE TRANSITION] stage=Complete time=4.5 progress=1 tensionFraction=0 bendingFraction=1 unloadingFraction=1
[STRETCH STATE PROGRESSION SUMMARY] stage=Complete elapsedTime=4.5 totalDuration=4.5 progress=1 tensionFraction=0 bendingFraction=1 unloadingFraction=1 accepted=1

This output confirms the remaining Phase 10L technical check passes.

Key result
[STRETCH CURRENT PROFILE SAMPLING SUMMARY]
passed=9/9
accepted=1

The sampler correctly handles all relevant regions and 
both duplicate boundaries.

Start boundary
StartLeft   ? ?=0
StartExact  ? ?=0.00222222
StartRight  ? ?=0.00222222

So the active curvature begins exactly at:

s=40
End boundary
EndLeft   ? ?=0.00222222
EndExact  ? ?=0
EndRight  ? ?=0

So the active curvature ends exactly at:

s=160

This matches the intended right-continuous rule:

?(s)=
?
?
?
	?

0,
0.00222222,
0,
	?

0?s<40
40?s<160
160?s?200
	?

Important numerical confirmation

Every sampling case reports:

sampled=1
finite=1
accepted=1

Therefore:

no division by zero occurred,
no NaN or infinity appeared,
duplicate arc lengths were handled safely,
exact-boundary selection is correct,
no unintended interpolation ramp exists.
Phase 10L status
Current parameter resolution      PASS
Active-zone profile construction  PASS
Six-sample layout                 PASS
Sample ordering                   PASS
Sample values                     PASS
Duplicate-boundary sampling       PASS
Finite numerical output           PASS
Boundary semantics                PASS

Phase 10L is now complete.

The next logical phase is:
=============================================================

===============================================================

Phase 10M — Integrate and store state-driven active-zone geometry

That phase should use the newly verified currentProfile
to generate a straight–arc–straight centerline and compare
its geometry against the 
expected active-zone shape.

Phase 10M — Integrate and Store State-Driven Active-Zone Geometry

Phase 10L proved that the current profile is:

s=0          s=40                 s=160         s=200
 |-------------|=====================|-------------|
   straight          active bend          straight

Phase 10M converts that profile into centerline nodes using the shared SpatialCurveIntegrator.

This geometry must remain separate from:

debugStretchLoadedIntegrationResult
debugStretchFinalIntegrationResult
normal manufacturing geometry
rotary-draw geometry
10M.1 — Add a separate st

10M.1 — Add a separate stored result

In AppController.h, near your other debug integration results, add:

// State-driven active-zone geometry.
//
// This result represents the instantaneous geometry
// generated from:
//
//     manufacturing state
//     +
//     current active-zone profile
//
// It is separate from the full-length loaded and final
// reference geometries.
SpatialCurveIntegrationResult
    debugStretchCurrentIntegrationResult;

Do not reuse:

debugStretchLoadedIntegrationResult

because that result represents constant loaded curvature over the entire 200 mm pipe.

The new result represents:

40 mm straight
120 mm bent
40 mm straight

10M.2 — Add a dedicated diagnostic function

In AppController.h add:

void debugTestStretchBendingCurrentGeometry(
    const StretchBendingEvaluationResult& evaluation
);

The parameter is required because evaluation is local to:

debugTestStretchBendingGeometry()
10M.3 — Call it from the correct place

Inside:

void AppController::debugTestStretchBendingGeometry()

you already have:

debugTestStretchBendingCurrentProfileParameters(
    evaluation
);

debugTestStretchBendingCurrentProfileBuilder(
    evaluation
);

Add:

debugTestStretchBendingCurrentGeometry(
    evaluation
);

The order becomes:

debugTestStretchBendingCurrentProfileParameters(
    evaluation
);

debugTestStretchBendingCurrentProfileBuilder(
    evaluation
);

debugTestStretchBendingCurrentGeometry(
    evaluation
);

This placement is correct because both required inputs already exist:

evaluation
debugStretchManufacturingState


10M.4 — Implement the geometry test

Add to AppController.cpp:

void AppController::
debugTestStretchBendingCurrentGeometry(
    const StretchBendingEvaluationResult& evaluation)
{
    if (!DEBUG_TEST_STRETCH_BENDING_INPUT)
        return;

    // =================================================
    // 1. COPY THE STORED READY STATE
    //
    // Work on a copy so this diagnostic does not change
    // the manufacturing state us..................
    ............................
    10M.5 — Expected geometry

For your current values:

total length     = 200
active start     = 40
active end       = 160
active length    = 120
after length     = 40
loaded curvature = 0.00222222
torsion          = 0

the bend angle inside the active zone is:

?=?L
active
	?

?=0.00222222×120
??0.2666664 rad

approximately:

15.28
?

The generated centerline should therefore look like:

start
  ????????????????????
      40 mm straight   \
                        \
                         ) 120 mm shallow arc
                          \
                           \????????????????? end
                               40 mm straight

The final straight section must follow the tangent
created by the active bend. It must not return to 
the original +X direction.

10M.6 — Expected basic console output

Approximately:

[STRETCH CURRENT GEOMETRY]
stage=LoadedHold
profileValid=1
resultValid=1
complete=1
samples=6
nodes=801
requestedLength=200
integratedLength=200
activeStart=40
activeEnd=160
curvature=0.00222222
torsion=0

And finally:

[STRETCH CURRENT GEOMETRY ACCEPTANCE] PASS

Because:

200 / 0.25 = 800 integration steps

the expected node count is:

801 nodes

including the initial node.

Phase 10M data flow
LoadedHold manufacturing state
             ?
             ?
CurrentProfileParameterResolver
             ?
             ?
?current = 0.00222222
?current = 0
             ?
             ?
CurrentProfileBuilder
             ?
             ?
straight / active / straight profile
             ?
             ?
SpatialCurveIntegrator
             ?
             ?
debugStretchCurrentIntegrationResult
             ?
             ?
straight–arc–straight centerline nodes

Phase 10M is accepted when the stored result is complete, contains 801 nodes, reaches exactly 200 mm, and the analytical
position/tangent checks pass.

============================================================






Phase 10O — Drive Current Geometry Through 
Manufacturing-State Progression

The goal now is to stop showing only a fixed LoadedHold shape.

Instead, the orange geometry should follow:

Ready
ApplyingTension
Forming
LoadedHold
Unloading
Complete

The pipeline becomes:

elapsed time
    ?
StretchBendingManufacturingStateAdvancer
    ?
updated manufacturing state
    ?
StretchBendingCurrentProfileBuilder
    ?
current ?(s), ?(s)
    ?
SpatialCurveIntegrator
    ?
debugStretchCurrentIntegrationResult
    ?
orange preview
10O.1 Store the evaluation and start frame

The update function will need data that currently
exists only locally inside:

debugTestStretchBendingGeometry()

Add these members to AppController.h:

StretchBendingEvaluationResult
    debugStretchEvaluationResult;

Frame
    debugStretchCurrentStartFrame;

double debugStretchCurrentSampleStep =
    0.25;

bool debugStretchPlaybackPrepared =
    false;

These members allow later state updates without 
rerunning the initial
feasibility setup.

10O.2 Store the valid evaluation

Inside:

debugTestStretchBendingGeometry()

after:

if (!evaluation.valid)
{
    // ...
    return;
}

store it:

debugStretchEvaluationResult =
    evaluation;


10O.3 Store the current-preview start frame

In:

debugTestStretchBendingCurrentGeometry(...)

you currently create:

Frame startFrame;

After configuring it, store it:

debugStretchCurrentStartFrame =
    startFrame;

debugStretchCurrentSampleStep =
    CURRENT_GEOMETRY_SAMPLE_STEP;

Then, after successful integration:

debugStretchPlaybackPrepared =
    result.valid
    && result.isComplete();

On rejection, reset it:

debugStretchPlaybackPrepared =
    false;





    10O.4 Add a geometry rebuild function

In AppController.h:

bool rebuildDebugStretchCurrentGeometry();

In AppController.cpp:

bool AppController::
rebuildDebugStretchCurrentGeometry()
{
    if (!debugStretchPlaybackPrepared)
    {
        debugStretchCurrentIntegrationResult.clear();
        return false;
    }

    if (!debugStretchManufacturingState.isValid())
    {
        debugStretchCurrentIntegrationResult.clear();
        return false;
    }

    if (!debugStretchEvaluationResult.valid)
    {
        debugStretchCurrentIntegrationResult.clear();
        return false;
    }

    const CurvatureTorsionProfile currentProfile =
        StretchBendingCurrentProfileBuilder::build(
            debugStretchManufacturingState,
            debugStretchEvaluationResult
        );

    if (!currentProfile.valid)
    {
        debugStretchCurrentIntegrationResult.clear();
        return false;
    }

    SpatialCurveIntegrator integrator;

    debugStretchCurrentIntegrationResult =
        integrator.integrate(
            debugStretchCurrentStartFrame,
            currentProfile,
            debugStretchCurrentSampleStep
        );

    return
        debugStretchCurrentIntegrationResult.valid
        && debugStretchCurrentIntegrationResult.isComplete();
}

This function owns:

state
    ?
profile
    ?
geometry

It should not advance time.
It only rebuilds geometry from the current state.


10O.5 Add a state-advance function

In AppController.h:

void advanceDebugStretchBendingPlayback(
    double deltaTime
);

Implementation:

void AppController::
advanceDebugStretchBendingPlayback(
    double deltaTime)
{
    if (!debugStretchPlaybackPrepared)
        return;

    if (!std::isfinite(deltaTime))
        return;

    if (deltaTime <= 0.0)
        return;

    StretchBendingManufacturingStateAdvancer advancer;

    advancer.advance(
        debugStretchManufacturingState,
        deltaTime
    );

    rebuildDebugStretchCurrentGeometry();
}

Adapt the advance() arguments to your actual advancer API.
It may require:

state
timing
deltaTime

For example:

debugStretchManufacturingState =
    StretchBendingManufacturingStateAdvancer::advance(
        debugStretchManufacturingState,
        debugStretchTiming,
        deltaTime
    );

Use the exact pattern already used in your state-progression test.




10O.6 Important architectural rule

Do not advance or integrate inside:

GLView::paintGL()

paintGL() should only read and draw:

AppController updates state and geometry
GLView renders stored geometry

Do not do:

advanceDebugStretchBendingPlayback(...)

inside:

drawStretchCurrentGeometryDebugPreview()

because rendering frequency is not a reliable simulation clock.


10O.7 Add a temporary manual step command

For the first test, use a key press instead of continuous animation.

For example, in your key handling:

case Qt::Key_BracketRight:
{
    if (app)
    {
        app->advanceDebugStretchBendingPlayback(
            0.25
        );

        update();
    }

    break;
}

Each key press advances the process by:

0.25 seconds

This gives predictable debugging.

Suggested keys:

] = advance stretch playback
[ = reset stretch playback




10O.8 Add reset support

In AppController.h:

void resetDebugStretchBendingPlayback();

Implementation:

void AppController::
resetDebugStretchBendingPlayback()
{
    if (!debugStretchEvaluationResult.valid)
        return;

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
        debugStretchCurrentSampleStep;

    StretchBendingActiveZone activeZone;

    activeZone.startS =
        40.0;

    activeZone.endS =
        160.0;

    debugStretchManufacturingState =
        StretchBendingManufacturingStateBuilder::
            buildReadyState(
                input,
                debugStretchEvaluationResult,
                activeZone
            );

    rebuildDebugStretchCurrentGeometry();

    std::cout
        << "[STRETCH PLAYBACK RESET]"
        << " stage="
        << stretchBendingManufacturingStageToString(
            debugStretchManufacturingState.stage
        )
        << " time="
        << debugStretchManufacturingState.elapsedTime
        << std::endl;
}

A cleaner future improvement would store the accepted input too,
but this is sufficient for the current debug phase.




10O.9 Add one update diagnostic

After advancing and rebuilding:

std::cout
    << "[STRETCH PLAYBACK STEP]"
    << " stage="
    << stretchBendingManufacturingStageToString(
        debugStretchManufacturingState.stage
    )
    << " time="
    << debugStretchManufacturingState.elapsedTime
    << " progress="
    << debugStretchManufacturingState.progress
    << " tensionFraction="
    << debugStretchManufacturingState.tensionFraction
    << " bendingFraction="
    << debugStretchManufacturingState.bendingFraction
    << " unloadingFraction="
    << debugStretchManufacturingState.unloadingFraction
    << " geometryValid="
    << debugStretchCurrentIntegrationResult.valid
    << " nodes="
    << debugStretchCurrentIntegrationResult.nodes.size()
    << std::endl;

Expected progression:

Ready:
    orange shape straight

ApplyingTension:
    still straight

Forming:
    orange active zone gradually bends

LoadedHold:
    maximum loaded bend

Unloading:
    orange bend opens slightly

Complete:
    final unloaded curvature remains


Why store the timing?

Think of it exactly like the other stored data.

evaluation
        ?
        ??? tells HOW MUCH to bend
        ?
timing
        ?
        ??? tells HOW FAST to bend
        ?
state
        ?
        ??? tells WHERE WE ARE NOW
        ?
current profile
        ?
geometry
        ?
renderer

Each object has a single responsibility:

Evaluation ? process physics.
Timing ? process schedule.
State ? current manufacturing moment.
Profile Builder ? current ?(s), ?(s).
Integrator ? current geometry.
Renderer ? display.

This separation is a clean architecture
because changing the timing (for example,
making unloading twice as slow) won't require 
touching the physics or the geometry builder. 
Only the timing object changes.

===============================================
===============================================
Phase 10P — Automatic Timed Playback, Pause/Resume, and Speed Control
==============================================================
=============================================================
Phase 10O already established the correct update function:

controller.advanceDebugStretchBendingPlayback(
    deltaTime
);

Phase 10P must call that same function automatically from a Qt timer.

The ownership remains:

MainWindow / UI timer
        ? supplies elapsed time
        ?
AppController::advanceDebugStretchBendingPlayback()
        ? advances state and rebuilds geometry
        ?
GLView::paintGL()
        ? only renders stored geometry
        ?
orange preview

Do not advance state inside paintGL().

10P.1 — Add timer state to MainWindow

In MainWindow.h, add:

#include <QElapsedTimer>
#include <QTimer>

Inside the MainWindow class, add private members:

private:
    // Periodically requests automatic stretch-playback
    // advancement.
    QTimer stretchPlaybackTimer;

    // Measures real elapsed time between timer callbacks.
    //
    // QTimer intervals are not guaranteed to be exact,
    // so elapsed time should be measured rather than
    // assuming every callback is exactly 16 ms.
    QElapsedTimer stretchPlaybackClock;

    bool stretchPlaybackRunning =
        false;

    // 1.0 = normal process time
    // 0.5 = half speed
    // 2.0 = double speed
    double stretchPlaybackSpeed =
        1.0;

Add private helper declarations:

private:
    void toggleStretchPlayback();

    void startStretchPlayback();

    void pauseStretchPlayback();

    void updateStretchPlayback();

    void increaseStretchPlaybackSpeed();

    void decreaseStretchPlaybackSpeed();