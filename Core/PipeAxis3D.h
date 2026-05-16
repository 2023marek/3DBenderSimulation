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

        double activeLength = 20.0;

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

        // Entry frame of machine / bend die.
        // Incoming stock is drawn behind this frame.
        Frame entryFrame;

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

        incomingStock.entryFrame = currentFrame;
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

        currentFrame.P = { 0,0,0 };
        currentFrame.T = { 1,0,0 };
        currentFrame.N = { 0,1,0 };
        currentFrame.B = { 0,0,1 };

        incomingStock.entryFrame = currentFrame;
        incomingStock.remainingLength = incomingStock.totalLength;

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
        // OWNER:
        // SimulationController decides WHEN feed happens.
        // PipeAxis3D decides WHAT geometric state changes.
        //
        // PIPEFLOW:
        //
        // Incoming Stock  --->  Active Zone  --->  Frozen Geometry
        //
        // FEED:
        // - does NOT create curvature
        // - does NOT create bend geometry
        // - only consumes/translates incoming stock
        // =====================================================

        if (distance <= 0.0)
            return;

        incomingStock.remainingLength -= distance;

        if (incomingStock.remainingLength < 0.0)
            incomingStock.remainingLength = 0.0;

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
                currentFrame,
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
        // PipeAxis3D owns HOW geometric frames are rotated.
        //
        // ROTATE:
        // - does NOT move pipe position
        // - does NOT add nodes
        // - rotates local frame around pipe tangent
        // - changes orientation of the NEXT bend plane
        // =====================================================

        if (std::abs(angleIncrement) < 1e-12)
            return;

        // Rotate manufacturing current frame.
        // This affects the orientation of the next bend.
        buildRotate(currentFrame, angleIncrement);

        // Incoming stock frame should stay consistent with machine entry.
        incomingStock.entryFrame = currentFrame;

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
    Frame currentFrame;
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
        // IMPORTANT:
        // Bend starts from explicit frame, not from nodes.back().
        // This prevents bends from jumping back near origin.
        // =====================================================

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
        // incoming stock
        //        ?
        // local curvature evolution
        //        ?
        // material exits bend die
        //        ?
        // geometry freezes
        //
        // ONLY THIS LOCAL REGION DEFORMS
        // ==========================================================


        // ==========================================================
        // STEP 1
        // Stop if bend already complete
        // ==========================================================

        if (activeZone.accumulatedAngle >= activeZone.targetAngle)
            return;


        // ==========================================================
        // STEP 2
        // Clamp angle increment
        //
        // Prevent overshooting target bend angle
        // ==========================================================

        double remaining =
            activeZone.targetAngle
            - activeZone.accumulatedAngle;

        double dA = std::min(stepAngle, remaining);


        // ==========================================================
        // STEP 3
        // Save previous tangent
        //
        // Needed for minimal frame transport
        // ==========================================================

        Vec3D prevT = activeZone.frame.T;


        // ==========================================================
        // STEP 4
        // Curvature propagation
        //
        // Pipe locally bends around current binormal
        //
        // This is the actual geometric deformation step
        // ==========================================================

        activeZone.frame.T =
            rotateAroundAxis(
                activeZone.frame.T,
                activeZone.frame.B,
                dA
            );


        // ==========================================================
        // STEP 5
        // Minimal rotation frame transport
        //
        // Keeps local frame stable during curvature evolution
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
        // More numerically stable than forward Euler
        // ==========================================================

        Vec3D midT =
            normalize(prevT + activeZone.frame.T);


        // ==========================================================
        // STEP 7
        // Advance material through bend die
        //
        // This represents pipe feed through active deformation zone
        // ==========================================================

        activeZone.frame.P =
            activeZone.frame.P
            + midT * ds;


        // ==========================================================
        // STEP 8
        // Store local active geometry
        //
        // IMPORTANT:
        // This is NOT frozen geometry yet
        // ==========================================================

        activeZone.localNodes.push_back({
            activeZone.frame.P,
            activeZone.frame.T,
            activeZone.frame.N,
            activeZone.frame.B
            });


        // ==========================================================
        // STEP 9
        // Accumulate bend progress
        // ==========================================================

        activeZone.accumulatedAngle += dA;


        // ==========================================================
// STEP 10
// Maintain fixed active deformation window
//
// Active zone must NOT grow forever.
//
// As new material enters:
// oldest material exits
// ? becomes frozen geometry
// ==========================================================

        double currentArcLength =
            activeZone.accumulatedAngle
            / activeZone.curvature;

        // ==========================================================
        // Calculate maximum node count allowed
        // ==========================================================

        size_t maxActiveNodes =
            static_cast<size_t>(
                activeZone.activeLength / ds);

        // Safety
        maxActiveNodes = std::max<size_t>(2, maxActiveNodes);

        // ==========================================================
        // If active zone becomes too large:
        //
        // oldest node exits deformation zone
        // ? freeze it
        // ==========================================================

        while (activeZone.localNodes.size() > maxActiveNodes)
        {
            // ==============================================
            // Move oldest deforming node into frozen history
            // ==============================================

            frozenNodes.push_back(
                activeZone.localNodes.front());

            // ==============================================
            // Remove from active deformation zone
            // ==============================================

            activeZone.localNodes.erase(
                activeZone.localNodes.begin());
        }
    }

    



    void freezeActiveZone()
    {
        // =====================================================
        // MOVE ACTIVE GEOMETRY INTO FROZEN HISTORY
        // =====================================================

        for (const auto& node : activeZone.localNodes)
        {
             frozenNodes.push_back(node);
        }

        // =====================================================
        // CLEAR ACTIVE GEOMETRY
        // =====================================================

        activeZone.localNodes.clear();

        // =====================================================
        // DEACTIVATE
        // =====================================================

        activeZone.active = false;

        // =====================================================
        // COMMIT FINAL FRAME
        //
        // Future geometry starts from here
        // =====================================================

        currentFrame = activeZone.frame;
    }


    void buildIncomingStock()
    {
        // =====================================================
        // OWNER:
        // PipeAxis3D owns construction of incoming stock render data.
        //
        // OUTPUT:
        // manufacturingRender.incomingStockNodes
        //
        // IMPORTANT:
        // This function does NOT simulate FEED.
        // It only rebuilds visible stock geometry from current state.
        // =====================================================

        manufacturingRender.incomingStockNodes.clear();

        if (!incomingStock.visible)
            return;

        if (incomingStock.remainingLength <= 0.0)
            return;

        if (ds <= 1e-9)
            return;

        // =====================================================
        // PIPEFLOW VIEW
        //
        // tail ---------------------> entry / die
        //
        // entryFrame.T points forward through the machine.
        // Incoming stock extends backward from entryFrame.P.
        // =====================================================

        const Frame& entry = incomingStock.entryFrame;

        double visibleLength = incomingStock.remainingLength;

        int stepCount =
            std::max(
                1,
                static_cast<int>(std::ceil(visibleLength / ds))
            );

        double stepLength =
            visibleLength / static_cast<double>(stepCount);

        Vec3D startPoint =
            entry.P - entry.T * visibleLength;

        for (int i = 0; i <= stepCount; ++i)
        {
            double s =
                stepLength * static_cast<double>(i);

            Vec3D p =
                startPoint + entry.T * s;

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
        // Later:
        // renderer should draw each group separately:
        // incomingStockNodes, activeZoneNodes, frozenNodes.
        // =====================================================

        nodes.clear();

        // =====================================================
        // WARNING:
        // Flattening can create artificial connector lines
        // between zones if renderer uses GL_LINE_STRIP.
        //
        // This is temporary until renderer supports zone groups.
        // =====================================================

        for (const auto& node : manufacturingRender.incomingStockNodes)
        {
            nodes.push_back(node);
        }

        for (const auto& node : manufacturingRender.activeZoneNodes)
        {
            nodes.push_back(node);
        }

        for (const auto& node : manufacturingRender.frozenNodes)
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
    
};