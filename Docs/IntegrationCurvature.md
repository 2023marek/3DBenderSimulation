Excellent. This is the point where the mathematics 
behind the code becomes much clearer.

The key idea is:

Curvature is not the position of the pipe.

Curvature tells us how fast the tangent vecto
rotates as we move along the pipe.

Let's build it step by step.

1. Arc-length parameter

Instead of using time, we move along the pipe.

The independent variable is

s

where

0?s?L

and

s = arc length
L = total pipe length
2. Curvature definition

The tangent vector is

T(s)

Curvature is defined as

ds
dT
	?

=?(s)N(s)
	?


This equation literally says

While moving one millimeter along the pipe,
the tangent rotates by an amount equal to the curvature.

3. Numerical integration

Our code cannot integrate continuously.

Instead it advances by small steps

ds

For one step,

T
i+1
	?

=T
i
	?

+?
i
	?

N
i
	?

ds
	?


This is the simplest Euler integration.

Then we normalize

T
i+1
	?

?
?T
i+1
	?

?
T
i+1
	?

	?


because the tangent must always have length 1.

4. Position integration

Once the tangent is known,

the position advances by

ds
dP
	?

=T
	?


Numerically,

P
i+1
	?

=P
i
	?

+T
i
	?

ds
	?


This equation is the one that actually generates
the centerline of the pipe.

5. Together

Every integration step performs

T
i+1
	?

=T
i
	?

+?
i
	?

N
i
	?

ds
	?


then

P
i+1
	?

=P
i
	?

+T
i
	?

ds
	?


This repeats until

s=L.
6. What happens when curvature is constant?

Suppose

?(s)=0.002

for the whole pipe.

Then every step rotates the tangent by exactly

??=?ds

For example,

ds=0.25 mm

gives

??=0.002×0.25=0.0005 rad

Each step rotates the tangent by only

0.0005 radians.

Thousands of these tiny rotations gradually form a circular arc.

7. Total bend angle

Since every step rotates by

d?=?ds,

the total rotation is

?=?
0
L
	?

?(s)ds
	?


For constant curvature,

?=?L
	?


Example:

L=200 mm
?=0.002

Then

?=0.002×200=0.4 rad

or about

22.9
?
.

This is exactly the bend angle that the generated pipe will have.

8. Where this appears in our code

The mathematical equations correspond almost one-to-one
with the integration loop:

while (s < totalLength)
{
    ? = profile.curvature(s)
    ? = profile.torsion(s)

    // Rotate local frame
    T += ? N ds
    N += ...
    B += ...

    normalize(T,N,B)

    // Advance centerline
    P += T ds

    // Store node
    nodes.push_back(...)
}
ASCII picture
                 curvature ?

                      ?
                      ?
          tangent rotates a little
                      ?

T0 ??????

            T1 ??????

                 T2 ??????

                      T3 ??????


The position follows the tangent:

P0 ????????????????????????????
     ds       ds       ds

This is why the class is called SpatialCurveIntegrator:
it integrates the differential equations of the Frenet–Serret frame
. Starting from one initial frame (P, T, N, B) and the functions ?(s) and ?(s), it reconstructs the entire 3D curve. In our implementation we use a simple numerical (Euler-style) integration, which is why reducing ds from 0.25 mm to 0.125 mm improved the accuracy—the approximation becomes closer to the continuous
mathematical solution.




=========================================================
Target curvature versus loaded curvature
=========================================================
The important distinction is that the evaluator handles process commands, while the profile builders convert those commands into functions of arc length.

1. Target curvature versus loaded curvature
Target curvature
target curvature

means the curvature you want the pipe to have after the machine releases it.

For example:

targetCurvature = 0.002 1/mm

corresponds to a target radius:

R = 1 / ?
  = 1 / 0.002
  = 500 mm

So the desired finished pipe is:

final radius = 500 mm
Loaded curvature

While the machine is still applying force, the pipe must usually be bent more strongly because it will partially recover during unloading.

In your simplified model:

loaded curvature > target curvature

For the values already tested:

target curvature = 0.002

springback ratio = 0.10

loaded curvature = 0.00222222

The relationship is approximately:

loaded? =
    target?
    / (1 - springbackRatio)

Therefore:

loaded? =
    0.002
    / 0.9

loaded? =
    0.00222222

ASCII:

machine bends more strongly
          ?
loaded ? = 0.00222222
          ? release force
elastic recovery / springback
          ?
final ? = 0.002

Because larger curvature means a smaller radius:

loaded shape:
R = 1 / 0.00222222
  ? 450 mm

final shape:
R = 1 / 0.002
  = 500 mm

So the loaded pipe is tighter than the final pipe.

2. How is final curvature predicted?

Your current model predicts it using the configured springback ratio:

predictedFinal? =
    loaded?
    × (1 - springbackRatio)

With the test values:

predictedFinal? =
    0.00222222 × 0.90

predictedFinal? =
    0.002

This is currently a simplified proportional model.

It does not yet simulate:

nonlinear material unloading,
residual stress distribution,
changing plastic zones,
local differences along the pipe,
detailed finite-element mechanics.

It says only:

release removes a chosen percentage
of the loaded curvature

The model is useful for architecture and process playback, but it is not yet a full physical springback solver.

3. Is curvature changing with ds?

Not necessarily.

ds is the small integration step along the pipe:

s = arc length
ds = small arc-length increment

The integrator repeatedly asks:

what are ? and ? at the current s?

Then it advances the frame by ds.

Constant profile

If curvature and torsion are constant:

?(s) = constant
?(s) = constant

then every integration step receives the same values.

For example:

s = 0.00    ? = 0.002    ? = 0
s = 0.25    ? = 0.002    ? = 0
s = 0.50    ? = 0.002    ? = 0
s = 0.75    ? = 0.002    ? = 0
...

The curvature is not changed by ds.

ds only controls how finely the geometry is calculated.

profile:
? and ? instructions

ds:
numerical resolution

Smaller ds:

more steps
more nodes
usually better numerical accuracy

Larger ds:

fewer steps
fewer nodes
usually lower numerical accuracy
4. What shape results from constant curvature and torsion?
Constant curvature, zero torsion
? = constant
? = 0

produces a planar circular arc.

constant ?
zero ?
    ?
circle or circular arc
Constant curvature, constant nonzero torsion
? = constant
? = constant and nonzero

produces a circular helix.

constant ?
constant nonzero ?
        ?
3D helix
Both zero
? = 0
? = 0

produces a straight line.

? = 0
? = 0
    ?
straight pipe
5. Why do we not calculate loaded torsion?

Yes—currently this is mainly a simplification.

Your springback compensation model acts only on curvature:

target curvature
    ? compensation
loaded curvature
    ? unloading prediction
final curvature

Torsion currently passes through unchanged:

loaded torsion = target torsion

predicted final torsion = target torsion

In the present stretch-bending test:

input.geometry.targetTorsion =
    0.0;

Therefore:

loaded ? = 0
final ? = 0

There is no visible problem because the stretch-bending shape is planar.

A more complete future model could introduce:

targetFinalTorsion

loadedTorsionCommand

predictedFinalTorsion

torsionalSpringbackRatio

For example:

loaded? =
    target?
    / (1 - torsionalSpringbackRatio)

But that would require us to define what physical machine action creates torsion and how the material recovers torsionally. We have not modeled that yet.

So the current deliberate boundary is:

curvature springback:
modeled

torsional springback:
not modeled yet
Updated process diagram
StretchBendingProcessInput
        ?
        ? targetFinalCurvature
        ? targetTorsion
        ? springbackRatio
       
StretchBendingEvaluator
        ?
        ??? checks feasibility
        ?
        ??? calculates loaded curvature command
        ?
        ?       loaded? =
        ?       target? / (1 - springback ratio)
        ?
        ??? predicts final curvature
        ?
        ?       final? =
        ?       loaded? × (1 - springback ratio)
        ?
        ??? currently leaves torsion unchanged
                loaded? = target?
                final?  = target?
       
StretchBendingEvaluationResult
        ?
        ?????????????????????????????????
        ?                               ?
       
StretchBendingProfileBuilder     FinalProfileBuilder
        ?                               ?
        ? uses loaded?                  ? uses predictedFinal?
        ? uses target?                  ? uses target?
       
loadedProfile                    finalProfile
        ?                               ?
        ? ?_loaded(s), ?_loaded(s)      ? ?_final(s), ?_final(s)
        ?????????????????????????????????
                       
             SpatialCurveIntegrator
                       ?
                       ? steps through arc length:
                       ? s = 0, ds, 2ds, 3ds...
                       ?
                       ? samples ?(s), ?(s)
                       ? updates P, T, N, B
                      
          loaded nodes / final nodes
                       ?
                      
                    renderer

The simplest mental model is:

Target curvature:
what finished pipe should have.

Loaded curvature:
what machine must temporarily create.

Predicted final curvature:
what remains after modeled springback.

ds:
how finely we calculate the shape,
not how curvature itself changes.


=======================================================
Integration meaning.
========================================================

Yes — let’s step back and simplify.

1. What does “integration” mean here?

In our code, integration means mathematical integration, not “merging.”

We start with:

curvature ?(s)
torsion   ?(s)

defined along pipe arc length s.

The integrator converts those values into actual geometry:

position P(s)
tangent  T(s)
normal   N(s)
binormal B(s)

So conceptually:

?(s), ?(s)
    ? mathematical integration
3D pipe centerline + local frames

It is similar to solving a differential equation step by step.

2. Is this loop the core calculation?

Yes.

while (currentArcLength
    < profile.totalArcLength - 1e-12)
{
    // sample ? and ?
    // advance frame
    // append node and arc length
}

This is the main integration loop.

Its logic is:

current arc-length position
        ?
read curvature and torsion
        ?
calculate how frame changes over small step
        ?
move position forward
        ?
store new pipe node
        ?
repeat

The real mathematical work is inside:

// advance frame

That part updates:

P
T
N
B

using the current:

?
?
step size ds
3. Do we render the nodes produced by integration?

Yes.

The integrator produces:

SpatialCurveIntegrationResult

which contains something like:

result.nodes

Each node contains pipe geometry information, for example:

position
tangent
normal
binormal
arc length

Rendering then uses those nodes.

For LINE rendering:

node positions
    ?
connected line segments

For MESH rendering:

node position + local frame
    ?
build circular cross-section at each node
    ?
connect neighboring rings
    ?
tube mesh

So:

integration nodes
        ?
renderer
        ?
visible pipe
4. What is a profile builder?

A profile builder creates the curvature/torsion instructions for the integrator.

It does not create final 3D geometry.

For stretch bending:

StretchBendingProfileBuilder

uses:

process input
evaluation result

and creates:

CurvatureTorsionProfile

For example:

arc length = 200 mm
curvature  = 0.00222222
torsion    = 0

The builder answers:

“What curvature and torsion should exist
at each position along the pipe?”

The integrator answers:

“What actual 3D shape results from that profile?”
ASCII architecture diagram
StretchBendingProcessInput
        ?
        ? target length
        ? target curvature
        ? target torsion
        ? springback settings
       
StretchBendingEvaluator
        ?
        ? checks feasibility
        ? calculates loaded curvature
        ? predicts final curvature
        
StretchBendingEvaluationResult
        ?
        ?????????????????????????????????
        ?                               ?
                                   
StretchBendingProfileBuilder     FinalProfileBuilder
        ?                               ?
        ? loaded ? / ? profile          ? final ? / ? profile
      
loadedProfile                    finalProfile
        ?                               ?
        ?????????????????????????????????
                     
             SpatialCurveIntegrator
                       ?
                       ? mathematical integration
                       ? step by step along s
                      
        SpatialCurveIntegrationResult
                       ?
                       ? nodes:
                       ? P, T, N, B, arc length
                       ?
                    Renderer
                       ?
             ?????????????????????
           
         LINE mode           MESH mode
       connect points      build tube rings
Pipe-flow version
PROCESS PARAMETERS
        ?
EVALUATION
        ?
CURVATURE / TORSION PROFILE
        ?
MATHEMATICAL INTEGRATION
        ?
PIPE NODES
        ?
LINE OR TUBE RENDERING

A compact mental model is:

Builder says how the pipe should bend.

Integrator calculates where the pipe goes.

Renderer draws the calculated nodes.



========================================================
ebugTestStretchBendingGeometry()
========================================================
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

State meaning

The three fractions have different meanings.

tensionFraction
    controls axial stretching load

bendingFraction
    controls progress toward loaded curvature

unloadingFraction
    interpolates loaded shape toward final shape
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





AppController
    |
    +-- debugStretchLoadedIntegrationResult
    |
    +-- debugStretchFinalIntegrationResult
    |
    +-- debugStretchManufacturingState


debugTestStretchBendingGeometry()
        ?
        ??? creates process input
        ??? evaluates feasibility
        ??? builds loaded profile
        ??? builds final profile
        ??? integrates loaded geometry
        ??? integrates final geometry
        ??? defines fixed active zone
        ??? stores manufacturing state



debugTestStretchBendingGeometry()
        ?
        ??? loadedProfile
        ??? finalProfile
        ??? activeZone
        ?
        ??? stores:
            debugStretchLoadedIntegrationResult
            debugStretchFinalIntegrationResult
            debugStretchManufacturingState