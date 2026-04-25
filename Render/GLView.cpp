
#include "GLView.h"
#include <QPainter> // uproszczony render (na start)
#include <QOpenGLContext>

void GLView::setPipe(const PipeAxis3D* p)
{
    pipe = p;
}

void GLView::initializeGL()
{
    QOpenGLContext* ctx = QOpenGLContext::currentContext();

    if (!ctx)
    {
        std::cout << "No OpenGL context!\n";
        return;
    }

    auto loader = [](const char* name) -> void*
    {
        return reinterpret_cast<void*>(
            QOpenGLContext::currentContext()->getProcAddress(name)
        );
    };

    if (!gladLoadGLLoader((GLADloadproc)loader))
    {
        std::cout << "Failed to initialize GLAD\n";
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 1);

}



void GLView::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TEMP TEST: change background slightly every frame (proof GPU works)
    static float t = 0.0f;
    t += 0.01f;

    float r = 0.2f + 0.2f * sin(t);
    glClearColor(r, 0.1f, 0.3f, 1.0f);
}