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

class LegacyPipeAxis3D
{
public:
    LegacyPipeAxis3D()
        : LegacyPipeAxis3D(0.5)
    { 
    }

    explicit LegacyPipeAxis3D(double stepSize)
        : ds(stepSize),
        dirty(true)
        
    {
        initialize();
    }

    

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
	//using RotationKinematicMode = ::RotationKinematicMode;
    
    //====================================================
	
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
  

     struct LocalFrame
    {
        Vec3D origin;

        Vec3D tangent;
        Vec3D normal;
        Vec3D binormal;
         
       
    };

   
   
    
      

    

    
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
    ops.clear();
    segments.clear();
    nodes.clear();
    cadNodes.clear();

    resetFrames();

    markDirty();
}

	// =====================================================================
	//GETTERS  
    

   

    
    

    

   
   

	// =====================================================================
    //SETTERRS
    
   

   

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
    //std::vector<Node> currentBendTraceNodes;
    //std::vector<Node> frozenNodes; 
   // ActiveZone activeZone;
   // IncomingStock incomingStock;
	
    //PositionedStraight positionedStraight;
   // !!!!!!Frame machineEntryFrame;//fixed machine/die entry frame
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

         dirty = true;
     }

	// GETTERS PRIVATE 
   




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
        //mfg.activeZone.localNodes.clear();
       // mfg.activeZone.active = false;

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
        currentFrame.P = { 0.0, 0.0, 0.0 };
        currentFrame.T = { 1.0, 0.0, 0.0 };
        currentFrame.N = { 0.0, 1.0, 0.0 };
        currentFrame.B = { 0.0, 0.0, 1.0 };
    }

    





    

    
    // freezeActive zone
   //order
    
    


  

	//=================================================
    // POSITIONING HELPER
    // HELPER FUNCTION
    
   
    

//POSTIONONG HELPER
   // bake positioned stright into frozen geometry 

   

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

      
   
    
};  