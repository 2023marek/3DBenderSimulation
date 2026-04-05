#include "../Core/PipeAxis2D.h"
#include "../Render/GnuplotExporter.h"
#include "../Core/Operations.h"

int main()
{
    PipeAxis2D pipe(10.0);

    pipe.addFeed(100);
    pipe.addBend(50, PI/2 , PipeAxis2D::BendDirection::CCW);

    pipe.addFeed(80);
    pipe.addBend(40, PI / 2, PipeAxis2D::BendDirection::CCW);

    pipe.build();

    GnuplotExporter::exportPipe(pipe);
	
   
    return 0;
}