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
        // =====================================================
        // OWNER:
        // PipeAxis3D owns manufacturing render grouping.
        //
        // PURPOSE:
        // Keep the three manufacturing zones separate.
        //
        // Renderer can later draw these with different colors,
        // line styles, or separate draw calls.
        // =====================================================

        std::vector<Node> incomingStockNodes; // Zone 1
        std::vector<Node> activeZoneNodes;    // Zone 2
        std::vector<Node> frozenNodes;        // Zone 3

        void clear()
        {
            incomingStockNodes.clear();
            activeZoneNodes.clear();
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
        activeZone.activeLength = 20.0;
        activeZone.active = false;

		resetFrames();

       
        incomingStock.remainingLength = incomingStock.totalLength;
		incomingStock.consumedLength = 0.0;

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
        // OWNER SPLIT:
        //
        // SimulationController:
        //   decides WHEN feed happens
        //   decides HOW MUCH feed this frame
        //
        // PipeAxis3D:
        //   owns incoming stock state
        //   owns frozen geometry rigid motion
        // =====================================================

        if (distance <= 0.0)
            return;

        // =====================================================
        // Clamp feed to available stock.
        // =====================================================

        double actualFeed =
            std::min(distance, incomingStock.remainingLength);

        if (actualFeed <= 0.0)
            return;

        // =====================================================
        // ZONE 1 — Incoming Stock
        //
        // Raw stock before machine entry becomes shorter.
        // =====================================================

        incomingStock.remainingLength -= actualFeed;
        incomingStock.consumedLength += actualFeed;

        if (incomingStock.remainingLength < 0.0)
            incomingStock.remainingLength = 0.0;

        // =====================================================
        // ZONE 3 — Frozen Geometry
        //
        // Finished geometry moves rigidly during FEED.
        // Shape is unchanged.
        // =====================================================

        moveFrozenGeometryDuringFeed(actualFeed);

        std::cout << "[FEED] consumed="
            << incomingStock.consumedLength
            << " remaining="
            << incomingStock.remainingLength
            << " movedFrozenBy="
            << actualFeed
            << std::endl;

        markDirty();
    }



    void processBend(
        double radius,
        double targetAngle,
        double angleIncrement)
    {
        // =====================================================
        // OWNER SPLIT:
        //
        // SimulationController:
        //   decides WHEN bending happens
        //   decides HOW MUCH angle this frame
        //
        // PipeAxis3D:
        //   owns active zone
        //   owns frame evolution
        //   owns frozen geometry
        // =====================================================

        if (radius <= 1e-9)
        {
            std::cerr << "[processBend ERROR] radius <= 0\n";
            return;
        }

        if (targetAngle <= 0.0)
            return;

        if (angleIncrement <= 0.0)
            return;

        // =====================================================
        // PIPEFLOW:
        //
        // currentFrame
        //      ?
        // active bend zone starts here
        //      ?
        // local curvature evolves
        //      ?
        // completed material freezes
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
        // Clamp increment to remaining bend angle.
        // Prevent overshoot.
        // =====================================================

        double remaining =
            activeZone.targetAngle
            - activeZone.accumulatedAngle;

        if (remaining <= 0.0)
        {
            freezeActiveZone();
            markDirty();
            return;
        }

        double stepAngle =
            std::min(angleIncrement, remaining);

        // =====================================================
        // Advance local active deformation.
        // =====================================================

        updateActiveZone(stepAngle);

        // =====================================================
        // If bend finished, freeze remaining active geometry.
        // =====================================================

        if (activeZone.accumulatedAngle
            >= activeZone.targetAngle)
        {
            freezeActiveZone();
        }

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
    std::vector<Node> frozenNodes; 
    ActiveZone activeZone;
    IncomingStock incomingStock;
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
        // =====================================================
        // OWNER:
        // PipeAxis3D owns active bend initialization.
        //
        // ACTIVE ZONE MEANING:
        // This is the local deformation window at the machine.
        //
        // BEND START:
        // The bend starts from an explicit frame.
        // In Manufacturing mode this should be machineEntryFrame.
        // =====================================================

        if (radius <= 1e-9)
        {
            std::cerr << "[BEND ERROR] Invalid radius: "
                << radius << std::endl;
            return;
        }

        // =====================================================
        // Reset active bend state
        // =====================================================

        activeZone.frame = startFrame;

        activeZone.curvature = 1.0 / radius;

        activeZone.targetAngle = targetAngle;

        activeZone.accumulatedAngle = 0.0;

        activeZone.localNodes.clear();

        activeZone.active = true;

        // =====================================================
        // Important:
        // Add start node so active zone is visible immediately.
        // Without this, the active zone appears only after
        // first deformation step.
        // =====================================================

        activeZone.localNodes.push_back({
            activeZone.frame.P,
            activeZone.frame.T,
            activeZone.frame.N,
            activeZone.frame.B
            });

        std::cout << "[ACTIVE ZONE BEGIN] "
            << "radius=" << radius
            << " targetAngle=" << targetAngle
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
        // ACTIVE ZONE PIPE FLOW
        //
        // machineEntryFrame
        //        ?
        // local curvature evolution
        //        ?
        // activeZone.localNodes
        //        ?
        // old material exits active window
        //        ?
        // frozenNodes
        //
        // ONLY activeZone.localNodes deform.
        // frozenNodes never deform again.
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
        //
        // Prevent overshooting target bend angle.
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
        //
        // Needed for midpoint integration and frame transport.
        // ==========================================================

        Vec3D prevT =
            activeZone.frame.T;

        // ==========================================================
        // STEP 4
        // Local curvature propagation
        //
        // Rotate tangent around current bend axis.
        // This is the geometric deformation step.
        // ==========================================================

        activeZone.frame.T =
            rotateAroundAxis(
                activeZone.frame.T,
                activeZone.frame.B,
                dA
            );

        activeZone.frame.T =
            activeZone.frame.T.normalized();

        // ==========================================================
        // STEP 5
        // Minimal frame transport
        //
        // Keeps N/B stable while tangent changes.
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
        // Arc-length step:
        //
        //     ds = R * dA
        //
        // But in your simulator ds is also the geometric sampling
        // step, so this advances one node-length along the bend.
        // ==========================================================

        Vec3D midT =
            normalize(prevT + activeZone.frame.T);

        if (midT.lengthSquared() < 1e-12)
        {
            midT = activeZone.frame.T;
        }

        activeZone.frame.P =
            activeZone.frame.P
            + midT * ds;

        // ==========================================================
        // STEP 7
        // Store new active-zone sample
        //
        // This node still belongs to active deformation.
        // It is NOT frozen yet.
        // ==========================================================

        activeZone.localNodes.push_back({
            activeZone.frame.P,
            activeZone.frame.T,
            activeZone.frame.N,
            activeZone.frame.B
            });

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
        // Transfer old active material into frozen geometry
        //
        // Active zone must remain local.
        // Oldest active nodes leave the deformation window.
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
            << " frozenNodes="
            << frozenNodes.size()
            << std::endl;
    }

    



    void freezeActiveZone()
    {
        // =====================================================
        // FINALIZE ACTIVE BEND
        //
        // Called when the BEND operation completes.
        //
        // Any remaining active nodes are no longer deforming.
        // They become frozen manufactured geometry.
        // =====================================================

        for (const auto& node : activeZone.localNodes)
        {
            frozenNodes.push_back(node);
        }

        activeZone.localNodes.clear();

        activeZone.active = false;

        // =====================================================
        // Commit final material frame.
        //
        // Future operations continue from this orientation.
        // =====================================================

        currentFrame = activeZone.frame;

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

    void buildManufacturingRenderData()
    {
        // =====================================================
        // OWNER:
        // PipeAxis3D owns manufacturing render reconstruction.
        //
        // PURPOSE:
        // Keep zones separated.
        //
        // Zone 1: Incoming stock
        // Zone 2: Active bend window
        // Zone 3: Frozen finished geometry
        // =====================================================

        manufacturingRender.clear();

        // =====================================================
        // ZONE 1 — Incoming Stock
        // =====================================================

        buildIncomingStock();

        // =====================================================
        // ZONE 2 — Active Bend Zone
        //
        // This is local, temporary deformation geometry.
        // It should be rendered separately from incoming stock
        // and frozen geometry.
        // =====================================================

        manufacturingRender.activeZoneNodes =
            activeZone.localNodes;
        // =====================================================

        manufacturingRender.activeZoneNodes =
            activeZone.localNodes;

        // =====================================================
        // ZONE 3 — Frozen Geometry
        // =====================================================

        manufacturingRender.frozenNodes =
            frozenNodes;
    }

    void flattenManufacturingRenderData()
    {
        // =====================================================
        // TEMPORARY LEGACY OUTPUT
        //
        // Current renderer expects one continuous nodes vector.
        //
        // Better physical order:
        //
        // Incoming Stock -> Frozen Geometry -> Active Zone
        //
        // This reduces false connector lines.
        // Final solution: render each zone separately.
        // =====================================================

        nodes.clear();

        // =====================================================
        // ZONE 1 — Incoming Stock
        // =====================================================

        for (const auto& node : manufacturingRender.incomingStockNodes)
        {
            nodes.push_back(node);
        }

        // =====================================================
        // ZONE 3 — Frozen Geometry
        //
        // Old material that already left active zone.
        // Should come before active zone in centerline order.
        // =====================================================

        for (const auto& node : manufacturingRender.frozenNodes)
        {
            nodes.push_back(node);
        }

        // =====================================================
        // ZONE 2 — Active Bend Zone
        //
        // Current local deformation window.
        // Comes last because it is the newest material.
        // =====================================================

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





    //HELPER

	// It's connected with freezeOldestActiveNode 
    void maintainActiveWindow()
    {
        // =====================================================
        // ACTIVE WINDOW CONTROL
        //
        // The active bend zone must remain local.
        // If it grows too long, oldest material freezes.
        // =====================================================

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
            freezeOldestActiveNode();
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


};