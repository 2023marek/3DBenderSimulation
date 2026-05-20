#pragma once
#include <glm/glm.hpp> 
#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>
#include "Core/Math/Vec3D.h"


#ifndef PI
#define PI 3.14159265358979323846
#endif


// FUTURE:
// PipeAxis3D -> GeometricPipeSimulator

// =========================================================================
// PHASE 5: CNC-COMPLIANT PIPE AXIS 3D
// =========================================================================
//
// KEY PRINCIPLE:
//   "Never mutate geometry directly. Store operations, rebuild from scratch."
//
// This is the industry-standard approach used in professional CNC software:
//   ? Deterministic (same operations always produce same geometry)
//   ? Editable (change any operation, rebuild automatically)
//   ? Realistic (matches real CNC machine behavior)
//   ? Scalable (handles unlimited operations)
//
// =========================================================================

class PipeAxis3D
{
public:
    // =====================================================================
    // FRAME (3D coordinate system along pipe)
    // =====================================================================
    struct Frame
    {
        Vec3D P;  // Position
        Vec3D T;  // Tangent (direction pipe is pointing)
        Vec3D N;  // Normal (perpendicular to tangent)
        Vec3D B;  // Binormal (perpendicular to both)
    };
    //======================================================================
    // OPERATION (User input for pipe construction)
    // INPUT LAYER 
    // =======================================================================
    // 
    struct Operation
    {
        enum Type
        {
            FEED,
            BEND,
            ROTATE
        };

        Type type = FEED;

        double length = 0.0;
        double R = 0.0;
        double angle = 0.0;
        double rotAngle = 0.0;
    };
    // =====================================================================
    // NODE (Point on pipe centerline)
    // =====================================================================
    struct Node
    {
        Vec3D pos;

        Vec3D T;
        Vec3D N;
        Vec3D B;

        glm::vec3 getPosition() const
        {
            return glm::vec3(
                (float)pos.x,
                (float)pos.y,
                (float)pos.z
            );
        }
    };

    struct ManufacturingRenderData
    {
        std::vector<Node> incomingStockNodes;
        std::vector<Node> positionedStraightNodes;
        std::vector<Node> activeZoneNodes;
        std::vector<Node> currentBendTraceNodes;
        std::vector<Node> frozenNodes;

        void clear()
        {
            incomingStockNodes.clear();
            positionedStraightNodes.clear();
            activeZoneNodes.clear();
            currentBendTraceNodes.clear();
            frozenNodes.clear();
        }
    };
  
    struct Segment
    {
        enum Type
        {
            LINE,
            ARC,
            ROTATE
        };

        Type type = LINE;

        double length = 0.0;   // used for LINE
        double curvature = 0.0; // ? = 1/R (ARC)
        double angle = 0.0;     // ARC bending angle
        double rotAngle = 0.0;  // twist

        double arcLength() const
        {
            if (curvature == 0.0) return 0.0;
            return angle / curvature; // R * angle
        }
    };


 struct GeometricSegment
    {
        enum Type
        {
            LINE,
            ARC
        };

        Type type;

        Frame startFrame;
        Frame endFrame;

        double length;

        double radius;
        double bendAngle;

        double rotationBefore;
    };

 struct FrozenSegment
    {
        GeometricSegment geometry;
    };



private:
    struct ActiveZone
    {
        // =====================================================
        // LOCAL DEFORMATION FRAME
        // =====================================================

        Frame frame;

        // =====================================================
        // BEND STATE
        // =====================================================

        double curvature = 0.0;

        double accumulatedAngle = 0.0;

        double targetAngle = 0.0;

        // =====================================================
        // ACTIVE DEFORMATION WINDOW
        // =====================================================

        double activeLength = 5.0;

        // =====================================================
        // LOCAL TEMPORARY GEOMETRY
        //
        // ONLY ACTIVE REGION DEFORMS
        // =====================================================

        std::vector<Node> localNodes;

        // =====================================================
        // STATE
        // =====================================================
        size_t frozenCountAtBendStart = 0;
        bool active = false;
    };

  struct IncomingStock
    {
        // =====================================================
        // OWNER:
        // PipeAxis3D owns incoming stock geometry state.
        //
        // SimulationController only calls processFeed(distance).
        // It does NOT build stock geometry.
        // =====================================================

        double totalLength = 300.0;      // full raw stock length
        double remainingLength = 300.0;  // stock not yet consumed by machine

		// Material already fed into machine     
        double consumedLength=0.0;

        bool visible = true;
    };
	

     struct LocalFrame
    {
        Vec3D origin;

        Vec3D tangent;
        Vec3D normal;
        Vec3D binormal;
         
       
    };

     struct PositionedStraight
     {
         // =====================================================
         // OWNER:
         // PipeAxis3D owns material already fed through machine
         // but not currently bending.
         //
         // Meaning:
         // - straight
         // - rigid
         // - not incoming anymore
         // - not active deformation
         // - not frozen bend geometry
         // =====================================================

         double length = 0.0;

         bool visible = true;

         std::vector<Node> nodes;
     };
   
    
      

    bool isBendActive() const
    {
        return activeZone.active;
    }
	

  
	public:


    


    // ======================================
// INPUT (authoritative)
// ======================================

    std::vector<Operation> operationHistory;
    //=================================================
    // ======================================
// GENERATED (derived)
// ======================================

    std::vector<GeometricSegment> generatedSegments;

    std::vector<Node> sampledNodes;

    //=================================================
public:
    // =====================================================================
    // CONSTRUCTOR
    // =====================================================================
    explicit PipeAxis3D(double stepSize = 0.5)
        : ds(stepSize), dirty(true)
    {   currentFrame.P = { 0,0,0 };
        currentFrame.T = { 1,0,0 };
        currentFrame.N = { 0,1,0 };
        currentFrame.B = { 0,0,1 };
        // =====================================================
   // INITIAL MACHINE ENTRY FRAME
   //
   // Incoming stock is positioned behind this frame.
   // =====================================================

		resetFrames();
    }

   
    // =====================================================================
    // CAD OPERATION INTERFACE (Store operations)-history API
    // =====================================================================

    /// Add straight feed
    void addFeed(double length)
    {
        Operation op;
        op.type = Operation::FEED;
        op.length = length;
        ops.push_back(op);
        markDirty();
    }

    /// Add arc bend
    void addBend(double radius, double angle)
    {
        Operation op;
        op.type = Operation::BEND;
        op.R = radius;
        op.angle = angle;
        ops.push_back(op);
        markDirty();
    }

    // Add rotation (twist)
    void addRotate(double angle)
    {
        Operation op;
        op.type = Operation::ROTATE;
        op.rotAngle = angle;
        ops.push_back(op);
        markDirty();
    }

    // =====================================================================
    // EDITING API (Modify operations)
    // =====================================================================

    // ================================
// EDITING ? ALWAYS MODIFY ops!
// ================================

    void setFeedLength(size_t index, double length)
    {
        if (index < ops.size() && ops[index].type == Operation::FEED)
        {
            ops[index].length = length;
            markDirty();
        }
    }

    void setBendRadius(size_t index, double radius)
    {
        if (index < ops.size() && ops[index].type == Operation::BEND)
        {
            ops[index].R = radius;
            markDirty();
        }
    }

    void setBendAngle(size_t index, double angle)
    {
        if (index < ops.size() && ops[index].type == Operation::BEND)
        {
            ops[index].angle = angle;
            markDirty();
        }
    }

    void clear()
    {
        ops.clear();
        segments.clear();
        nodes.clear();
        frozenNodes.clear();

        manufacturingRender.clear();

        activeZone.localNodes.clear();
        activeZone.accumulatedAngle = 0.0;
        activeZone.curvature = 0.0;
        activeZone.activeLength = 5.0;
        activeZone.active = false;

		resetFrames();

       
        incomingStock.remainingLength = incomingStock.totalLength;
		incomingStock.consumedLength = 0.0;
        positionedStraight.length = 0.0;
        positionedStraight.nodes.clear();

		resetFrames();
        markDirty();
    }

	// =====================================================================
	//GETTERS  for debugging 
    Frame getMachineEntryFrame() const
    {
        return machineEntryFrame;
    }

    double getIncomingStockRemainingLength() const
    {
        return incomingStock.remainingLength;
    }

    double getIncomingStockConsumedLength() const
    {
        return incomingStock.consumedLength;
    }

    double getIncomingStockTotalLength() const
    {
        return incomingStock.totalLength;
    }

   

	// =====================================================================
    //SETTERRS
    void setIncomingStockLength(double length)
    {
        if (length <= 0.0)
            return;

        incomingStock.totalLength = length;
        incomingStock.remainingLength = length;
        incomingStock.consumedLength = 0.0;

        markDirty();
    }



    // ====================================================
    // FUNCTION MANUFACTURING
    // 
     void updatePipeGeometryManufacturing()
    {
        // =====================================================
        // MANUFACTURING MODE
        //
        // Do NOT recreate PipeAxis3D.
        // Do NOT addFeed/addBend/addRotate here.
        // Do NOT call CAD build().
        //
        // PipeAxis3D already owns:
        // - incomingStock
        // - activeZone
        // - frozenNodes
        //
        // This function only asks it to rebuild visible nodes
        // from current manufacturing state.
        // =====================================================

        reconstructVisiblePipe();
    }

     void reconstructVisiblePipe()
     {
         // =====================================================
         // MANUFACTURING RENDER RECONSTRUCTION
         //
         // This does NOT execute CNC program.
         // This does NOT rebuild CAD history.
         //
         // It only converts current manufacturing state into
         // renderable zone groups.
         // =====================================================

         
         buildNodes();
     }
    // =====================================================================
    // BUILD (Reconstruct geometry from operations)
    // =====================================================================

    void build()
    {
        if (!dirty)
            return;

        std::cout << "\n=== BUILD START ===\n";

        buildSegments();

        executeSegments();

        std::cout << "ops: "
            << ops.size()
            << std::endl;

        std::cout << "segments: "
            << segments.size()
            << std::endl;

        std::cout << "nodes: "
            << nodes.size()
            << std::endl;

        std::cout << "frozenNodes: "
            << frozenNodes.size()
            << std::endl;

        std::cout << "activeNodes: "
            << activeZone.localNodes.size()
            << std::endl;

        dirty = false;
    }
    // MANUFACTURING API
	//=====================================================================
    void processFeed(double distance)
    {
        // =====================================================
        // FOUR-ZONE FEED PIPEFLOW
        //
        // IncomingStock  --->  PositionedStraight
        //
        // FEED:
        // - does NOT bend
        // - does NOT create active curvature
        // - does NOT directly create frozen bend geometry
        // - moves material from Zone 1 into Zone 2
        // =====================================================

        if (distance <= 0.0)
            return;

        double actualFeed =
            std::min(distance, incomingStock.remainingLength);

        if (actualFeed <= 0.0)
            return;

        // =====================================================
        // ZONE 1 — incoming stock becomes shorter
        // =====================================================

        incomingStock.remainingLength -= actualFeed;
        incomingStock.consumedLength += actualFeed;

        if (incomingStock.remainingLength < 0.0)
            incomingStock.remainingLength = 0.0;

        // =====================================================
        // ZONE 2 — positioned straight becomes longer
        // =====================================================

        positionedStraight.length += actualFeed;

        // =====================================================
        // ZONE 4 — already finished geometry is pushed forward
        //
        // Keep this if you already implemented rigid translation
        // of frozen geometry during feed.
        // =====================================================

        moveFrozenGeometryDuringFeed(actualFeed);

        std::cout << "[FEED 4Z] incomingRemaining="
            << incomingStock.remainingLength
            << " positionedStraight="
            << positionedStraight.length
            << " consumed="
            << incomingStock.consumedLength
            << std::endl;

        markDirty();
    } 


    void processBend(
        double radius,
        double targetAngle,
        double angleIncrement)
    {
        if (radius <= 1e-9)
        {
            std::cerr << "[processBend ERROR] radius <= 0\n";
            return;
        }

        if (targetAngle <= 0.0)
            return;

        if (angleIncrement <= 0.0)
            return;

        if (positionedStraight.length <= 0.0)
        {
            std::cerr << "[processBend WARNING] No positioned straight material available.\n";
            return;
        }

        // =====================================================
        // Start active bend once.
        // =====================================================

        if (!activeZone.active)
        {
            beginBendFromFrame(
                machineEntryFrame,
                radius,
                targetAngle
            );
        }

        // =====================================================
        // Previous downstream attachment frame.
        //
        // This is where old frozen geometry is currently attached:
        //
        // active bend output frame
        //        +
        // remaining positioned straight
        // =====================================================

        Frame previousFrozenAttachFrame =
            makePositionedStraightEndFrame(
                activeZone.frame,
                positionedStraight.length
            );

        // =====================================================
        // Clamp angle by:
        // - remaining target bend angle
        // - available positioned straight material
        // =====================================================

        double remainingAngle =
            activeZone.targetAngle
            - activeZone.accumulatedAngle;

        if (remainingAngle <= 0.0)
        {
            freezeActiveZone();
            markDirty();
            return;
        }

        double maxAngleByMaterial =
            positionedStraight.length / radius;

        double stepAngle =
            std::min(
                angleIncrement,
                std::min(remainingAngle, maxAngleByMaterial)
            );

        if (stepAngle <= 0.0)
            return;

        double arcStepLength =
            radius * stepAngle;

        // =====================================================
        // ZONE 2 -> ZONE 3
        //
        // Bending consumes positioned straight material.
        // =====================================================

        positionedStraight.length -= arcStepLength;

        if (positionedStraight.length < 0.0)
            positionedStraight.length = 0.0;

        // =====================================================
        // Update active bend geometry.
        //
        // This updates activeZone.frame and adds trace nodes.
        // =====================================================

        updateActiveZone(stepAngle);

        // =====================================================
        // New downstream attachment frame.
        //
        // Old frozen body must follow the END of the remaining
        // positioned straight, not the active zone itself.
        // =====================================================

        Frame newFrozenAttachFrame =
            makePositionedStraightEndFrame(
                activeZone.frame,
                positionedStraight.length
            );

        // =====================================================
        // Move old frozen geometry as rigid body.
        //
        // Shape does not deform.
        // It only follows the downstream attachment frame.
        // =====================================================

        transformFrozenGeometryBetweenFrames(
            previousFrozenAttachFrame,
            newFrozenAttachFrame
        );

        // =====================================================
        // Finish bend if target reached.
        // =====================================================

        if (activeZone.accumulatedAngle
            >= activeZone.targetAngle)
        {
            freezeActiveZone();
        }

        std::cout << "[BEND 4Z] arcStep="
            << arcStepLength
            << " positionedStraightLeft="
            << positionedStraight.length
            << std::endl;

        markDirty();
    }
    void processRotate(double angleIncrement)
    {
        // =====================================================
        // OWNER:
        // SimulationController decides WHEN rotation happens.
        // PipeAxis3D owns HOW machine/material frames rotate.
        //
        // ROTATE:
        // - does NOT move machine entry position
        // - does NOT move incoming stock position
        // - rotates N/B around feed axis T
        // - changes orientation of next bend plane
        // =====================================================

        if (std::abs(angleIncrement) < 1e-12)
            return;

        // =====================================================
        // Rotate material roll orientation at machine entry.
        //
        // P and T stay the same.
        // N/B rotate around T.
        // =====================================================

        buildRotate(machineEntryFrame, angleIncrement);

        // Keep current frame orientation synchronized for now.
        // Later we may split this into a separate bendPlaneFrame.

        buildRotate(currentFrame, angleIncrement);

       

        markDirty();
    }





	
    





    // =====================================================================
    // QUERY
    // =====================================================================

    const std::vector<Node>& getNodes() const { return nodes; }
	const std::vector<Segment>& getSegments() const { return segments; }
    size_t getSegmentCount() const { return segments.size(); }

    // =====================================================================
    // PROPERTIES
    // =====================================================================
    const ManufacturingRenderData& getManufacturingRenderData() const
    {
        return manufacturingRender;
    }

    double getTotalLength() const
    {
        Segment seg;
        double total = 0.0;
        for (const auto& seg : segments)
        {
            if (seg.type == Segment::LINE)
                total += seg.length;
            else if (seg.type == Segment::ARC)
                total += seg.arcLength();
        }
        return total;
    }

    void printSegments() const
    {
        std::cout << "\n=== Pipe Segments ===\n";
        for (size_t i = 0; i < segments.size(); ++i)
        {
            const auto& seg = segments[i];
            if (seg.type == Segment::LINE)
            {
                std::cout << i << ": FEED " << seg.length << " mm\n";
            }
            else if (seg.type == Segment::ARC)
            {
                std::cout << i << ": BEND R=" << seg.curvature << " mm, angle="
                    << (seg.angle * 180.0 / PI) << " deg\n";
            }
            else if (seg.type == Segment::ROTATE)
            {
                std::cout << i << ": ROTATE " << (seg.rotAngle * 180.0 / PI) << " deg\n";
            }
        }
        std::cout << "====================\n\n";
    }

private:
    // =====================================================================
    // DATA
    // =====================================================================
    //CAD /History data
    std::vector<Operation> ops;     // INPUT
    std::vector<Segment> segments;  // SIMULATION Operations to execute
    //Render output
    std::vector<Node> nodes;        // RENDER Resulting geometry 
    std::vector<Node> cadNodes;     // CAD Preview geometry
	//Manufacturing-state data
    std::vector<Node> currentBendTraceNodes;
    std::vector<Node> frozenNodes; 
    ActiveZone activeZone;
    IncomingStock incomingStock;
    PositionedStraight positionedStraight;
    Frame machineEntryFrame;//fixed machine/die entry frame
    Frame currentFrame;//current pipe/material frame
    // Manufacturing render grouping
    ManufacturingRenderData manufacturingRender;
    

    //General
    double ds;                      // Segment step size (mm)
    bool dirty;                     // Rebuild needed?



    
    //Future/CAD derived data -optional for now
	//std::vector<Operation> operationHistory;  // Copy of all operations (for potential future use)
	//std::vector<GeometricSegment> generatedSegments; // Detailed geometric segments (for potential future use)
	//std::vector<Node> sampledNodes; // Sampled nodes along pipe (for potential future use)
    

	// GETTERS PRIVATE 
    Frame getPositionedStraightStartFrame() const
    {
        if (activeZone.active)
        {
            return activeZone.frame;
        }

        // During FEED, new positioned straight is created
        // from the machine entry.
        return machineEntryFrame;
    }




    // =====================================================================
     //Private Functions
    // =====================================================================
       //Internal state

     void markDirty() { dirty = true; }
     
     //MATH/FRAME helpers
     //==================================================

    Vec3D rotateAroundAxis(const Vec3D& v, const Vec3D& axis, double angle) const
    {
        Vec3D a = normalize(axis);
        return v * cos(angle)
            + cross(a, v) * sin(angle)
            + a * dot(a, v) * (1.0 - cos(angle));
    }



    void orthonormalizeFrame(Frame& f)
    {
        // =====================================================
        // STEP 1
        // Normalize tangent
        // =====================================================

        f.T = f.T.normalized();

        // =====================================================
        // STEP 2
        // Rebuild binormal from T and N
        // =====================================================

        f.B = cross(f.T, f.N).normalized();

        // =====================================================
        // STEP 3
        // Rebuild normal from corrected B and T
        // =====================================================

        f.N = cross(f.B, f.T).normalized();
    }



    void transportFrame(
        const Vec3D& prevT,
        const Vec3D& currT,
        Frame& frame)
    {
        Vec3D axis = cross(prevT, currT);

        double axisLenSq = axis.lengthSquared();

        // =====================================================
        // Nearly identical tangents
        // =====================================================

        if (axisLenSq < 1e-12)
            return;

        axis = axis.normalized();

        double d = dot(prevT, currT);

        d = std::clamp(d, -1.0, 1.0);

        double angle = std::acos(d);

        // =====================================================
        // Minimal transport
        // =====================================================

        frame.N = rotateAroundAxis(frame.N, axis, angle);

        frame.B = rotateAroundAxis(frame.B, axis, angle);

        orthonormalizeFrame(frame);
    }




    //CAD Rebuild Path

    void buildSegments()
    {
        segments.clear();

        for (const auto& op : ops)
        {
            Segment s;

            if (op.type == Operation::FEED)
            {
                s.type = Segment::LINE;
                s.length = op.length;
            }
            else if (op.type == Operation::BEND)
            {
                s.type = Segment::ARC;
                s.curvature = 1.0 / op.R;   // ? key idea
                s.angle = op.angle;
            }
            else if (op.type == Operation::ROTATE)
            {
                s.type = Segment::ROTATE;
                s.rotAngle = op.rotAngle;
            }

            segments.push_back(s);
        }
    }



    void executeSegments()
    {
        // =====================================================
        // CAD REBUILD PATH
        //
        // This is NOT manufacturing simulation.
        // It produces deterministic CAD geometry from operations.
        // =====================================================

        nodes.clear();
        cadNodes.clear();

        // Do not use manufacturing containers here.
        // frozenNodes belongs to ManufacturingPlayback.
        activeZone.localNodes.clear();
        activeZone.active = false;

        Frame frame;

        frame.P = { 0,0,0 };
        frame.T = { 1,0,0 };
        frame.N = { 0,1,0 };
        frame.B = { 0,0,1 };

        cadNodes.push_back({
            frame.P,
            frame.T,
            frame.N,
            frame.B
            });

        for (const auto& seg : segments)
        {
            if (seg.type == Segment::LINE)
            {
                buildLineCAD(frame, seg.length);
            }
            else if (seg.type == Segment::ARC)
            {
                buildArcCAD(
                    frame,
                    seg.curvature,
                    seg.angle
                );
            }
            else if (seg.type == Segment::ROTATE)
            {
                buildRotate(frame, seg.rotAngle);
            }
        }

        currentFrame = frame;

        // Final CAD render geometry.
        nodes = cadNodes;
    }





void buildLineCAD(Frame& frame, double length)
{
    // =====================================================
    // CAD LINE BUILDER
    //
    // OWNER:
    // PipeAxis3D CAD rebuild path.
    //
    // LINE:
    // - moves frame.P along frame.T
    // - does not rotate frame
    // - writes to cadNodes, NOT frozenNodes
    // =====================================================

    if (length <= 0.0)
        return;

    int stepCount =
        std::max(1, static_cast<int>(std::ceil(length / ds)));

    double stepLength =
        length / static_cast<double>(stepCount);

    for (int i = 0; i < stepCount; ++i)
    {
        frame.P = frame.P + frame.T * stepLength;

        cadNodes.push_back({
            frame.P,
            frame.T,
            frame.N,
            frame.B
            });
    }
}


   
    void buildRotate(Frame& frame, double angle)
    {
        // Twist frame around tangent
        // Position unchanged, but N and B rotate
        frame.N = rotateAroundAxis(frame.N, frame.T, angle);
        frame.B = rotateAroundAxis(frame.B, frame.T, angle);
        // No new nodes added - geometry unchanged
    }

    //MANUFACTURING PATH
    //=========================================
    void beginBendFromFrame(
        const Frame& startFrame,
        double radius,
        double targetAngle)
    {
        if (radius <= 1e-9)
        {
            std::cerr << "[BEND ERROR] Invalid radius: "
                << radius << std::endl;
            return;
        }

        activeZone.frame = startFrame;
        activeZone.curvature = 1.0 / radius;
        activeZone.targetAngle = targetAngle;
        activeZone.accumulatedAngle = 0.0;
        activeZone.localNodes.clear();
        activeZone.active = true;

        activeZone.frozenCountAtBendStart =
            frozenNodes.size();

        Node startNode{
            activeZone.frame.P,
            activeZone.frame.T,
            activeZone.frame.N,
            activeZone.frame.B
        };

        // Small moving active window starts here.
        activeZone.localNodes.push_back(startNode);

        // Full visible bend trace starts here and stays anchored.
        currentBendTraceNodes.clear();
        currentBendTraceNodes.push_back(startNode);

        std::cout << "[ACTIVE ZONE BEGIN] radius="
            << radius
            << " targetAngle="
            << targetAngle
            << std::endl;
    }



    void buildArcCAD(
        Frame& frame,
        double curvature,
        double angle)
    {
        // =====================================================
        // CAD ARC BUILDER
        //
        // This builds a perfect planar circular arc.
        //
        // ROTATE changes the frame before this function.
        // This function then bends around the current frame.B.
        // =====================================================

        if (std::abs(curvature) < 1e-12)
            return;

        if (std::abs(angle) < 1e-12)
            return;

        double radius =
            1.0 / std::abs(curvature);

        double arcLength =
            radius * std::abs(angle);

        int steps =
            std::max(1, static_cast<int>(std::ceil(arcLength / ds)));

        double stepAngle =
            angle / static_cast<double>(steps);

        double stepLength =
            arcLength / static_cast<double>(steps);

        // =====================================================
        // Fixed bend axis for this entire CAD arc.
        //
        // This prevents the bend plane from drifting.
        // =====================================================

        Vec3D bendAxis =
            frame.B.normalized();

        for (int i = 0; i < steps; ++i)
        {
            Vec3D prevT =
                frame.T;

            frame.T =
                rotateAroundAxis(
                    frame.T,
                    bendAxis,
                    stepAngle
                ).normalized();

            Vec3D midT =
                normalize(prevT + frame.T);

            frame.P =
                frame.P + midT * stepLength;

            // Keep frame locked to the arc plane.
            frame.B = bendAxis;
            frame.N = cross(frame.B, frame.T).normalized();

            cadNodes.push_back({
                frame.P,
                frame.T,
                frame.N,
                frame.B
                });
        }

        currentFrame = frame;
    }


//HELPER FRAMES REST HELPER

    void resetFrames()
    {
        // =====================================================
        // MACHINE ENTRY FRAME
        //
        // OWNER:
        // PipeAxis3D owns the machine entry reference.
        //
        // Meaning:
        // P = fixed die / entry position
        // T = feed direction
        // N/B = current roll orientation around pipe axis
        // =====================================================

        machineEntryFrame.P = { 0,0,0 };
        machineEntryFrame.T = { 1,0,0 };
        machineEntryFrame.N = { 0,1,0 };
        machineEntryFrame.B = { 0,0,1 };

        // =====================================================
        // CURRENT MATERIAL FRAME
        //
        // At reset, material frame starts at machine entry.
        // =====================================================

        currentFrame = machineEntryFrame;

        // =====================================================
        // Incoming stock is visualized behind machine entry.
        // =====================================================

       
    }

    


//=============================
// LEGACY NOT USED IN CURRENT IMPLEMENTATION
//======================
   // void beginBend(
    //    const Frame& startFrame,
    //    double radius,
    //    double targetAngle)
    //{
        // =====================================================
        // INITIALIZE ACTIVE BEND STATE
        // =====================================================

     //   activeZone.frame = startFrame;
     //   activeZone.curvature = 1.0 / radius;
     //   activeZone.targetAngle = targetAngle;
     //   activeZone.accumulatedAngle = 0.0;
     //   activeZone.localNodes.clear();
     //   activeZone.active = true;

        // =====================================================
        // START FROM CURRENT PIPE END FRAME
        // =====================================================

      //  if (!nodes.empty())
      //  {
      //      const Node& last = nodes.back();

      //      activeZone.frame.P = last.pos;

       //     activeZone.frame.T = last.T;
      //      activeZone.frame.N = last.N;
      //      activeZone.frame.B = last.B;
      //  }
      //  else
      //  {
      //      activeZone.frame.P = { 0,0,0 };
    //
       //     activeZone.frame.T = { 1,0,0 };
       //     activeZone.frame.N = { 0,1,0 };
       //     activeZone.frame.B = { 0,0,1 };
       // }
    //}



    void updateActiveZone(double stepAngle)
    {
        // ==========================================================
        // ACTIVE ZONE PIPE FLOW — FOUR-ZONE MODEL
        //
        // PositionedStraight
        //        ?
        // ActiveZone.localNodes
        //        ?
        // currentBendTraceNodes
        //        ?
        // FrozenGeometry after bend completion
        //
        // IMPORTANT:
        // activeZone.localNodes is only the local deformation window.
        // currentBendTraceNodes is the visible arc being formed.
        // frozenNodes is completed geometry from previous bends.
        // ==========================================================

        // ==========================================================
        // STEP 1
        // Validate active bend state
        // ==========================================================

        if (!activeZone.active)
            return;

        if (std::abs(activeZone.curvature) < 1e-12)
            return;

        if (stepAngle <= 0.0)
            return;

        if (activeZone.accumulatedAngle >= activeZone.targetAngle)
            return;

        if (ds <= 1e-9)
            return;

        // ==========================================================
        // STEP 2
        // Clamp angular increment
        // ==========================================================

        double remaining =
            activeZone.targetAngle
            - activeZone.accumulatedAngle;

        double dA =
            std::min(stepAngle, remaining);

        if (dA <= 0.0)
            return;

        // ==========================================================
        // STEP 3
        // Save previous tangent
        // ==========================================================

        Vec3D prevT =
            activeZone.frame.T;

        // ==========================================================
        // STEP 4
        // Curvature propagation
        //
        // Rotate tangent around current bend axis.
        // ==========================================================

        activeZone.frame.T =
            rotateAroundAxis(
                activeZone.frame.T,
                activeZone.frame.B,
                dA
            ).normalized();

        // ==========================================================
        // STEP 5
        // Frame transport
        // ==========================================================

        transportFrame(
            prevT,
            activeZone.frame.T,
            activeZone.frame
        );

        // ==========================================================
        // STEP 6
        // Midpoint integration
        //
        // stepLength = R * dA
        // curvature = 1 / R
        // therefore stepLength = dA / curvature
        // ==========================================================

        Vec3D midT =
            normalize(prevT + activeZone.frame.T);

        if (midT.lengthSquared() < 1e-12)
        {
            midT = activeZone.frame.T;
        }

        double stepLength =
            std::abs(dA / activeZone.curvature);

        activeZone.frame.P =
            activeZone.frame.P
            + midT * stepLength;

        // ==========================================================
        // STEP 7
        // Create new sample node
        //
        // 1. activeZone.localNodes:
        //    local deformation window
        //
        // 2. currentBendTraceNodes:
        //    full visible arc trace
        // ==========================================================

        Node newNode{
            activeZone.frame.P,
            activeZone.frame.T,
            activeZone.frame.N,
            activeZone.frame.B
        };

        activeZone.localNodes.push_back(newNode);

        currentBendTraceNodes.push_back(newNode);

        // ==========================================================
        // STEP 8
        // Accumulate bend progress
        // ==========================================================

        activeZone.accumulatedAngle += dA;

        if (activeZone.accumulatedAngle > activeZone.targetAngle)
        {
            activeZone.accumulatedAngle =
                activeZone.targetAngle;
        }

        // ==========================================================
        // STEP 9
        // Keep active window short
        //
        // This removes old nodes from activeZone.localNodes only.
        // It does NOT push them to frozenNodes.
        // ==========================================================

        maintainActiveWindow();

        // ==========================================================
        // DEBUG
        // ==========================================================

        std::cout << "[ACTIVE ZONE] angle="
            << activeZone.accumulatedAngle
            << " / "
            << activeZone.targetAngle
            << " activeNodes="
            << activeZone.localNodes.size()
            << " traceNodes="
            << currentBendTraceNodes.size()
            << " frozenNodes="
            << frozenNodes.size()
            << std::endl;
    }

    
    // freezeActive zone
   //order
    
    void freezeActiveZone()
    {
        // =====================================================
        // STEP 1
        // Current bend trace becomes frozen geometry.
        // =====================================================

        for (const auto& node : currentBendTraceNodes)
        {
            frozenNodes.push_back(node);
        }

        currentBendTraceNodes.clear();

        // =====================================================
        // STEP 2
        // Bend output frame.
        // Positioned straight starts from here.
        // =====================================================

        currentFrame = activeZone.frame;

        // =====================================================
        // STEP 3
        // Remaining positioned straight becomes frozen too.
        //
        // This prevents old positioned straight from staying
        // as a temporary scalar length after the bend.
        // =====================================================

        freezePositionedStraight();

        // =====================================================
        // STEP 4
        // Clear active state.
        // =====================================================

        activeZone.localNodes.clear();
        activeZone.active = false;

        std::cout << "[FREEZE ACTIVE ZONE] frozenNodes="
            << frozenNodes.size()
            << std::endl;
    }




    void buildIncomingStock()
    {
        // =====================================================
        // OWNER:
        // PipeAxis3D owns incoming stock render reconstruction.
        //
        // OUTPUT:
        // manufacturingRender.incomingStockNodes
        //
        // IMPORTANT:
        // This function does NOT simulate FEED.
        // It only draws current incoming stock state.
        // =====================================================

        manufacturingRender.incomingStockNodes.clear();

        if (!incomingStock.visible)
            return;

        if (incomingStock.remainingLength <= 0.0)
            return;

        if (ds <= 1e-9)
            return;

        // =====================================================
        // MACHINE ENTRY REFERENCE
        //
        // machineEntryFrame is the fixed die / entry point.
        //
        // Incoming stock is always drawn behind it.
        // =====================================================

        const Frame& entry =
            machineEntryFrame;

        double visibleLength =
            incomingStock.remainingLength;

        int stepCount =
            std::max(
                1,
                static_cast<int>(std::ceil(visibleLength / ds))
            );

        double stepLength =
            visibleLength / static_cast<double>(stepCount);

        // =====================================================
        // Stock tail is behind the machine entry.
        //
        // entry.P = machine entry / die location
        // entry.T = feed direction
        //
        // tail = entry.P - entry.T * remainingLength
        // =====================================================

        Vec3D tailPoint =
            entry.P - entry.T * visibleLength;

        // =====================================================
        // Build stock from tail toward machine entry.
        //
        // tail ---------------------> machine entry
        // =====================================================

        for (int i = 0; i <= stepCount; ++i)
        {
            double s =
                stepLength * static_cast<double>(i);

            Vec3D p =
                tailPoint + entry.T * s;

            manufacturingRender.incomingStockNodes.push_back({
                p,
                entry.T,
                entry.N,
                entry.B
                });
        }
    }


	//=================================================
    //POSITIONING

    void buildPositionedStraight()
    {
        // =====================================================
        // OWNER:
        // PipeAxis3D owns positioned-straight visualization.
        //
        // This does NOT simulate feed or bend.
        // It only draws Zone 2 from current state.
        // =====================================================

        manufacturingRender.positionedStraightNodes.clear();
        positionedStraight.nodes.clear();

        if (!positionedStraight.visible)
            return;

        if (positionedStraight.length <= 0.0)
            return;

        if (ds <= 1e-9)
            return;

        Frame startFrame =
            getPositionedStraightStartFrame();

        double visibleLength =
            positionedStraight.length;

        int stepCount =
            std::max(
                1,
                static_cast<int>(std::ceil(visibleLength / ds))
            );

        double stepLength =
            visibleLength / static_cast<double>(stepCount);

        for (int i = 0; i <= stepCount; ++i)
        {
            double s =
                stepLength * static_cast<double>(i);

            Vec3D p =
                startFrame.P + startFrame.T * s;

            Node node{
                p,
                startFrame.T,
                startFrame.N,
                startFrame.B
            };

            positionedStraight.nodes.push_back(node);

            manufacturingRender.positionedStraightNodes.push_back(node);
        }
    }

//POSTIONONG HELPER
   // bake positioned stright into frozen geometry 

    void freezePositionedStraight()
    {
        // =====================================================
        // ZONE 2 -> ZONE 4 TRANSFER
        //
        // When a bend operation finishes, any remaining
        // positioned straight section is no longer a temporary
        // feed buffer.
        //
        // It becomes completed pipe geometry and should move
        // later as part of frozen geometry.
        // =====================================================

        if (positionedStraight.length <= 0.0)
            return;

        if (ds <= 1e-9)
            return;

        // Start from the current bend output frame.
        Frame frame = currentFrame;

        int stepCount =
            std::max(
                1,
                static_cast<int>(std::ceil(positionedStraight.length / ds))
            );

        double stepLength =
            positionedStraight.length / static_cast<double>(stepCount);

        for (int i = 1; i <= stepCount; ++i)
        {
            double s =
                stepLength * static_cast<double>(i);

            Vec3D p =
                frame.P + frame.T * s;

            frozenNodes.push_back({
                p,
                frame.T,
                frame.N,
                frame.B
                });
        }

        // Move currentFrame to the end of this straight section.
        currentFrame.P =
            frame.P + frame.T * positionedStraight.length;

        currentFrame.T = frame.T;
        currentFrame.N = frame.N;
        currentFrame.B = frame.B;

        positionedStraight.length = 0.0;
        positionedStraight.nodes.clear();

        std::cout << "[FREEZE POSITIONED STRAIGHT]\n";
    }

	//Positionin Helper

    Frame makePositionedStraightEndFrame(
        const Frame& startFrame,
        double length) const
    {
        // =====================================================
        // POSITIONED STRAIGHT END FRAME
        //
        // startFrame:
        //   usually activeZone.frame during bending
        //
        // length:
        //   remaining positioned straight length
        //
        // The old frozen body should attach here.
        // =====================================================

        Frame endFrame = startFrame;

        if (length > 0.0)
        {
            endFrame.P =
                startFrame.P
                + startFrame.T.normalized() * length;
        }

        return endFrame;
    }




    void buildManufacturingRenderData()
    {
        manufacturingRender.clear();

        // =====================================================
        // ZONE 1 — Incoming Stock
        // =====================================================

        buildIncomingStock();

        // =====================================================
        // ZONE 2 — Positioned Straight
        // =====================================================

        buildPositionedStraight();

        // =====================================================
        // CURRENT BEND TRACE
        //
        // This is the visible arc currently being formed.
        // It is NOT the same as activeZone.localNodes.
        // =====================================================

        manufacturingRender.currentBendTraceNodes =
            currentBendTraceNodes;

        // =====================================================
        // ZONE 3 — Active Bend Window
        //
        // Small moving/local deformation window.
        // =====================================================

        manufacturingRender.activeZoneNodes =
            activeZone.localNodes;

        // =====================================================
        // ZONE 4 — Frozen Geometry
        // =====================================================

        manufacturingRender.frozenNodes =
            frozenNodes;

        std::cout << "[MFG RENDER DATA] "
            << "incoming=" << manufacturingRender.incomingStockNodes.size()
            << " positioned=" << manufacturingRender.positionedStraightNodes.size()
            << " trace=" << manufacturingRender.currentBendTraceNodes.size()
            << " active=" << manufacturingRender.activeZoneNodes.size()
            << " frozen=" << manufacturingRender.frozenNodes.size()
            << std::endl;
    }


    void flattenManufacturingRenderData()
    {
        // =====================================================
        // TEMPORARY LEGACY OUTPUT
        //
        // This is only for getNodes() compatibility/debugging.
        // Renderer should eventually draw zones separately.
        // =====================================================

        nodes.clear();

        // ZONE 1 — Incoming Stock
        for (const auto& node : manufacturingRender.incomingStockNodes)
        {
            nodes.push_back(node);
        }

        // ZONE 2 — Positioned Straight
        for (const auto& node : manufacturingRender.positionedStraightNodes)
        {
            nodes.push_back(node);
        }

        // CURRENT BEND TRACE
        //
        // Full visible arc currently being formed.
        for (const auto& node : manufacturingRender.currentBendTraceNodes)
        {
            nodes.push_back(node);
        }

        // ZONE 4 — Frozen Geometry
        for (const auto& node : manufacturingRender.frozenNodes)
        {
            nodes.push_back(node);
        }

        // ZONE 3 — Active Bend Window
        //
        // Draw last in flattened order because it is the working zone.
        for (const auto& node : manufacturingRender.activeZoneNodes)
        {
            nodes.push_back(node);
        }
    }

   // void buildNodes()
    //{
        // =====================================================
        // VISIBLE PIPE RECONSTRUCTION
        //
        // OWNER:
        // PipeAxis3D owns final render geometry assembly.
        //
        // Pipe is reconstructed from:
        //
        // 1. Incoming Stock
        // 2. Active Bend Zone
        // 3. Frozen Geometry
        //
        // =====================================================

       // nodes.clear();

        // =====================================================
        // ZONE 1 — INCOMING STOCK
        //
        // Straight only.
        // Translation/feed only.
        // No curvature.
        // =====================================================

       // buildIncomingStock();

        // =====================================================
        // ZONE 2 — ACTIVE BEND WINDOW
        //
        // Local temporary deformation.
        // This is the only deforming area.
        // =====================================================

       // for (const auto& node : activeZone.localNodes)
       // {
       //     nodes.push_back(node);
       // }

        // =====================================================
        // ZONE 3 — FROZEN GEOMETRY
        //
        // Immutable manufactured pipe.
        // No deformation.
        // =====================================================

        //for (const auto& node : frozenNodes)
        //{
         //   nodes.push_back(node);
        //}
    //}

    void buildNodes()
    {
        // =====================================================
        // BUILD MANUFACTURING RENDER DATA
        //
        // Zone 1: Incoming Stock
        // Zone 2: Active Bend Zone
        // Zone 3: Frozen Geometry
        // =====================================================

        buildManufacturingRenderData();

        // =====================================================
        // TEMPORARY LEGACY OUTPUT
        //
        // Current renderer still expects one nodes vector.
        // =====================================================

        flattenManufacturingRenderData();
    }



    // HELPER

  


    void freezeOldestActiveNode()
    {
        // =====================================================
        // OWNER:
        // PipeAxis3D owns transfer from active deformation
        // state into frozen manufactured geometry.
        //
        // PIPEFLOW:
        //
        // activeZone.localNodes.front()
        //        ?
        // frozenNodes
        //
        // Meaning:
        // oldest material has exited bend window.
        // It no longer deforms.
        // =====================================================

        if (activeZone.localNodes.empty())
            return;

        frozenNodes.push_back(
            activeZone.localNodes.front()
        );

        activeZone.localNodes.erase(
            activeZone.localNodes.begin()
        );
    }

    void releaseOldestActiveNode()
    {
        // =====================================================
        // Active window remains local.
        //
        // The old node is NOT pushed into frozenNodes here.
        // It is already preserved in currentBendTraceNodes.
        // =====================================================

        if (activeZone.localNodes.empty())
            return;

        activeZone.localNodes.erase(
            activeZone.localNodes.begin()
        );
    }



    //HELPER

	// It's connected with freezeOldestActiveNode
    // 

    void maintainActiveWindow()
    {
        if (ds <= 1e-9)
            return;

        size_t maxActiveNodes =
            static_cast<size_t>(
                std::ceil(activeZone.activeLength / ds)
                );

        maxActiveNodes =
            std::max<size_t>(2, maxActiveNodes);

        while (activeZone.localNodes.size() > maxActiveNodes)
        {
            releaseOldestActiveNode();
        }
    }
    

	// =====================================================
	// TRANSLATE
    // 
	// =====================================================
    void translateNode(Node& node, const Vec3D& delta)
    {
        // =====================================================
        // RIGID TRANSLATION
        //
        // Position changes.
        // T/N/B do NOT change.
        //
        // This means the geometry moves but does not deform.
        // =====================================================

        node.pos = node.pos + delta;
    }

    void translateFrame(Frame& frame, const Vec3D& delta)
    {
        // =====================================================
        // RIGID TRANSLATION OF FRAME
        //
        // Only origin moves.
        // Orientation stays unchanged.
        // =====================================================

        frame.P = frame.P + delta;
    }

    


    void translateFrozenGeometry(const Vec3D& delta)
{
    // =====================================================
    // ZONE 3 — FROZEN GEOMETRY RIGID MOTION
    //
    // OWNER:
    // PipeAxis3D owns frozen geometry state.
    //
    // Meaning:
    // Finished geometry moves through space,
    // but its shape is not recalculated.
    // =====================================================

    for (auto& node : frozenNodes)
    {
        translateNode(node, delta);
    }
}

    void moveFrozenGeometryDuringFeed(double feedDistance)
    {
        // =====================================================
        // FEED EFFECT ON ZONE 3
        //
        // During FEED, finished pipe is pushed forward.
        //
        // Machine entry stays fixed.
        // Incoming stock shortens.
        // Frozen geometry translates rigidly.
        // =====================================================

        if (feedDistance <= 0.0)
            return;

        if (frozenNodes.empty())
            return;

        Vec3D feedDirection =
            machineEntryFrame.T.normalized();

        Vec3D delta =
            feedDirection * feedDistance;

        translateFrozenGeometry(delta);

        // Keep current material frame consistent with moved geometry.
        translateFrame(currentFrame, delta);

        std::cout << "[ZONE 3 MOVE] frozen geometry translated by "
            << feedDistance
            << std::endl;
    }
	//=============================
    // ATTACH  RIGIDLY ZONE 3  TO ACTIVE ZONE 2
	//==============================


    // HELPERS
	// FRAME TRANSFORMS HELPERS
    Vec3D vectorToLocal(
        const Vec3D& v,
        const Frame& frame) const
    {
        // =====================================================
        // Convert world vector into frame-local coordinates.
        // =====================================================

        return {
            dot(v, frame.T),
            dot(v, frame.N),
            dot(v, frame.B)
        };
    }

    Vec3D vectorFromLocal(
        const Vec3D& v,
        const Frame& frame) const
    {
        // =====================================================
        // Convert local vector into world coordinates.
        // =====================================================

        return
            frame.T * v.x +
            frame.N * v.y +
            frame.B * v.z;
    }

    Vec3D pointToLocal(
        const Vec3D& p,
        const Frame& frame) const
    {
        // =====================================================
        // Convert world point into frame-local coordinates.
        // =====================================================

        Vec3D relative =
            p - frame.P;

        return vectorToLocal(
            relative,
            frame
        );
    }

    Vec3D pointFromLocal(
        const Vec3D& p,
        const Frame& frame) const
    {
        // =====================================================
        // Convert local point into world coordinates.
        // =====================================================

        return
            frame.P +
            frame.T * p.x +
            frame.N * p.y +
            frame.B * p.z;
    }
//NODE TRANSFORM HELPER

    void transformNodeBetweenFrames(
        Node& node,
        const Frame& fromFrame,
        const Frame& toFrame)
    {
        // =====================================================
        // RIGID BODY TRANSFORM
        //
        // Shape does not deform.
        //
        // We convert node position/orientation into old frame,
        // then rebuild it in the new frame.
        // =====================================================

        Vec3D localPos =
            pointToLocal(
                node.pos,
                fromFrame
            );

        Vec3D localT =
            vectorToLocal(
                node.T,
                fromFrame
            );

        Vec3D localN =
            vectorToLocal(
                node.N,
                fromFrame
            );

        Vec3D localB =
            vectorToLocal(
                node.B,
                fromFrame
            );

        node.pos =
            pointFromLocal(
                localPos,
                toFrame
            );

        node.T =
            vectorFromLocal(
                localT,
                toFrame
            ).normalized();

        node.N =
            vectorFromLocal(
                localN,
                toFrame
            ).normalized();

        node.B =
            vectorFromLocal(
                localB,
                toFrame
            ).normalized();
    }

//FROZEN GEOMETRY TRANSFORM

    void transformFrozenGeometryBetweenFrames(
        const Frame& fromFrame,
        const Frame& toFrame)
    {
        // =====================================================
        // ZONE 3 — RIGID BODY FOLLOW
        //
        // Frozen geometry does NOT deform.
        // But during active bending it must follow the outgoing
        // active zone frame.
        //
        // This makes old completed geometry stay attached
        // to the active bend.
        // =====================================================

        if (frozenNodes.empty())
            return;

        for (auto& node : frozenNodes)
        {
            transformNodeBetweenFrames(
                node,
                fromFrame,
                toFrame
            );
        }
    }

    // ADDITIONALLY NEED Update updateActiveZone()
    // IT WAS DONE

    // END -ATTACH  RIGIDLY ZONE 3  TO ACTIVE ZONE 2
//==============================


};