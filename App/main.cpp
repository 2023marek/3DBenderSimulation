#include "../Core/PipeAxis2D.h"
//#include "../Render/GnuplotExporter.h"
#include "../Core/Operations.h"

int main()
{
    PipeAxis2D pipe(10.0);
    
    pipe.addFeed(100);
    pipe.addBend(50, PI/2 , PipeAxis2D::BendDirection::CCW);
    
    pipe.addFeed(80);
    pipe.addBend(40, PI / 2, PipeAxis2D::BendDirection::CCW);
   
   
    // STEP 1: build segments from ops
    pipe.rebuildFromOperations();

    // STEP 2: build nodes
    pipe.build();

    pipe.printSegments();

    // MODIFY (works now)
    pipe.setBendRadius(3, 200.0);
    pipe.setBendAngle(1, PI / 12.0);

    // OR:
    pipe.modifyBend(1, 200.0, PI / 4.0);

    // rebuild nodes only
    pipe.build();

    pipe.printSegments();
    //GnuplotExporter::exportPipe(pipe);
	
   
    return 0;
}