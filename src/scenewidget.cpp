#include "scenewidget.h"
#include <QSurfaceFormat>

SceneWidget::SceneWidget(QWidget *parent) :
    QOpenGLWidget(parent)
{
    // Set opengl version & profile
    QSurfaceFormat format;

    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setSamples(4);

    setFormat(format);
}

SceneWidget::~SceneWidget()
{

}

void SceneWidget::initializeGL()
{
    // Init opengl
    initializeOpenGLFunctions();
}

void SceneWidget::paintGL()
{

}

void SceneWidget::resizeGL(int w, int h)
{

}
