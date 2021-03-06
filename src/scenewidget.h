#ifndef SCENEWIDGET_H
#define SCENEWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_2_Core>
#include <QWidget>
#include <string>
#include <glm/glm.hpp>

class SceneWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_2_Core
{
    Q_OBJECT

public:
    SceneWidget(QWidget *parent = 0);
    ~SceneWidget();

    enum class Space
    {
        Model, World, View, NDC, RenderedImage
    };

protected:
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int w, int h);
    virtual void keyPressEvent(QKeyEvent *event);

public slots:
    void setModelScaleX(float val) { m_modelScale.x = val; recalcModelMatrix(); }
    void setModelScaleY(float val) { m_modelScale.y = val; recalcModelMatrix(); }
    void setModelScaleZ(float val) { m_modelScale.z = val; recalcModelMatrix(); }

    void setModelRotateX(float val) { m_modelRotate.x = val; recalcModelMatrix(); }
    void setModelRotateY(float val) { m_modelRotate.y = val; recalcModelMatrix(); }
    void setModelRotateZ(float val) { m_modelRotate.z = val; recalcModelMatrix(); }

    void setModelTranslateX(float val) { m_modelTranslate.x = val; recalcModelMatrix(); }
    void setModelTranslateY(float val) { m_modelTranslate.y = val; recalcModelMatrix(); }
    void setModelTranslateZ(float val) { m_modelTranslate.z = val; recalcModelMatrix(); }

    void setViewPositionX(float val) { m_viewPosition.x = val; recalcViewMatrix(); }
    void setViewPositionY(float val) { m_viewPosition.y = val; recalcViewMatrix(); }
    void setViewPositionZ(float val) { m_viewPosition.z = val; recalcViewMatrix(); }

    void setViewTargetX(float val) { m_viewTarget.x = val; recalcViewMatrix(); }
    void setViewTargetY(float val) { m_viewTarget.y = val; recalcViewMatrix(); }
    void setViewTargetZ(float val) { m_viewTarget.z = val; recalcViewMatrix(); }

    void setViewUpVecX(float val) { m_viewUpVec.x = val; recalcViewMatrix(); }
    void setViewUpVecY(float val) { m_viewUpVec.y = val; recalcViewMatrix(); }
    void setViewUpVecZ(float val) { m_viewUpVec.z = val; recalcViewMatrix(); }

    void setProjectionNear(float val) { m_projectionNear = val; recalcProjectionMatrix(); }
    void setProjectionFar(float val) { m_projectionFar = val; recalcProjectionMatrix(); }
    void setProjectionFov(float val) { m_projectionFov = val; recalcProjectionMatrix(); }


signals:
    void modelMatrixChanged(const glm::mat4 &matrix);
    void viewMatrixChanged(const glm::mat4 &matrix);
    void projectionMatrixChanged(const glm::mat4 &matrix);
    void currentSpaceChanged(const Space space);

private:
    void initProgram();
    std::string getFileContents(const std::string &path) const;
    GLuint compileShader(const std::string &path, GLenum type);
    void checkShaderErrors(GLuint shader, bool isProgram, GLenum param, const std::string &errorMsg);
    void initData();
    void initCubeData();
    void initGridData();
    void initFrustumData();
    void initNdcData();
    void updateFrustumData();
    void updateNdcData();

    void recalcModelMatrix();
    void recalcViewMatrix();
    void recalcProjectionMatrix();
    void updateMvpMatrix();

    GLuint m_program;
    GLuint m_cubeVao;
    GLuint m_cubeVertexDataVbo;
    GLuint m_cubeIndicesVbo;
    GLuint m_gridVao;
    GLuint m_gridVertexDataVbo;
    GLuint m_gridColorDataVbo;
    GLuint m_frustumVao;
    GLuint m_frustumVertexDataVbo;
    GLuint m_frustumColorDataVbo;
    GLuint m_frustumIndicesVbo;
    GLuint m_ndcVao;
    GLuint m_ndcVertexDataVbo;
    GLuint m_ndcIndicesVbo;
    GLuint m_mvpMatrixUnif;

    glm::mat4 m_modelMatrix;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;

    glm::mat4 m_mvpMatrix;
    glm::mat4 m_gridMvpMatrix;
    glm::mat4 m_frustumMvpMatrix;

    constexpr static unsigned numOfVertices = 24;

    glm::vec3 m_modelScale;
    glm::vec3 m_modelRotate;
    glm::vec3 m_modelTranslate;

    glm::vec3 m_viewPosition;
    glm::vec3 m_viewTarget;
    glm::vec3 m_viewUpVec;

    glm::vec3 m_worldCameraPosition;
    glm::vec3 m_worldCameraTarget;
    glm::vec3 m_worldCameraUpVec;

    float m_projectionNear;
    float m_projectionFar;
    float m_projectionFov;

    Space m_currentSpace;
    float m_aspect;
};

#endif // SCENEWIDGET_H
