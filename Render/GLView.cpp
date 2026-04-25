#include "GLView.h"
#include <QPainter> // uproszczony render (na start)

void GLView::setPipe(const PipeAxis3D* p)
{
    pipe = p;
}

void GLView::initializeGL()
{
    // tutaj póŸniej: OpenGL init
}

void GLView::paintGL()
{
    QPainter painter(this);

    painter.fillRect(rect(), Qt::black);
    painter.setPen(Qt::green);

    if (!pipe) return;

    // ?? NA RAZIE: prosty debug draw (2D projection)
    const auto& nodes = pipe->getNodes();

    for (size_t i = 1; i < nodes.size(); ++i)
    {
        int x1 = int(nodes[i-1].pos.x);
        int y1 = int(nodes[i-1].pos.y);
        int x2 = int(nodes[i].pos.x);
        int y2 = int(nodes[i].pos.y);

        painter.drawLine(x1, y1, x2, y2);
    }
}