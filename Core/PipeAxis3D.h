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
        // =====================================================
        // CLEAR PROCEDURAL INPUT
        // =====================================================

        ops.clear();

        // =====================================================
        // CLEAR RECONSTRUCTED GEOMETRY
        // =====================================================

        segments.clear();

        nodes.clear();

        // =====================================================
        // CLEAR FROZEN MANUFACTURING HISTORY
        // =====================================================

        frozenNodes.clear();

        // =====================================================
        // RESET ACTIVE DEFORMATION ZONE
        // =====================================================

        activeZone.localNodes.clear();

        activeZone.accumulatedAngle = 0.0;

        activeZone.curvature = 0.0;

        activeZone.activeLength = 20.0;

        activeZone.active = false;

        // =====================================================
        // RESET RECONSTRUCTION FRAME
        // =====================================================

        currentFrame.P = { 0,0,0 };

        currentFrame.T = { 1,0,0 };
        currentFrame.N = { 0,1,0 };
        currentFrame.B = { 0,0,1 };

        // =====================================================
        // REBUILD REQUIRED
        // =====================================================

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
         // This does NOT execute the program.
         // This does NOT rebuild from operation history.
         //
         // It only assembles:
         //
         // Incoming Stock
         // Active Bend Zone
         // Frozen Geometry
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



    void processBend(double radius, double targetAngle, double angleIncrement)
    {
        // =====================================================
        // OWNER:
        // SimulationController decides WHEN and HOW MUCH.
        // PipeAxis3D owns HOW geometry changes.
        // =====================================================

        if (!activeZone.active)
        {
            beginBend(radius, targetAngle);
        }

        updateActiveZone(angleIncrement);

        if (activeZone.accumulatedAngle >= activeZone.targetAngle)
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

	//Manufacturing-state data
    std::vector<Node> frozenNodes; 
    ActiveZone activeZone;
    IncomingStock incomingStock;
    Frame currentFrame;

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
        // ============================================
        // RESET SIMULATION STATE
        // ============================================

        nodes.clear();

        frozenNodes.clear();

        activeZone.localNodes.clear();

        activeZone.active = false;

        // ============================================
        // RESET FRAME
        // ============================================

        Frame frame;

        frame.P = { 0,0,0 };

        frame.T = { 1,0,0 };
        frame.N = { 0,1,0 };
        frame.B = { 0,0,1 };

        // ============================================
        // INITIAL NODE
        // ============================================

        frozenNodes.push_back({
            frame.P,
            frame.T,
            frame.N,
            frame.B
            });

        // ============================================
        // EXECUTE SEGMENTS
        // ============================================

        for (const auto& seg : segments)
        {
            // ========================================
            // FEED
            // ========================================

            if (seg.type == Segment::LINE)
            {
                buildLine(frame, seg.length);
            }

            // ========================================
            // BEND
            // ========================================

            else if (seg.type == Segment::ARC)
            {
                beginBend(
                    1.0 / seg.curvature,
                    seg.angle
                );

                double stepAngle =
                    seg.curvature * ds;

                while (activeZone.active)
                {
                    updateActiveZone(stepAngle);

                    if (activeZone.accumulatedAngle
                        >= activeZone.targetAngle)
                    {
                        freezeActiveZone();
                    }
                }

                frame = currentFrame;
            }

            // ========================================
            // ROTATE
            // ========================================

            else if (seg.type == Segment::ROTATE)
            {
                buildRotate(frame, seg.rotAngle);
            }
        }

        // ============================================
        // FINAL VISIBLE GEOMETRY
        // ============================================

        nodes = frozenNodes;

        for (const auto& n : activeZone.localNodes)
        {
            nodes.push_back(n);
        }
    }



    void buildLine(Frame& frame, double length)
    {
        int stepCount = std::max(1, (int)(length / ds));

        for (int i = 0; i < stepCount; ++i)
        {
            // Move along current tangent
            frame.P = frame.P + frame.T * ds;
            //nodes.push_back({ frame.P, frame.T });
            frozenNodes.push_back({
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

void beginBend(
        double radius,
        double targetAngle)
    {
        // =====================================================
        // INITIALIZE ACTIVE BEND STATE
        // =====================================================

        activeZone.curvature = 1.0 / radius;

        activeZone.targetAngle = targetAngle;

        activeZone.accumulatedAngle = 0.0;

        activeZone.localNodes.clear();

        activeZone.active = true;

        // =====================================================
        // START FROM CURRENT PIPE END FRAME
        // =====================================================

        if (!nodes.empty())
        {
            const Node& last = nodes.back();

            activeZone.frame.P = last.pos;

            activeZone.frame.T = last.T;
            activeZone.frame.N = last.N;
            activeZone.frame.B = last.B;
        }
        else
        {
            activeZone.frame.P = { 0,0,0 };

            activeZone.frame.T = { 1,0,0 };
            activeZone.frame.N = { 0,1,0 };
            activeZone.frame.B = { 0,0,1 };
        }
    }



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
         // PipeAxis3D owns construction of visible incoming stock.
         //
         // This function is RENDER RECONSTRUCTION ONLY.
         // It does NOT simulate feed.
         // It does NOT change remainingLength.
         // =====================================================

         if (!incomingStock.visible)
             return;

         if (incomingStock.remainingLength <= 0.0)
             return;

         if (ds <= 1e-9)
             return;

         // =====================================================
         // PIPEFLOW VIEW
         //
         // incoming stock is behind the machine entry frame:
         //
         //     tail ---------------------> entry / die
         //
         // Direction:
         //     entryFrame.T points forward through the machine.
         //
         // Therefore incoming stock extends backward:
         //     -entryFrame.T
         // =====================================================

         const Frame& entry = incomingStock.entryFrame;

         double visibleLength = incomingStock.remainingLength;

         int stepCount =
             std::max(1, static_cast<int>(visibleLength / ds));

         Vec3D startPoint =
             entry.P - entry.T * visibleLength;

         // =====================================================
         // Build from stock tail toward machine entry.
         //
         // This keeps node order physically correct:
         //
         // Incoming Stock -> Active Zone -> Frozen Geometry
         // =====================================================

         for (int i = 0; i <= stepCount; ++i)
         {
             double s = std::min(i * ds, visibleLength);

             Vec3D p =
                 startPoint + entry.T * s;

             nodes.push_back({
                 p,
                 entry.T,
                 entry.N,
                 entry.B
                 });
         }
     }
    
   

    void buildNodes()
    {
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

        nodes.clear();

        // =====================================================
        // ZONE 1 — INCOMING STOCK
        //
        // Straight only.
        // Translation/feed only.
        // No curvature.
        // =====================================================

        buildIncomingStock();

        // =====================================================
        // ZONE 2 — ACTIVE BEND WINDOW
        //
        // Local temporary deformation.
        // This is the only deforming area.
        // =====================================================

        for (const auto& node : activeZone.localNodes)
        {
            nodes.push_back(node);
        }

        // =====================================================
        // ZONE 3 — FROZEN GEOMETRY
        //
        // Immutable manufactured pipe.
        // No deformation.
        // =====================================================

        for (const auto& node : frozenNodes)
        {
            nodes.push_back(node);
        }
    }
    
};