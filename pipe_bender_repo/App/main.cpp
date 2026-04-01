#include "../Core/PipeAxis2D.h"
#include "../Render/GnuplotExporter.h"

int main()
{
    PipeAxis2D pipe(2.0);

    pipe.addFeed(100);
    pipe.addBend(50, PI / 2, BendDirection::CCW);

    pipe.addFeed(80);
    pipe.addBend(40, PI / 3, BendDirection::CW);

    pipe.build();

    GnuplotExporter::exportPipe(pipe, "pipe.dat");

    return 0;
}