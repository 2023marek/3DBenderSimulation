#pragma once

#include <glm/glm.hpp> 
#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>

#include "Core/Geometry/Frame.h"
#include "Core/Geometry/PipeNode.h"
#include "Core/Geometry/PipeSegment.h"
#include "Core/Operations.h"
#include "Core/Math/Vec3D.h"
#include "Core/Manufacturing/RotationKinematicMode.h"
#include "Core/Manufacturing/ManufacturingTypes.h"
#include "Core/Manufacturing/ManufacturingState.h"


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



    // =====================================================
   // TEMPORARY COMPATIBILITY ALIASES
   //
   // Phase 2 extracts shared geometry primitives.
   //
   // Existing code can still use:
   //     PipeAxis3D::Frame
   //     PipeAxis3D::Node
   //     PipeAxis3D::Segment
   //
   // Later phases may replace these names directly.
   // =====================================================

    using Frame = ::Frame;
    using Node = ::PipeNode;
    using Segment = ::PipeSegment;
	using RotationKinematicMode = ::RotationKinematicMode;
    using IncomingStock = ::ManufacturingIncomingStock;
    using PositionedStraight = ::ManufacturingPositionedStraight;
    using ActiveZone = ::ManufacturingActiveZone;
	using ManufacturingRenderData = ::ManufacturingRenderData;
    using ManufacturingState = ::ManufacturingState;
    //====================================================
	// Temporary compatibility aliases
    // 

    // =====================================================================
    // FRAME (3D coordinate system along pipe)
    // =====================================================================
  //  struct Frame
  //  {
  //      Vec3D P;  // Position
  //      Vec3D T;  // Tangent (direction pipe is pointing)
  //      Vec3D N;  // Normal (perpendicular to tangent)
  //      Vec3D B;  // Binormal (perpendicular to both)
 //   };
    //======================================================================
    // OPERATION (User input for pipe construction)
    // INPUT LAYER 
    // =======================================================================
    // 
    
    // =====================================================================
    // NODE (Point on pipe centerline)
    // =====================================================================
   // struct Node
   // {
   //     Vec3D pos;

 //       Vec3D T;
  //      Vec3D N;
  //      Vec3D B;

 //       glm::vec3 getPosition() const
  //      {
  //          return glm::vec3(
  //              (float)pos.x,
   //             (float)pos.y,
  //              (float)pos.z
  //          );
  //      }
  //  };

  //  struct ManufacturingRenderData
   // {
   //     std::vector<Node> incomingStockNodes;
   //     std::vector<Node> positionedStraightNodes;
   //     std::vector<Node> activeZoneNodes;
   //     std::vector<Node> currentBendTraceNodes;
    //    std::vector<Node> frozenNodes;

       // void clear()
       // {
       //     incomingStockNodes.clear();
       //     positionedStraightNodes.clear();
       //     activeZoneNodes.clear();
       //     currentBendTraceNodes.clear();
       //     frozenNodes.clear();
       // }
   // };
  
  //  struct Segment
   // {
     //   enum Type
     //   {
     //       LINE,
     //       ARC,
     //       ROTATE
      //  };

        //Type type = LINE;

        // LINE
        //double length = 0.0;

        // ARC
        //double curvature = 0.0;
        //double angle = 0.0;
        //BendDirection bendDirection = BendDirection::CCW;

        // ROTATE
        //double rotAngle = 0.0;
        //RotationDirection rotationDirection = RotationDirection::CCW;

        //double arcLength() const
        //{
          //  if (std::abs(curvature) < 1e-12)
            //    return 0.0;

            //return angle / std::abs(curvature);
        //}
    //};
    void markGeometryDirty()
    {
        markDirty();
    }
      
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

 //enum class RotationKinematicMode
 //{
     // =====================================================
     
     // The pipe rolls around machineEntryFrame.T.
     // =====================================================
    // PipeRoll,
     // =====================================================
     // Tool / bend plane rotates around the pipe axis.
     // =====================================================
    // ToolHeadRotate
   // };


private:
   // struct ActiveZone
  //  {
    //    Frame frame;

   //     double curvature = 0.0;
    //    double accumulatedAngle = 0.0;
    //    double targetAngle = 0.0;

    //    BendDirection direction = BendDirection::CCW;

    //    double activeLength = 5.0;

    //    std::vector<Node> localNodes;

    //    bool active = false;

    //    size_t frozenCountAtBendStart = 0;
   // };

 // struct IncomingStock
   // {
        // =====================================================
        // OWNER:
        // PipeAxis3D owns incoming stock geometry state.
        //
        // SimulationController only calls processFeed(distance).
        // It does NOT build stock geometry.
        // =====================================================

      //  double totalLength = 300.0;      // full raw stock length
      //  double remainingLength = 300.0;  // stock not yet consumed by machine

		// Material already fed into machine     
       // double consumedLength=0.0;

       // bool visible = true;
    //};
	

     struct LocalFrame
    {
        Vec3D origin;

        Vec3D tangent;
        Vec3D normal;
        Vec3D binormal;
         
       
    };

    // struct PositionedStraight
    // {
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

       //  double length = 0.0;

       //  bool visible = true;

       //  std::vector<Node> nodes;
    // };
   
    
      

    bool isBendActive() const
    {
        return mfg.activeZone.active;
    }
	

    RotationKinematicMode rotationMode =
        RotationKinematicMode::PipeRoll;
	public:

        // =====================================================
        // TEMPORARY MANUFACTURING BEND BRIDGE
        //
        // Used during Phase 5B-3D.
        // ManufacturingPipeSimulator owns processBend(),
        // but PipeAxis3D still owns the bend helper logic.
        // =====================================================

        

       


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
    public:
        // =====================================================
        // CONSTRUCTORS
        // =====================================================

        PipeAxis3D()
            : PipeAxis3D(0.5)
        {
        }

        explicit PipeAxis3D(double stepSize)
            : ds(stepSize),
            dirty(true),
            ownedMfg(),
            mfg(ownedMfg)
        {
            initialize();
        }

        PipeAxis3D(
            double stepSize,
            ManufacturingState& externalState)
            : ds(stepSize),
            dirty(true),
            ownedMfg(),
            mfg(externalState)
        {
            initialize();
        }


// temporary bridge frame accessorsm later for moveout
        const Frame& getMachineEntryFrame() const
        {
            return machineEntryFrame;
        }

        Frame& getMachineEntryFrame()
        {
            return machineEntryFrame;
        }

        const Frame& getCurrentFrame() const
        {
            return currentFrame;
        }

        Frame& getCurrentFrame()
        {
            return currentFrame;
        }

        void setCurrentFrame(const Frame& frame)
        {
            currentFrame = frame;
        }

    //Helper for Bend direction
    double bendDirectionSign(BendDirection dir) const
    {
        return static_cast<int>(dir);
    }
    //Helper for initialize
    
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
    void addBend(double radius, double angle, BendDirection bendDirection = BendDirection::CCW)
    {
        Operation op;
        op.type = Operation::BEND;
        op.R = radius;
        op.angle = angle;
        op.bendDirection;
        ops.push_back(op);
        markDirty();
    }

    // Add rotation (twist)
   // Add rotation / twist operation
   void addRotate(
    double angle,
    RotationDirection rotationDirection = RotationDirection::CCW)
{
    Operation op;

    op.type = Operation::ROTATE;
    op.angle = angle;
    op.rotationDirection = rotationDirection;

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
        // CAD / operation-history data
        // =====================================================

        ops.clear();
        segments.clear();
        nodes.clear();
        cadNodes.clear();

        // =====================================================
        // Manufacturing state
        //
        // ManufacturingState::clear() resets:
        // - incomingStock
        // - positionedStraight
        // - activeZone
        // - currentBendTraceNodes
        // - frozenNodes
        // - renderData
        // =====================================================

        mfg.clear();

        // =====================================================
        // Frames
        // =====================================================

        resetFrames();

        markDirty();
    }

	// =====================================================================
	//GETTERS  
    

   

    
    double getIncomingStockRemainingLength() const
    {
        return mfg.incomingStock.remainingLength;
    }

    double getIncomingStockConsumedLength() const
    {
        return mfg.incomingStock.consumedLength;
    }

    double getIncomingStockTotalLength() const
    {
        return mfg.incomingStock.totalLength;
    }

    RotationKinematicMode getRotationKinematicMode() const
    {
        return rotationMode;
    }

	// =====================================================================
    //SETTERRS
    void setIncomingStockLength(double length)
    {
        if (length <= 0.0)
            return;

        mfg.incomingStock.totalLength = length;
        mfg.incomingStock.remainingLength = length;
        mfg.incomingStock.consumedLength = 0.0;

        markDirty();
    }

    void setRotationKinematicMode(RotationKinematicMode mode)
    {
        rotationMode = mode;
    }

   

    // ====================================================
    // FUNCTION MANUFACTURING
    // 
   
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
            << mfg.frozenNodes.size()
            << std::endl;

        std::cout << "activeNodes: "
            << mfg.activeZone.localNodes.size()
            << std::endl;

        dirty = false;
    }
    // MANUFACTURING API
	//=====================================================================
   


    



  




	
    





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
        return mfg.renderData;
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
    ManufacturingState ownedMfg;
    ManufacturingState& mfg;
    std::vector<Operation> ops;     // INPUT
    std::vector<Segment> segments;  // SIMULATION Operations to execute
    //Render output
    std::vector<Node> nodes;        // RENDER Resulting geometry 
    std::vector<Node> cadNodes;     // CAD Preview geometry

	//Manufacturing-state data
    //std::vector<Node> currentBendTraceNodes;
    //std::vector<Node> frozenNodes; 
   // ActiveZone activeZone;
   // IncomingStock incomingStock;
	
    //PositionedStraight positionedStraight;
    Frame machineEntryFrame;//fixed machine/die entry frame
    Frame currentFrame;//current pipe/material frame
    // Manufacturing render grouping
    //ManufacturingRenderData manufacturingRender;
    
    

    //General
    double ds;                      // Segment step size (mm)
    bool dirty;                     // Rebuild needed?



    
    //Future/CAD derived data -optional for now
	//std::vector<Operation> operationHistory;  // Copy of all operations (for potential future use)
	//std::vector<GeometricSegment> generatedSegments; // Detailed geometric segments (for potential future use)
	//std::vector<Node> sampledNodes; // Sampled nodes along pipe (for potential future use)
 //Helpers
 private:
     void initialize()
     {
         resetFrames();

         mfg.clear();

         dirty = true;
     }

	// GETTERS PRIVATE 
    Frame getPositionedStraightStartFrame() const
    {
        if (mfg.activeZone.active)
        {
            return mfg.activeZone.frame;
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

                if (op.R > 1e-9)
                    s.curvature = 1.0 / op.R;
                else
                    s.curvature = 0.0;

                s.angle = op.angle;
                s.bendDirection = op.bendDirection;
            }
            else if (op.type == Operation::ROTATE)
            {
                s.type = Segment::ROTATE;

                s.rotAngle = op.angle;
                s.rotationDirection = op.rotationDirection;
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
        mfg.activeZone.localNodes.clear();
        mfg.activeZone.active = false;

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


//HELPER FRAMES RESET HELPER

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

    





    

    
    // freezeActive zone
   //order
    
    


  

	//=================================================
    // POSITIONING HELPER
    // HELPER FUNCTION
    
   
    

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

        if (mfg.positionedStraight.length <= 0.0)
            return;

        if (ds <= 1e-9)
            return;

        // Start from the current bend output frame.
        Frame frame = currentFrame;

        int stepCount =
            std::max(
                1,
                static_cast<int>(std::ceil(mfg.positionedStraight.length / ds))
            );

        double stepLength =
            mfg.positionedStraight.length / static_cast<double>(stepCount);

        for (int i = 1; i <= stepCount; ++i)
        {
            double s =
                stepLength * static_cast<double>(i);

            Vec3D p =
                frame.P + frame.T * s;

               mfg.frozenNodes.push_back({
                p,
                frame.T,
                frame.N,
                frame.B
                });
        }

        // Move currentFrame to the end of this straight section.
        currentFrame.P =
            frame.P + frame.T * mfg.positionedStraight.length;

        currentFrame.T = frame.T;
        currentFrame.N = frame.N;
        currentFrame.B = frame.B;

        mfg.positionedStraight.length = 0.0;
        mfg.positionedStraight.nodes.clear();

        std::cout << "[FREEZE POSITIONED STRAIGHT]\n";
    }

	//Positionin Helper

           




    

    

   

    void buildNodes()
    {
        // =====================================================
        // BUILD MANUFACTURING RENDER DATA
        //
        // Zone 1: Incoming Stock
        // Zone 2: Active Bend Zone
        // Zone 3: Frozen Geometry
        // =====================================================

        // =====================================================
        // TEMPORARY LEGACY OUTPUT
        //
        // Current renderer still expects one nodes vector.
        // =====================================================

        
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

        if (mfg.activeZone.localNodes.empty())
            return;

        mfg.frozenNodes.push_back(
            mfg.activeZone.localNodes.front()
        );

        mfg.activeZone.localNodes.erase(
         mfg.activeZone.localNodes.begin()
        );
    }

    

    //HELPER AVOID DUPLICATE NODES

    



   


    //=========

    //HELPER

	// It's connected with freezeOldestActiveNode
    // 

   

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

    for (auto& node : mfg.frozenNodes)
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

        if (mfg.frozenNodes.empty())
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

   

//FROZEN GEOMETRY TRANSFORM

    

    // ADDITIONALLY NEED Update updateActiveZone()
    // IT WAS DONE

    // END -ATTACH  RIGIDLY ZONE 3  TO ACTIVE ZONE 2
//==============================


    //ROTATE RIGID MODULE
    //============================

    //Point rotation around machine axis

    Vec3D rotatePointAroundAxisLine(
        const Vec3D& point,
        const Vec3D& axisPoint,
        const Vec3D& axisDir,
        double angle) const
    {
        // =====================================================
        // OWNER:
        // PipeAxis3D owns geometric transforms.
        //
        // PURPOSE:
        // Rotate a WORLD POINT around a WORLD AXIS LINE.
        //
        // Axis line:
        //     axisPoint + axisDir * t
        //
        // Used for rigid-body ROTATE operation.
        // =====================================================

        Vec3D local =
            point - axisPoint;

        Vec3D rotatedLocal =
            rotateAroundAxis(
                local,
                axisDir,
                angle
            );

        return axisPoint + rotatedLocal;
    }

    //Rigid Node Rotation
     
    void rotateNodeAroundMachineAxis(
        Node& node,
        double angle)
    {
        // =====================================================
        // RIGID BODY ROTATION OF ONE PIPE NODE
        //
        // Position rotates around machine feed axis.
        // Frame vectors rotate around same axis.
        //
        // Shape is not recalculated.
        // This is rigid-body motion.
        // =====================================================

        const Vec3D axisPoint =
            machineEntryFrame.P;

        const Vec3D axisDir =
            machineEntryFrame.T.normalized();

        node.pos =
            rotatePointAroundAxisLine(
                node.pos,
                axisPoint,
                axisDir,
                angle
            );

        node.T =
            rotateAroundAxis(
                node.T,
                axisDir,
                angle
            ).normalized();

        node.N =
            rotateAroundAxis(
                node.N,
                axisDir,
                angle
            ).normalized();

        node.B =
            rotateAroundAxis(
                node.B,
                axisDir,
                angle
            ).normalized();
    }

    //Rigid frame Rotation


    void rotateFrameAroundMachineAxis(
        Frame& frame,
        double angle)
    {
        // =====================================================
        // RIGID BODY ROTATION OF FRAME
        //
        // Used for:
        // - currentFrame
        // - activeZone.frame
        //
        // Position rotates around machine axis.
        // Orientation rotates with the body.
        // =====================================================

        const Vec3D axisPoint =
            machineEntryFrame.P;

        const Vec3D axisDir =
            machineEntryFrame.T.normalized();

        frame.P =
            rotatePointAroundAxisLine(
                frame.P,
                axisPoint,
                axisDir,
                angle
            );

        frame.T =
            rotateAroundAxis(
                frame.T,
                axisDir,
                angle
            ).normalized();

        frame.N =
            rotateAroundAxis(
                frame.N,
                axisDir,
                angle
            ).normalized();

        frame.B =
            rotateAroundAxis(
                frame.B,
                axisDir,
                angle
            ).normalized();

        orthonormalizeFrame(frame);
    }

    //Helper for node vector

    void rotateNodeListAroundMachineAxis(
        std::vector<Node>& list,
        double angle)
    {
        // =====================================================
        // Rotate one manufacturing zone as rigid body.
        // =====================================================

        for (auto& node : list)
        {
            rotateNodeAroundMachineAxis(
                node,
                angle
            );
        }
    }

    //HELPER

    void syncCurrentFrameFromFrozen()
    {
        // =====================================================
        // Keep currentFrame aligned with final frozen node.
        // =====================================================

        if (mfg.frozenNodes.empty())
            return;

        const Node& last =
            mfg.frozenNodes.back();

        currentFrame.P = last.pos;
        currentFrame.T = last.T;
        currentFrame.N = last.N;
        currentFrame.B = last.B;
    }

	//HELPER
	//=====================================================
    //Rotate Tool Only

    void rotateToolPlaneAroundMachineAxis(double angle)
    {
        // =====================================================
        // TOOL-HEAD ROTATION MODE
        //
        // OWNER:
        // PipeAxis3D owns machine/bend-plane frame state.
        //
        // Meaning:
        // - machineEntryFrame.P stays fixed
        // - machineEntryFrame.T stays fixed
        // - N/B rotate around T
        //
        // This changes the next bend plane WITHOUT moving
        // already manufactured pipe geometry.
        // =====================================================

        Vec3D axis =
            machineEntryFrame.T.normalized();

        if (axis.lengthSquared() < 1e-12)
            return;

        machineEntryFrame.N =
            rotateAroundAxis(
                machineEntryFrame.N,
                axis,
                angle
            ).normalized();

        machineEntryFrame.B =
            rotateAroundAxis(
                machineEntryFrame.B,
                axis,
                angle
            ).normalized();

        orthonormalizeFrame(machineEntryFrame);
    }
    //Helper 
    // Rotate Pipe Only Default
    void rotatePipeBodyAroundMachineAxis(double angle)
    {
        // =====================================================
        // PIPE-ROLL MODE
        //
        // OWNER:
        // PipeAxis3D owns pipe manufacturing geometry.
        //
        // Meaning:
        // The already-fed / manufactured pipe rotates as
        // a rigid body around machineEntryFrame.T.
        //
        // IMPORTANT:
        // This function does NOT rotate machineEntryFrame.N/B.
        // The machine bend plane stays fixed.
        //
        // PIPEFLOW:
        //
        // frozen geometry
        // positioned straight
        // active bend trace
        // active window
        //        ?
        // rigid rotation around machineEntryFrame.T
        // =====================================================

        rotateNodeListAroundMachineAxis(
            mfg.frozenNodes,
            angle
        );

        rotateNodeListAroundMachineAxis(
            mfg.currentBendTraceNodes,
            angle
        );

        rotateNodeListAroundMachineAxis(
            mfg.activeZone.localNodes,
            angle
        );

        rotateNodeListAroundMachineAxis(
            mfg.positionedStraight.nodes,
            angle
        );

        // currentFrame belongs to downstream pipe body.
        rotateFrameAroundMachineAxis(
            currentFrame,
            angle
        );

        if (mfg.activeZone.active)
        {
            rotateFrameAroundMachineAxis(
                mfg.activeZone.frame,
                angle
            );
        }

        syncCurrentFrameFromFrozen();
    }
};  