Proposed implementation sequence

I would now temporarily branch away from the old uniform-active-zone model and create a new path:

Phase H1
Define StretchHelixWrappingInput
    support radius
    pipe diameter
    axial speed
    rotation speed
    pipe length
    stretch strain

Phase H2
Compute machine-command helix:
    P, r, ?, ?

Phase H3
Render the cylindrical support/reference helix

Phase H4
Create MovingContactState:
    wrappedLength / contactFrontS

Phase H5
Build ?(s),?(s) behind the moving contact front

Phase H6
Integrate and render current progressively wrapped pipe

Phase H7
Synchronize contact advancement with ? and axial feed

Phase H8
Apply tension mechanics and feasibility limits

Phase H9
Add unloading and curvature/torsion springback

Phase H10
Accept a pre-bent semi-finished pipe from Pass 1

That last point is especially important for your production process: 
the initial input to this model eventually does not need to be a straight pipe. 
It can be the centerline geometry produced by the first machine/pass.

So the long-term flow becomes:

PASS 1
rotary draw + roll forming
        ?
        ?
semi-finished pipe geometry
        ?
        ?
PASS 2
stretch-helix wrapping machine
        ?
        ??? tension
        ??? cylinder rotation
        ??? axial movement
        ??? moving contact front
        ?
        ?
helical finished component

This is the model I recommend pursuing now. The existing stretch-bending
implementation remains useful as a verified mechanics/solver foundation, but 
the moving-contact wrapping model should become the actual 
stretch-helix production model.

Proposed new model

I recommend calling it something like:

StretchHelixWrappingProcess

rather than extending the existing StretchBendingCurrentProfileBuilder.

The new data flow should be:

MACHINE COMMANDS

support radius
rotation speed ?
axial speed vz
tension T / stretch strain ?0
pipe length
        ?
        ?
HelixWrappingKinematics
        ?
        ??? pitch P
        ??? centerline radius r
        ??? curvature ?
        ??? torsion ?
        ?
        ?
MovingContactState
        ?
        ??? wrappedLength
        ??? contactFrontS
        ??? contactProgress
        ?
        ?
ContactProfileBuilder
        ?
        ?
?(s), ?(s)
        ?
        ?
SpatialCurveIntegrator
        ?
        ?
current pipe geometry

The fundamental time-dependent variable should no longer be:

?(t)

as in your current stretch-bending model.

Instead:

wrappedLength(t)

or:

contactFrontS(t)

should grow.

===========================================================================
Phase H1 — Define StretchHelixWrappingInput

H1 should do data definition only. No geometry integration, no contact progression, no rendering yet.

The purpose is to describe the machine command and workpiece needed for the wrapping process:

support cylinder
      +
pipe geometry/material
      +
axial motion
      +
rotation
      +
stretch/tension command
      ?
StretchHelixWrappingInput
H1.1 — Create the input type

Create:

Core/Forming/StretchHelixWrappingInput.h
#pragma once

#include <cmath>

#include "Core/Forming/StretchBendingPipeSection.h"
#include "Core/Forming/StretchBendingMaterial.h"

// =====================================================
// STRETCH-HELIX WRAPPING INPUT
//
// Describes the commanded setup for progressively
// wrapping a tensioned pipe around a cylindrical support.
//
// This structure contains only process input.
// It does NOT contain:
//....................
H1.2 — Why supportOuterRadius, not helixRadius

This is intentional.

Your machine controls the physical support radius:

support axis
    O
    |
    | supportOuterRadius
    |
    +--------- support surface
              |
              | pipeRadius
              |
              * workpiece centerline

So later we derive:

helixCenterlineRadius =
    supportOuterRadius
    + pipeSection.outerDiameter * 0.5;

The input should store the machine dimension; derived geometry belongs in H.....
H1.3 — Why keep axialSpeed and rotationSpeed

Do not store pitch as the primary command yet.

The machine command is:

rotationSpeed
+
axialSpeed

and later H2 derives:

P=
???
2?v
z
	?

	?


This lets the simulator answer:

“What helix does this machine motion actually produce?”

instead of forcing the geometry first.

H1.4 — Add a known-valid debug builder

In AppController.h, private:

StretchHelixWrappingInput
buildTestStretchHelixWrappingInput() const;

In AppController.cpp:..........................

H1.5 — Add a simple validation test

In AppController.h, private:

void debugTestStretchHelixWrappingInput() const;

Implementation:.......................
H1.6 — Important architecture boundary

At the end of H1 we should have only:

StretchHelixWrappingInput
        ?
        ??? pipe/material
        ??? support radius
        ??? axial speed
        ??? rotation speed
        ??? rotation direction
        ??? stretch strain
        ??? sample step

We should not yet calculate:

pitch
helix radius
curvature
torsion
contact front
wrapped length
geometry

Those begin in Phase H2 — derive helix geometry and machine kinematics from StretchHelixWrappingInput.

H1 acceptance is simply: the valid case passes
Phase H2 — Derive Helix Geometry and Machine Kinematics

H2 converts the machine-level input from H1 into the geometric quantities the rest of the simulator will use.

The flow is:

StretchHelixWrappingInput
        -
        +¦¦ supportOuterRadius
        +¦¦ pipe outer diameter
        +¦¦ axialSpeed
        +¦¦ rotationSpeed
        L¦¦ rotationDirection
        -
        ¡
StretchHelixWrappingKinematics
        -
        +¦¦ centerlineRadius r
        +¦¦ pitch P
        +¦¦ b = P / 2?
        +¦¦ curvature ?
        +¦¦ torsion ?
        +¦¦ helixAngle ?
        L¦¦ arcLengthPerRevolution

The important idea is that H2 still does no progressive wrapping.
It only answers:

Given this machine motion, what helix would the contacted pipe follow?

H2.1 — Create the result structure

Create:

Core/Forming/StretchHelixWrappingKinematics.h
#pragma once

#include <cmath>

// =====================================================
// STRETCH-HELIX WRAPPING KINEMATICS
//
// Derived geometric/machine quantities for a cylindrical
// helix generated by axial translation plus rotation.
//
// This structure contains no playback/contact state.
// =====================================================

H2.2 — Create the kinematics evaluator

Create:

Core/Forming/StretchHelixWrappingKinematicsBuilder.h
#pragma once

#include "Core/Forming/StretchHelixWrappingInput.h"
#include "Core/Forming/StretchHelixWrappingKinematics.h"

// =====================================================
// STRETCH-HELIX WRAPPING KINEMATICS BUILDER
//
// Converts machine motion into target helix geometry.
// =====================================================

H2.3 — Important note about rotation sign

You currently have both:

double rotationSpeed;
int rotationDirection;

For H2, use:

abs(rotationSpeed)

for the magnitude and:

rotationDirection

for handedness.

So:

rotationSpeed = 2 rad/s
rotationDirection = +1

produces positive torsion.

And:

rotationSpeed = 2 rad/s
rotationDirection = -1

produces negative torsion.

That avoids ambiguous states like:

rotationSpeed = -2
rotationDirection = -1

Later, we may simplify this API to one signed angular velocity,
but do not change H1 now.

H2.4 — Add a debug test

In AppController.h, private:

void debugTestStretchHelixWrappingKinematics() const;

Implementation:

void AppController::
debugTestStretchHelixWrappingKinematics() const
{
    const StretchHelixWrappingInput input =
        buildTestStretchHelixWrappingInput();

    const StretchHelixWrappingKinematics kinematics =
        StretchHelixWrappingKinematicsBuilder::build(
            input
        );
H2.5 — Expected numerical values

With the H1 test input:

supportOuterRadius = 50 mm
pipe OD            = 20 mm   ‹ assuming your existing test pipe is 20
axialSpeed         = 20 mm/s
rotationSpeed      = 2 rad/s

the centerline radius should be:

H2.6 — Add consistency checks

We should verify that the derived values reconstruct the expected geometric identities.

Add after the main diagnostic:

const double denominator =
    kinematics.centerlineRadius
        * kinematics.centerlineRadius
    + kinematics.helixRisePerRadian
        * kinematics.helixRisePerRadian;

const double reconstructedKa

H2.7 — Test handedness

This is important before we ever render a helix.

Add:

StretchHelixWrappingInput oppositeInput =
    input;

oppositeInput.rotationDirection =
    -1;

const StretchHelixWrappingKinematics opposite =
    StretchHelixWrappingKinematicsBuilder::build(
        oppositeInput
    );

const bool handednessAccepted =
    opposite.valid
    && std::abs(
        opposite.curvature
        - kinematics.curvature
    ) <= tolerance
    && std::abs(
        opposite.torsion
        + kinematics.torsion
    ) <= tolerance;

std::cout
    << "[STRETCH HELIX HANDEDNESS]"
    << " positiveTau="
    << kinematics.torsion
    << " negativeTau="
    << opposite.torsion
    << " accepted="
    << handednessAccepted
    << std::endl;

Expected:

positiveTau=...
negativeTau=-...
accepted=1

Curvature must stay the same. Only torsion changes sign.

H2.8 — One conceptual distinction to keep clear

At the end of H2:

rotation + axial motion
        ¡
target contact helix
        ¡
?helix, ?helix

But the pipe has not yet wrapped around anything.

We have only calculated the trajectory it should follow when it is in contact.

So:

H1 = machine command

H2 = helix implied by that machine command

H3 = visualize the support cylinder and reference helix

H4 = introduce moving contact / wrapped length

That sequence is important because when H4 starts moving the contact front, we already know that the geometry behind the front must approach the H2 ? and ?.

H2 acceptance

Proceed to H3 only when you get:

inputValid=1
kinematics valid=1
curvatureAccepted=1
torsionAccepted=1
handedness accepted=1

and the printed pitch/radius/?/? values are numerically sensible.


=================================================================
Phase H3 — Reference Support + Reference Helix

Yes. We should keep H3 much simpler. No new HUD data, no dashboard, no contact mechanics yet. H3 should establish one standard engineering visualization that we can later reuse for the wrapping simulation.

Phase H3 — Reference Support + Reference Helix

The purpose of H3 is only to answer visually:

“For the machine parameters calculated in H2, where is the cylindrical support and where should the pipe centerline lie when fully wrapped?”

Nothing moves yet.

H3 principle

Side view, simplified:

                     pipe centerline helix
                           /
                       /  /
                    /    /
                 /      /
              /        /
          ???????????????????
          ?                 ?
          ?  support pipe   ?
          ?                 ?
          ?                 ?
          ???????????????????
                 ?
                 ?
              axis Z

Looking along the support axis:

                 workpiece centerline
                         ?
                       / |
                      /  | Dpipe / 2
                     /   |
                    ?????+
              support surface
                  /
                 / Rsupport
                /
               O
         support axis

Therefore:

r
helix
	?

=R
support
	?

+
2
D
pipe
	?

	?


For our current test:

support radius       = 50 mm
pipe OD              = 20 mm

centerline radius    = 60 mm

And H2 tells us:

pitch      = 62.8319 mm
?          = 0.0162162
?          = 0.0027027
H3.1 — What we render

Only two new reference objects:

GRAY/CYAN  ? cylindrical support centerline/surface reference
YELLOW     ? full reference helix centerline

Your actual stretch pipe remains separate.

Conceptually:

                      yellow reference helix
                    /      /      /
                  /      /      /
               ????????????????????
               ?                  ?
               ?  support pipe    ?
               ?                  ?
               ????????????????????

Later H4 will introduce:

ORANGE ? actual progressively wrapping pipe

That will give us:

cyan/gray support
yellow target
orange current

This is very similar to your existing rotary-draw debugging philosophy:

reference geometry
       +
current manufactured geometry
H3.2 — Store the H1/H2 results in AppController

We need GLView to read them.

In AppController.h, private fields:

StretchHelixWrappingInput
    debugStretchHelixWrappingInput;

StretchHelixWrappingKinematics
    debugStretchHelixWrappingKinematics;

    H3.3 — Store the valid H2 calculation

In:

debugTestStretchHelixWrappingKinematics()

you currently probably have:

const StretchHelixWrappingInput input =
    buildTestStretchHelixWrappingInput();................................
H3.4 — Build the reference helix using your existing integrator

This is important.

We do not need another helix generator.

You already proved:

SpatialCurveIntegrator
+
constant ?
+
constant ?
=
correct helix

So reuse it.

Add in AppController.h, private field:

SpatialCurveIntegrationResult
    debugStretchHelixReferenceResult;

H3.5 — Build the reference profile

Still after successful H2:

const CurvatureTorsionProfile referenceProfile =
    ConstantCurvatureTorsionProfileBuilder::build(
        input.pipeArcLength,
        kinematics.curvature,
        kinematics.torsion
    );

This describes:

s = 0 -------------------------------- s = L

? = constant
? = constant

This is the fully wrapped target reference, not the current pipe.

H3.6 — Choose a separate start frame

Keep it away from your existing geometry.

For example:

Frame referenceStartFrame;

referenceStartFrame.P =
    Vec3D{
        0.0,
        -500.0,
        0.0
    };

referenceStartFrame.T =
    Vec3D{
        0.0,....................

        .................
                1.0
    };

The exact orientation is not physically important yet.

H3 asks only:

Does the reference geometry look like the correct helix?


H3.7 — Integrate
SpatialCurveIntegrator integrator;

debugStretchHelixReferenceResult =
    integrator.integrate(
        referenceStartFrame,
        referenceProfile,
        input.sampleStep
    );

Then diagnostic:

std::cout
    << "[STRETCH HELIX REFERENCE]"
    << " valid="......................................

    H3.8 — Render the reference helix

In GLView.h, add private:

PipeRenderer
    stretchHelixReferenceRenderer;

bool showStretchHelixReference =
    true;................

H3.9 — Drawing function

In GLView.cpp:

void GLView::drawStretchHelixReference()
{
    if (!showStretchHelixReference)
        return;

    if (!app || !shader)
        return;.......................

H3.10 — Call from paintGL()

Add:

drawStretchHelixReference();

near your other debug/reference geometries.

At this point you should see one yellow helix.

H3 acceptance

Do not add contact progression yet.

First confirm only:

? reference profile valid
? reference integration complete
? yellow helix visible
? LINE works
? MESH works
? approximate radius visually correct
? pitch visually correct
? support reference has radius 50 mm
? helix centerline has radius 60 mm

The conceptual result of H3 should remain very simple:

          TARGET HELIX
             yellow
               /
             /
           /

      ??????????????
      ?  SUPPORT   ?
      ?   R=50     ?
      ??????????????

centerline radius = 60

Once that is correct, H4 will be the important phase:
we stop displaying only a complete reference helix and
introduce the moving wrapping/contact front, where the orange pipe progressively 
occupies the yellow reference trajectory.

So the flow inside this one function is:

buildTestStretchHelixWrappingInput()
              ?
              ?
           input
              ?
              ?
     validate H1 input
              ?
              ?
StretchHelixWrappingKinematicsBuilder
              ?
              ?
         kinematics
              ?
              ?
       validate H2
              ?
              ?
     store H1/H2 data
              ?
              ?
??????????????????????????????????????
?             H3.5                   ?
?                                    ?
? input.pipeArcLength                ?
? kinematics.curvature               ?
? kinematics.torsion                 ?
?             ?                      ?
?             ?                      ?
? CurvatureTorsionProfile            ?
??????????????????????????????????????
              ?
              ?
         H3.6 frame
              ?
              ?
       H3.7 integration
              ?
              ?
       reference helix
==========================================================================

Phase H4 — introduce the moving wrapping/contact front and progressively occupy
the reference helix with the actual pipe geometry.

That is where the simulation begins to represent the physical
wrapping process rather than only showing the final target helix.

Phase H4 — Moving Wrapping / Contact Front

Now we make the first major change from a reference helix to a forming simulation.

H3 gave us:

YELLOW = complete target helix

       / / / /
      / / / /
     / / / /

H4 adds an actual pipe that progressively occupies this target:

YELLOW = target/reference
ORANGE = currently wrapped pipe


Step 0

ORANGE ???????????????????????????????

YELLOW     / / / / /


Step 1

ORANGE     /???? straight remainder
          /
YELLOW    / / / / /


Step 2

ORANGE    / /
         / /???? straight remainder

YELLOW    / / / / /


Step 3

ORANGE    / / / /
         / / / /????

YELLOW    / / / / /


Step final

ORANGE    / / / / /
YELLOW    / / / / /

The important principle is:

? and ? do NOT gradually increase.

Instead:

?wrapped = ?H2
?wrapped = ?H2

and

wrappedLength grows.

So H4 introduces:

L
wrapped
	?

(t)

and:

s
front
	?

=L
wrapped
	?


For now the contact front starts at material coordinate:

s=0

Later, when Pass 2 consumes a pre-bent pipe, we can
introduce a non-zero contact-start position.

H4.1 — Define the wrapping state

Create:

Core/Forming/StretchHelixWrappingState.h

Use a simple structure:

#pragma once

#include <cmath>

// =====================================================
// STRETCH-HELIX WRAPPING STATE...................

H4.2 — Build the initial state

Create:

Core/Forming/StretchHelixWrappingStateBuilder.h
#pragma once

#include "Core/Forming/StretchHelixWrappingInput.h"
#include "Core/Forming/StretchHelixWrappingState.h"

class StretchHelixWrappingStateBuilder
{.................................

H4.3 — Create the most important H4 builder

Now we need:

StretchHelixWrappingState
            +
H2 kinematics
            ?
?(s), ?(s)

Create:

Core/Forming/StretchHelixCurrentProfileBuilder.h
#pragma once

#include "Core/Geometry/CurvatureTorsionProfile.h"

#include "Core/Forming/StretchHelixWrappingInput.h"
#include "Core/Forming/StretchHelixWrappingKinematics.h"
#include "Core/Forming/StretchHelixWrappingState.h"

class StretchHelixCurrentProfileBuilder
{
public:
    static CurvatureTorsionProfile build(
        const StretchHelixWrappingInput& input,
        const StretchHelixWrappingKinematics& kinematics,
        const StretchHelixWrappingState& state
    );......................

    H4.4 — The profile mathematics

Suppose:

total length   = 500 mm
wrapped length = 150 mm

Then:

s=0                    s=150                         s=500
 |=======================|-----------------------------|
       WRAPPED                  FREE
       
 ? = ?helix                  ? = 0
 ? = ?helix                  ? = 0

So:

?(s)={
?
h
	?

,
0,
	?

0?s<s
f
	?

s>s
f
	?

	?
H4.5 — Implement the current profile builder

Create .cpp:

#include "Core/Forming/StretchHelixCurrentProfileBuilder.h"

#include <cmath>

CurvatureTorsionProfile
StretchHelixCurrentProfileBuilder::build(
    const StretchHelixWrappingInput& input,
    const StretchHelixWrappingKinematics& kinematics,
    const StretchHelixWrappingState& state)
{
    CurvatureTorsionProfile profile;

    profile.clear();..........................

    }

This is the conceptual heart of H4.

H4.6 — Why duplicate samples again?

You already solved this exact problem during Phase 10L.

At:

s = contactFrontS

we need two values:

before front:
    ? = ?helix
    ? = ?helix

after front:
    ? = 0
    ? = 0

So:

sample 1: s=150 ?=0.0162162 ?=0.0027027
sample 2: s=150 ?=0         ?=0

This represents a sharp material boundary.

Your existing:

SpatialCurveIntegrator::sampleProfileAtArcLength()

already has the duplicate-sample behavior we tested in 10L.

That work is now being reused.......................

H4.7 — Store current wrapping state and geometry

In AppController.h, private:

StretchHelixWrappingState
    debugStretchHelixWrappingState;

CurvatureTorsionProfile
    debugStretchHelixCurrentProfile;.................


H4.8 — Initialize H4 after successful H3

After the H3 reference geometry has been successfully built:

debugStretchHelixWrappingState =
    StretchHelixWrappingStateBuilder::buildInitial(
        debugStretchHelixWrappingInput
    );

Check:.....................


H4.9 — Add one geometry rebuild function

This will become very important later.

In AppController.h, private:

bool rebuildDebugStretchHelixCurrentGeometry();

Implementation:

bool AppController::
rebuildDebugStretchHelixCurrentGeometry()
{..............................................
H4.10 — First manual contact-front test

Before creating automatic motion, test fixed percentages.

Add private:

void debugTestStretchHelixContactProgression();

Implementation:

void AppController::............................

H4.11 — Render current pipe orange

In GLView.h, private:

PipeRenderer
    stretchHelixCurrentRenderer;

bool showStretchHelixCurrent =
    true;

    H4.12 — Drawing order

In paintGL():

// Yellow target.
drawStretchHelixReference();

// Orange actual/current geometry.
drawStretchHelixCurrent();.........................
H4.13 — Add manual step control

Now make it interactive.

In AppController.h, public:

void advanceDebugStretchHelixWrapping(
    double deltaWrappedLength
);

void resetDebugStretchHelixWrapping();


H4.14 — Temporary keys

Use free keys of your choice.

For example:

H = advance wrapping
J = reset wrapping

If free:

case Qt::Key_H:
{
    std::cout
        << "[KEY] H - ADVANCE STRETCH HELIX WRAP\n";

    controller.advanceDebugStretchHelixWrapping(
        25.0
    );

    break;
}

case Qt::Key_J:.......................
Important limitation of H4

H4 is deliberately a kinematic moving-front model.

It assumes:

behind contact front:
    pipe exactly follows H2 helix

ahead of contact front:
    pipe is tangent/free

at front:
    instantaneous transition

Real contact will have a finite transition zone due to:

bending stiffness
contact pressure
friction
roller/support geometry
tension

Do not solve that yet.

First verify that the basic physical concept is represented correctly:

fixed target helix
+
moving material contact front
+
wrapped region grows

That is the foundation we need before adding machine-time 
synchronization in H5.


==========================
Phase H5 — 
wrapped part:
follows reference helix exactly

free part:
straight line starting at front
and aligned with tangent of reference helix

This is a better model for the forming principle.

So I would make this the first task of Phase H5, before
adding the visible support cylinder:

H5.1
Find the front point on the reference helix.

H5.2
Read its tangent frame.

H5.3
Use reference-helix nodes only up to frontS.

H5.4
Append a straight tangent tail from the front.

H5.5
Verify that the tail no longer appears to rotate incorrectly 
around the support.

The key architectural change is subtle but important:

H4:
current geometry derived only from ?(s), ?(s)

H5:
wrapped part derived from reference helix
+
free part derived explicitly from front tangent

That will make the animation much closer to the actual pipe approaching
and wrapping onto the helical path.
Phase H5 — Make the Free Pipe Tangent to the Moving Helix Contact Front

H4 proved the moving-front concept. H5 now corrects the kinematics of the unwrapped straight section.

The target behavior is:

REFERENCE HELIX — yellow

        / / / / / /
       / / / / / /
      / / / / / /


CURRENT PIPE — orange

        / / /
       / / *
          / \
         /   \
        /     \???????????? straight free pipe
              ^
              tangent direction at contact front

The essential rule becomes:

0 ... frontS
    copy the reference helix

frontS ... pipe end
    straight line tangent to the reference helix
    at frontS

This is more physical than asking the curvature/torsion integrator to generate the entire current pipe.

H5.1 — Change the conceptual ownership

H4 used:

WrappingState
     ?
?(s), ?(s)
     ?
SpatialCurveIntegrator
     ?
whole current pipe

For H5 we use:

             H3 reference helix
                    ?
                    ? find front
                    ?
             reference node
                    ?
           ???????????????????
           ?                 ?
           ?                 ?
 wrapped section          free section
 copy reference         tangent straight
           ?                 ?
           ???????????????????
                    ?
             CURRENT PIPE

Do not delete the H4 profile builder. It remains a useful mathematical/debug test.

H5 introduces a second, more physical geometry-construction path.

H5.2 — Store explicit current geometry

In AppController.h, private:

std::vector<PipeNode>
    debugStretchHelixContactGeometryNodes;

Public getter:..............

H5.3 — Add the H5 geometry builder

In AppController.h, private:

bool rebuildDebugStretchHelixContactGeometry();

This function will use:

debugStretchHelixReferenceResult
debugStretchHelixWrappingState.............
H5.4 — Find the moving front node

We know:

reference length = 500 mm
reference nodes  = 2001
sample step      = 0.25 mm

Therefore the reference geometry is uniformly sampled.

For now, calculate the front index using normalized arc length:

const std::vector<PipeNode>& referenceNodes =
    debugStretchHelixReferenceResult.nodes;

const double totalLength =
    debugStretchHelixWrappingInput.pipeArcLength;

const double frontS =
    debugStretchHelixWrappingState.contactFrontS;.....................

H5.5 — Derive the tangent from the reference helix

We don't need to rely on a particular PipeNode frame member.

Use neighboring positions.

For an interior node:

Vec3D tangent =
    referenceNodes[frontIndex + 1].pos
    - referenceNodes[frontIndex - 1].pos;...
    H5.6 — Copy the wrapped part

Start the H5 rebuild function:

bool AppController::
rebuildDebugStretchHelixContactGeometry()
{
    debugStretchHelixContactGeometryNodes.clear();...

H5.2 — Store explicit current geometry

In AppController.h, private:

std::vector<PipeNode>
    debugStretchHelixContactGeometryNodes;

Public getter:

const std::vector<PipeNode>&
getDebugStretchHelixContactGeometryNodes() const;

You may need the header containing PipeNode.

Implementation:..................
H5.3 — Add the H5 geometry builder

In AppController.h, private:

bool rebuildDebugStretchHelixContactGeometry();

This function will use:

debugStretchHelixReferenceResult
debugStretchHelixWrappingState

It does not call SpatialCurveIntegrator.

H5.4 — Find the moving front no.....................
H5.4 — Find the moving front node

We know:

reference length = 500 mm
reference nodes  = 2001
sample step      = 0.25 mm

Therefore the reference geometry is uniformly sampled.

For now, calculate the front index using normalized arc length:

const std::vector<PipeNode>& referenceNodes =
    debugStretchHelixReferenceResult.nodes;

const double totalLength =
    debugStretchHelixWrappingInput.pipeArcLength;

const double frontS =
    debugStretchHelixWrappingState.contactFrontS;

const double normalizedFront =
    std::clamp(
        frontS / totalLength,
        0.0,
        1.0
    );

const std::size_t lastIndex =
    referenceNodes.size() - 1;

const std::size_t frontIndex =
    static_cast<std::size_t>(
        std::llround(
            normalizedFront
            * static_cast<double>(lastIndex)
        )
    );

For your current test this produces approximately:

frontS =   0 ? index    0
frontS =  25 ? index  100
frontS = 125 ? index  500
frontS = 250 ? index 1000
frontS = 500 ? index 2000


H5.5 — Derive the tangent from the reference helix

We don't need to rely on a particular PipeNode frame member.

Use neighboring positions.

For an interior node:

Vec3D tangent =
    referenceNodes[frontIndex + 1].pos
    - referenceNodes[frontIndex - 1].pos;

tangent.normalize();

However, adapt normalize() to your actual Vec3D API.

If your Vec3D does not have .normalize(), use your existing normalization helper.

A complete boundary-safe pattern:

Vec3D tangent;....................
H5.6 — Copy the wrapped part

Start the H5 rebuild function:

bool AppController::
rebuildDebugStretchHelixContactGeometry()
{
    debugStretchHelixContactGeometryNodes.clear();

    if (!debugStretchHelixReferenceResult.valid)
        return false;

    if (!debugStretchHelixReferenceResult.isComplete())
        return false;

    const std::vector<PipeNode>& referenceNodes =
        debugStretchHelixReferenceResult.nodes;

    if (referenceNodes.size() < 2)
        return false;

    const double totalLength =
        debugStretchHelixWrappingInput.pipeArcLength;.......
At this point:

orange wrapped part
=
yellow helix exactly

H5.7 — Find the tangent

Continue in the same function:

    Vec3D tangent;

    if (frontIndex == 0)
    {
        tangent =
            referenceNodes[1].pos
            - referenceNodes[0].pos;
    }
    else if (.............

Do not invent another vector class.

H5.8 — Generate the straight free tail

The front point:

const Vec3D frontPosition =
    referenceNodes[frontIndex].pos;

The remaining material length:

const double remainingLength =
    totalLength
    - frontS;

Now append nodes along:

P(s)=P
front
	?

+sT
front
	?


where:

0 ? s ? remainingLength

Use the same sample density as the reference geometry.

Continue:

    const Vec3D frontPosition =
        referenceNodes[frontIndex].pos;

    const double remainingLength =
        totalLength
        - frontS;

    const std::size_t remainingNodeCount =
        lastIndex
        - frontIndex;

    if (remainingNodeCount > 0)
    {
        for (std::size_t j = 1;
             j <= remainingNodeCount;
             ++j)
        {
            const double localFraction =
                static_cast<double>(j)
                / static_cast<double>(
                    remainingNodeCount
                );

            const double localLength =
                remainingLength
                * localFraction;

            PipeNode node =
                referenceNodes[frontIndex];

            node.pos =
                frontPosition
                + tangent
                * localLength;

            debugStretchHelixContactGeometryNodes.push_back(
                node
            );
        }
    }

Using:

PipeNode node =
    referenceNodes[frontIndex];

is intentional.

It preserves whatever other metadata your PipeNode currently carries while we explicitly replace its position.

For H5 rendering, position is what matters.

H5.9 — Complete the builder

Finish:

    const bool accepted =
        debugStretchHelixContactGeometryNodes.size()
        == referenceNodes.size();

    std::cout
        << "[STRETCH HELIX CONTACT GEOMETRY]"
        << " frontS="
        << frontS
        << " frontIndex="
        << frontIndex
        << " wrappedNodes="
        << (
            frontIndex + 1
        )
        << " totalNodes="
        << debugStretchHelixContactGeometryNodes.size()
        << " accepted="
        << accepted
        << std::endl;

    return accepted;
}

So the complete node count should remain:

2001

throughout the entire animation.

H5.10 — Special cases

This algorithm naturally produces sensible endpoints.

At reset
frontS = 0

Only the first reference point is copied.

Then nearly the entire pipe is constructed as a straight tangent:

*
 \____________________________________
At 50%
frontS = 250

approximately:

half helix
~~~~~~~~~~~~*
             \
              \____________________
At completion
frontS = 500

all reference nodes are copied:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

No straight tail remains.

That is precisely our desired H5 behavior.

H5.11 — Call H5 whenever H4 state changes

Inside:

advanceDebugStretchHelixWrapping()

after changing:

wrappedLength
contactFrontS
progress
complete

currently you probably call:

rebuildDebugStretchHelixCurrentGeometry();

Keep that H4 diagnostic if you want.

Then add:

const bool contactGeometryValid =
    rebuildDebugStretchHelixContactGeometry();

Your diagnostic can become:

std::cout
    << "[STRETCH HELIX WRAP STEP]"
    << " wrappedLength="
    << debugStretchHelixWrappingState.wrappedLength
    << " frontS="
    << debugStretchHelixWrappingState.contactFrontS
    << " progress="
    << debugStretchHelixWrappingState.progress
    << " complete="
    << debugStretchHelixWrappingState.complete
    << " profileGeometryValid="
    << debugStretchHelixCurrentResult.valid
    << " contactGeometryValid="
    << contactGeometryValid
    << std::endl;
H5.12 — Update reset

Inside:

resetDebugStretchHelixWrapping()

after rebuilding the state, call:

rebuildDebugStretchHelixCurrentGeometry();

rebuildDebugStretchHelixContactGeometry();

Now J should immediately restore the straight tangent pipe.

H5.13 — Render H5 instead of H4 orange geometry

This is important.

Keep the yellow H3 reference:

drawStretchHelixReference();

But for orange current pipe, render:

debugStretchHelixContactGeometryNodes

instead of:

debugStretchHelixCurrentResult.nodes

Update the current getter/render path accordingly.

For example, in GLView::drawStretchHelixCurrent():

const std::vector<PipeNode>& nodes =
    app->getDebugStretchHelixContactGeometryNodes();

if (nodes.size() < 2)
    return;

Then use the same orange rendering:

shader->setVec3(
    "pipeColor",
    glm::vec3(
        1.0f,
        0.45f,
        0.05f
    )
);

LINE:

drawStretchDebugLine(
    nodes,
    stretchHelixCurrentRenderer,
    3.0f
);

MESH:

drawStretchDebugTube(
    nodes,
    stretchHelixCurrentRenderer,
    1.2,
    16
);

This preserves your standard visualization infrastructure.

H5.14 — Add a front marker later, not now

Don't add another HUD section.

If needed, later we can render one small marker:

yellow = target
orange = pipe
red    = moving contact point

But first verify the geometry itself.

H5 acceptance test

Press:

J

Expected:

orange pipe:
almost completely straight

yellow:
complete static helix

Then:

H
H
H
...

Expected physical progression:

STEP 1

  yellow target:
       ~~~~~~~~~~~~~~~~~~~~~~~~~~~

  orange:
       ~*?????????????????????????
        ?
       tangent front


STEP 2

       ~~~~~~*????????????????????
              ?


STEP 3

       ~~~~~~~~~~~~*??????????????
                    ?


COMPLETE

       ~~~~~~~~~~~~~~~~~~~~~~~~~~~

The critical acceptance conditions are:

? wrapped orange section overlaps yellow reference

? free orange section is perfectly straight

? straight section starts exactly at contact front

? straight section is tangent to yellow helix

? there is no visible corner at the front

? front travels along yellow reference

? yellow reference never moves

? orange total pipe length stays approximately 500 mm

? completion makes orange and yellow coincide

? LINE and MESH both work

One subtle point: the straight free section will still change
its spatial orientation as the contact front travels around the 
cylinder, because the tangent of a helix rotates around the support. 
That rotation is physically expected. What H5 fixes is that the free
pipe is now explicitly tied to the reference-helix tangent at the
contact point, instead of being an incidental consequence of integrating a
piecewise curvature profile.

====================================


Phase H6 — Synchronize Wrapping with Machine Time


Yes. After H6, the straight free section will still change its spatial orientation as the contact front moves around the support.

That is physically expected in the model we are building, because the free section is always tangent to the reference helix at the current contact point:

                   free straight pipe
                          \
                           \
                            \
                             *  contact front
                           / /
                         / /
                       / /
                 reference helix

As the front advances around the cylinder, the helix tangent itself rotates in 3D. Therefore the straight free section rotates with that tangent.

H6 will change how fast the front moves, not this geometric rule:

H5:
front position chosen manually
? free pipe tangent follows that position

H6:
front position calculated from machine time
? free pipe tangent still follows that position

So H6 becomes approximately:

rotation speed ?
axial speed vz
time dt
      ?
      ?
machine motion
      ?
      ?
contactFrontS(t)
      ?
      ?
reference helix point + tangent
      ?
      ??? wrapped section
      ?      follows helix
      ?
      ??? free section
             remains straight
             and tangent at front

One subtle point will matter later: whether the whole incoming
free pipe physically rotates around the support axis, or whether
machine guides/clamps constrain its position while the support 
rotates underneath it. That depends on the real
machine arrangement.

For H6, I recommend preserving the current H5 rule—straight and 
tangent at the moving front—because it is the clean kinematic foundation.
Later we can add guide/clamp constraints if the actual machine keeps
the incoming pipe in a fixed spatial direction.

Phase H6 — Synchronize Wrapping with Machine Time

Now we replace the manual rule:

press H
? add 25 mm wrapped length

with:

time advances
? machine rotates
? machine moves axially
? contact front advances
? wrapped length grows automatically

H6 should still reuse the H5 geometry exactly as it is.

The new flow is:

dt
?
??? axialSpeed
??? rotationSpeed
?
?
machine travel during dt
?
?
wrappedLength increment
?
?
contactFrontS
?
?
H5 tangent/contact geometry
H6.1 — First derive material speed along the helix

Your H2 helix has:

r = centerline radius
b = rise per radian
? = rotation speed

For one radian of support rotation:

circumferential travel = r
axial travel           = b

So centerline travel per radian is:

r
2
+b
2
	?


Therefore wrapped centerline speed is:..................

his is the quantity H6 should use to advance wrappedLength.

For your current case:

r  = 60 mm
?  = 2 rad/s
vz = 20 mm/s

approximately:

circumferential speed = 120 mm/s
axial speed           = 20 mm/s

wrapped speed
? sqrt(120? + 20?)
? 121.66 mm/s

So a 500 mm pipe would wrap in roughly:

500 / 121.66 ? 4.11 s..................
H6.2 — Add a wrapping speed helper

Create:

Core/Forming/StretchHelixWrappingKinematicsUtils.h

or, simpler for now, add a function to the existing H2 kinematics type:

double centerlineSpeed =
    0.0;

I prefer adding it to StretchHelixWrappingKinematics.

In:

StretchHelixWrappingKinematics

add:

// Workpiece centerline travel speed along the
// reference helix.
//
// Units:
//     mm / s
double centerlineSpeed =
    0.0;

Also clear it:

centerlineSpeed =
    0.0;
H6.3 — Compute it inside H2 builder

Inside:

StretchHelixWrappingKinematicsBuilder::build()

after:

const double r =
    result.centerlineRadius;

const double b =
    result.helixRisePerRadian;

add:

result.centerlineSpeed =
    absoluteRotationSpeed
    * std::sqrt(
        r * r
        + b * b
    );

Equivalent check:

const double alternativeSpeed =
    std::sqrt(
        (
            r
            * absoluteRotationSpeed
        )
        * (
            r
            * absoluteRotationSpeed
        )
        +
        input.axialSpeed
        * input.axialSpeed
    );

These should match.

H6.4 — Add speed diagnostic

Extend your H2 log:

<< " centerlineSpeed="
<< kinematics.centerlineSpeed

Expected approximately:

centerlineSpeed=121.655

Then add acceptance:

const double expectedCenterlineSpeed =
    std::sqrt(
        std::pow(
            kinematics.centerlineRadius
            * std::abs(................
Add to diagnostic:

<< " speedAccepted="
<< centerlineSpeedAccepted
H6.5 — Add elapsed time to wrapping state

In:

StretchHelixWrappingState

add:

// Process time since wrapping started.
//
// Units:
//     seconds
double elapsedTime =
    0.0;

In clear():

elapsedTime =
    0.0;

In buildInitial():

state.elapsedTime =
    0.0;

Also validate it:

if (!std::isfinite(elapsedTime)
    || elapsedTime < 0.0)
{
    return false;
}
H6.6 — Create a state advancer

Create:

Core/Forming/StretchHelixWrappingStateAdvancer.h
#pragma once

#include "Core/Forming/StretchHelixWrappingInput.h"
#include "Core/Forming/StretchHelixWrappingKinematics.h"
#include "Core/Forming/StretchHelixWrappingState.h"

class StretchHelixWrappingStateAdvancer
{
public:
    static void advance(
        StretchHelixWrappingState& state,
        double dt,
        const StretchHelixWrappingInput& input,
        const StretchHelixWrappingKinematics& kinematics..............
Implementation:

#include "Core/Forming/StretchHelixWrappingStateAdvancer.h"

#include <algorithm>
#include <cmath>

void StretchHelixWrappingStateAdvancer::advance(
    StretchHelixWrappingState& state,
    double dt,
    const StretchHelixWrappingInput& input,
    const StretchHelixWrappingKinematics& kinematics)
{.......................


H6.7 — Add AppController time-based advance

In AppController.h, public:

void advanceDebugStretchHelixWrappingTime(
    double dt
);

Implementation:

void AppController::
advanceDebugStretchHelixWrappingTime(
    double dt)
{
    if (!debugStretchHelixWrappingState.valid)
        return;

    StretchHelixWrappingStateAdvancer::advance(
        debugStretchHelixWrappingState,
        dt,
        debugStretchHelixWrappingInput,
        debugStretchHelixWrappingKinematics
    );

    const bool contactGeometryValid =
        rebuildDebugStretchHelixContactGeometry();

    std::cout
        << "[STRETCH HELIX TIME STEP]"
        << " dt="
        << dt
        << " elapsedTime="
        << debugStretchHelixWrappingState.elapsedTime
        << " wrappedLength="
        << debugStretchHelixWrappingState.wrappedLength
        << " frontS="
        << debugStretchHelixWrappingState.contactFrontS
        << " progress="
        << debugStretchHelixWrappingState.progress
        << " complete="
        << debugStretchHelixWrappingState.complete
        << " geometryValid="
        << contactGeometryValid
        << std::endl;
}

Notice: H6 should rebuild the H5 contact geometry,
not rely on the old H4 profile geometry.

H6.8 — Reset must reset time too

Your existing:

resetDebugStretchHelixWrapping()

already rebuilds initial state.

Because buildInitial() now sets:

elapsedTime = 0.0;

reset automatically becomes:

wrappedLength = 0
frontS        = 0
elapsedTime   = 0

Update the diagnostic:

std::cout
    << "[STRETCH HELIX WRAP RESET]"
    << " wrappedLength="
    << debugStretchHelixWrappingState.wrappedLength
    << " frontS="
    << debugStretchHelixWrappingState.contactFrontS
    << " time="
    << debugStretchHelixWrappingState.elapsedTime
    << std::endl;................................

H6.9 — Temporary manual time-step key

Keep H, but change what it means.

Instead of:

controller.advanceDebugStretchHelixWrapping(
    25.0
);

use:

controller.advanceDebugStretchHelixWrappingTime(
    0.25
);

Now:

H = advance machine by 0.25 seconds

For your current speed:

121.66 mm/s × 0.25 s
? 30.4 mm

So each H press should advance the front by about:

30.4 mm

not exactly 25 mm anymore.

That is the first visible proof that H6 is machine-driven.

H6.10 — Expected log

After reset:

[STRETCH HELIX WRAP RESET]
wrappedLength=0
frontS=0
time=0

First H:

[STRETCH HELIX TIME STEP]
dt=0.25
elapsedTime=0.25
wrappedLength?30.41
frontS?30.41
progress?0.0608
complete=0
geometryValid=1

Second:

elapsedTime=0.5
wrappedLength?60.83
progress?0.1217

And so on.

Around 4.1 seconds:

..................................

H6.11 — Add completion guard

At the start of:

advanceDebugStretchHelixWrappingTime()

add:

if (debugStretchHelixWrappingState.complete)
{
    std::cout
        << "[STRETCH HELIX TIME STEP]"
        << " ignored=1"
        << " reason=AlreadyComplete"
        << std::endl;

    return;
}

This mirrors the guard from your previous stretch playback system.

H6.12 — Add a time-consistency acceptance test

Add a debug test:

void AppController::
debugTestStretchHelixWrappingTimeProgression()

Use:

StretchHelixWrappingState state =
    StretchHelixWrappingStateBuilder::buildInitial(
        debugStretchHelixWrappingInput
    );

Then choose:

const double dt =
    1.0;

Advance once:

StretchHelixWrappingStateAdvancer::advance(
    state,
    dt,
    debugStretchHelixWrappingInput,
    debugStretchHelixWrappingKinematics
);

Expected:

const double expectedLength =
    std::min(
        debugStretchHelixWrappingInput.pipeArcLength,
        debugStretchHelixWrappingKinematics.centerlineSpeed
            * dt
    );

Acceptance:

const bool accepted =
    std::abs(
        state.wrappedLength
        - expectedLength
    ) <= 1e-9;

Diagnostic:

std::cout
    << "[STRETCH HELIX TIME ACCEPTANCE]"
    << " dt="
    << dt
    << " expectedLength="
    << expectedLength
    << " actualLength="
    << state.wrappedLength
    << " accepted="
    << accepted
    << std::endl;
H6.13 — What should visually change?

The geometry rule does not change from H5:

wrapped section
    follows yellow reference helix

free section
    stays straight
    tangent at moving contact front

What changes is the progression law.

Before H6:

front += arbitrary 25 mm

After H6:

front += centerlineSpeed × dt

So:

machine commands
        ?
time
        ?
physical travel distance
        ?
contact front
        ?
H5 geometry

That is the first real connection between your simulated
machine motion and the visible forming process.

H6 acceptance

Proceed only when:

? centerlineSpeed derived correctly
? elapsedTime advances correctly
? wrappedLength = centerlineSpeed × time
? contactFrontS follows wrappedLength
? progress reaches 1
? geometry remains valid
? yellow reference stays fixed
? orange free section stays tangent
? completion occurs around the expected process time
? stepping after Complete is ignored

After H6, the next logical phase is H7 — automatic timed
playback using real frame dt, plus pause/resume and machine-speed controls
for axial speed and rotation speed.