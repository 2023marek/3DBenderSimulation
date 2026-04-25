#pragma once
#include <QOpenGLWidget>
#include "Core/PipeAxis3D.h"

// Widget renderuj¹cy (nic nie wie o symulacji!)
class GLView : public QOpenGLWidget
{
    Q_OBJECT

public:
    void setPipe(const PipeAxis3D* pipe);

protected:
    void initializeGL() override;
    void paintGL() override;

private:
    const PipeAxis3D* pipe = nullptr;
};