#include "../Core/PipeAxis3D.h"
#include "../Render/GnuplotExporter3D.h"
#include "../Core/Operations.h"
int main()
{
    PipeAxis3D pipe(5.0);

    pipe.addFeed(100);
    pipe.addBend(50, PI / 2);
    pipe.addRotate(PI / 2);
    pipe.addFeed(80);
    pipe.addBend(40, PI / 2);

    pipe.build();

    GnuplotExporter::exportPipe3D(pipe);

    return 0;
}