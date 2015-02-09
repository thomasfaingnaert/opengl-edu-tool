#include "scenewidget.h"
#include <QSurfaceFormat>
#include <iostream>

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

    // Print version info
    std::clog << "OpenGL version: " << glGetString(GL_VERSION) <<
                 "\nGLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) <<
                 "\nRenderer: " << glGetString(GL_RENDERER) <<
                 "\nVendor: " << glGetString(GL_VENDOR) << '\n' << std::endl;
}

void SceneWidget::paintGL()
{
}

void SceneWidget::resizeGL(int w, int h)
{

}
