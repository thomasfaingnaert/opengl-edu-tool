#ifndef SCENEWIDGET_H
#define SCENEWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_2_Core>
#include <QWidget>
#include <string>
#include <glm/glm.hpp>

class SceneWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_2_Core
{
public:
    SceneWidget(QWidget *parent = 0);
    ~SceneWidget();

protected:
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int w, int h);

private:
    void initProgram();
    std::string getFileContents(const std::string &path) const;
    GLuint compileShader(const std::string &path, GLenum type);
    void checkShaderErrors(GLuint shader, bool isProgram, GLenum param, const std::string &errorMsg);
    void initData();

    GLuint m_program;
    GLuint m_vao;
    GLuint m_positionVbo;
    GLuint m_indicesVbo;
    GLuint m_mvpMatrixUnif;
    glm::mat4 m_mvpMatrix;
};

#endif // SCENEWIDGET_H
