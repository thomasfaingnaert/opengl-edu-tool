#include "scenewidget.h"
#include <QSurfaceFormat>
#include <QKeyEvent>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

SceneWidget::SceneWidget(QWidget *parent) :
    QOpenGLWidget(parent), m_modelScale(1.0f, 1.0f, 1.0f), m_viewPosition(10.0f, 10.0f, 10.0f), m_viewTarget(0.0f, 0.0f, -1.0f), m_viewUpVec(0.0f, 1.0f, 0.0f), m_currentSpace(Space::Model)
{
    // Set opengl version & profile
    QSurfaceFormat format;

    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setSamples(4);

    setFormat(format);
    setFocusPolicy(Qt::StrongFocus);
}

SceneWidget::~SceneWidget()
{

}

void SceneWidget::initializeGL()
{
    // Init opengl
    bool success = initializeOpenGLFunctions();

    // Check if opengl init was successfull
    if (!success)
        throw std::runtime_error("Could not load OpenGL functions.\nDo you have OpenGL v3.2?");

    // Print version info
    std::clog << "OpenGL version: " << glGetString(GL_VERSION) <<
                 "\nGLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) <<
                 "\nRenderer: " << glGetString(GL_RENDERER) <<
                 "\nVendor: " << glGetString(GL_VENDOR) << '\n' << std::endl;

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClearDepth(1.0f);

    // Enable face culling
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glDepthRange(0.0f, 1.0f);

    // Init
    initProgram();
    initData();

    // Update matrices
    recalcModelMatrix();
    recalcViewMatrix();
}

void SceneWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_program);

    // Draw cube
    glBindVertexArray(m_cubeVao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(0));

    // Use grid mvp matrix
    glUniformMatrix4fv(m_mvpMatrixUnif, 1, GL_FALSE, glm::value_ptr(m_gridMvpMatrix));

    // Draw grid
    glBindVertexArray(m_gridVao);
    glDrawArrays(GL_LINES, 0, 86);

    // Restore old mvp matrix
    glUniformMatrix4fv(m_mvpMatrixUnif, 1, GL_FALSE, glm::value_ptr(m_mvpMatrix));

    // Cleanup
    glBindVertexArray(0);
    glUseProgram(0);
}

void SceneWidget::resizeGL(int w, int h)
{
    // Adjust viewport
    glViewport(0, 0, w, h);

    // Adjust perspective matrix
    m_aspect = static_cast<float>(w) / h;

    m_projectionMatrix = glm::perspective(glm::radians(90.0f), m_aspect, 0.1f, 30.0f);

    updateMvpMatrix();
}

void SceneWidget::initProgram()
{
    GLuint vs = compileShader("../res/shader.vert", GL_VERTEX_SHADER);
    GLuint fs = compileShader("../res/shader.frag", GL_FRAGMENT_SHADER);

    m_program = glCreateProgram();

    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);

    glLinkProgram(m_program);
    checkShaderErrors(m_program, true, GL_LINK_STATUS, "Could not link program");

    glValidateProgram(m_program);
    checkShaderErrors(m_program, true, GL_VALIDATE_STATUS, "Could not validate program");

    glDetachShader(m_program, vs);
    glDetachShader(m_program, fs);

    glDeleteShader(vs);
    glDeleteShader(fs);

    // Load uniforms
    m_mvpMatrixUnif = glGetUniformLocation(m_program, "mvpMatrix");
}

std::string SceneWidget::getFileContents(const std::string &path) const
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Could not open file: " + path);

    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

void SceneWidget::checkShaderErrors(GLuint shader, bool isProgram, GLenum param, const std::string &errorMsg)
{
    GLint status;
    if (isProgram)
        glGetProgramiv(shader, param, &status);
    else
        glGetShaderiv(shader, param, &status);

    if (status)
        return; // No errors

    GLint logLength;
    if (isProgram)
        glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    else
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    GLchar *buffer = new GLchar[logLength];
    if (isProgram)
        glGetProgramInfoLog(shader, logLength, nullptr, buffer);
    else
        glGetShaderInfoLog(shader, logLength, nullptr, buffer);


    std::ostringstream msgStream;
    msgStream << errorMsg << ":\n" << buffer;
    std::string msg = msgStream.str();

    delete[] buffer;
    throw std::runtime_error(msg);
}

GLuint SceneWidget::compileShader(const std::string &path, GLenum type)
{
    std::string typeStr;
    switch (type)
    {
    case GL_VERTEX_SHADER:
        typeStr = "vertex";
        break;
    case GL_FRAGMENT_SHADER:
        typeStr = "fragment";
        break;
    default:
        typeStr = "unknown";
        break;
    }

    GLuint shader = glCreateShader(type);

    std::string source = getFileContents(path);
    const GLchar *sourceCStr = source.c_str();

    glShaderSource(shader, 1, &sourceCStr, nullptr);
    glCompileShader(shader);
    checkShaderErrors(shader, false, GL_COMPILE_STATUS, "Could not compile " + typeStr + " shader");

    return shader;
}

void SceneWidget::initData()
{
    initCubeData();
    initGridData();
}

void SceneWidget::initGridData()
{
    // Grid constants
    const int size = 20;
    const float delta = 1.0f;

    // Data
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> colours;

    // z-aligned line end points
    for (int x = -size/2; x <= size/2; x += delta)
    {
        // vertices
        vertices.push_back(x);
        vertices.push_back(0);
        vertices.push_back(delta * size / 2.0f);

        vertices.push_back(x);
        vertices.push_back(0);
        vertices.push_back(- delta * size / 2.0f);

        // colours
        if (x != 0)
        {
            colours.push_back(0.0f);
            colours.push_back(0.0f);
            colours.push_back(0.0f);

            colours.push_back(0.0f);
            colours.push_back(0.0f);
            colours.push_back(0.0f);
        }
        else // z-axis
        {
            colours.push_back(0.0f);
            colours.push_back(0.0f);
            colours.push_back(1.0f);

            colours.push_back(0.0f);
            colours.push_back(0.0f);
            colours.push_back(1.0f);
        }
    }

    // x-aligned line end points
    for (int z = -size/2; z <= size/2; z += delta)
    {
        // vertices
        vertices.push_back(delta * size / 2.0f);
        vertices.push_back(0);
        vertices.push_back(z);

        vertices.push_back(- delta * size / 2.0f);
        vertices.push_back(0);
        vertices.push_back(z);

        // colours
        if (z != 0)
        {
            colours.push_back(0.0f);
            colours.push_back(0.0f);
            colours.push_back(0.0f);

            colours.push_back(0.0f);
            colours.push_back(0.0f);
            colours.push_back(0.0f);
        }
        else // x-axis
        {
            colours.push_back(1.0f);
            colours.push_back(0.0f);
            colours.push_back(0.0f);

            colours.push_back(1.0f);
            colours.push_back(0.0f);
            colours.push_back(0.0f);
        }
    }

    // y-axis vertices
    vertices.push_back(0);
    vertices.push_back(size / 2.0f);
    vertices.push_back(0);

    vertices.push_back(0);
    vertices.push_back(- size / 2.0f);
    vertices.push_back(0);

    // y-axis colours
    colours.push_back(0.0f);
    colours.push_back(1.0f);
    colours.push_back(0.0f);

    colours.push_back(0.0f);
    colours.push_back(1.0f);
    colours.push_back(0.0f);

    // Create and bind vao
    glGenVertexArrays(1, &m_gridVao);
    glBindVertexArray(m_gridVao);

    // Create and bind position vbo
    glGenBuffers(1, &m_gridVertexDataVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_gridVertexDataVbo);

    // Fill position buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    // Position attrib
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    // Create and bind colour vbo
    glGenBuffers(1, &m_gridColorDataVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_gridColorDataVbo);

    // Fill colour buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * colours.size(), colours.data(), GL_STATIC_DRAW);

    // Colour attrib
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    // Cleanup
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SceneWidget::initCubeData()
{
    // Data
    GLfloat data[] =
    {
        /* POSITIONS */

        // Front face
        -1.0f, -1.0f, +1.0f,
        -1.0f, +1.0f, +1.0f,
        +1.0f, +1.0f, +1.0f,
        +1.0f, -1.0f, +1.0f,

        // Right face
        +1.0f, -1.0f, +1.0f,
        +1.0f, +1.0f, +1.0f,
        +1.0f, +1.0f, -1.0f,
        +1.0f, -1.0f, -1.0f,

        // Top face
        -1.0f, +1.0f, +1.0f,
        -1.0f, +1.0f, -1.0f,
        +1.0f, +1.0f, -1.0f,
        +1.0f, +1.0f, +1.0f,

        // Back face
        -1.0f, -1.0f, -1.0f,
        -1.0f, +1.0f, -1.0f,
        +1.0f, +1.0f, -1.0f,
        +1.0f, -1.0f, -1.0f,

        // Left face
        -1.0f, -1.0f, +1.0f,
        -1.0f, +1.0f, +1.0f,
        -1.0f, +1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,

        // Bottom face
        -1.0f, -1.0f, +1.0f,
        -1.0f, -1.0f, -1.0f,
        +1.0f, -1.0f, -1.0f,
        +1.0f, -1.0f, +1.0f,

        /* COLORS */

        // Front face
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

        // Right face
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,

        // Top face
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,

        // Back face
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 1.0f,

        // Left face
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f,

        // Bottom face
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
    };

    // Indices
    GLushort indices[] =
    {
        // Front face
        0, 1, 2,
        0, 2, 3,

        // Right face
        4, 5, 6,
        4, 6, 7,

        // Top face
        8, 9, 10,
        8, 10, 11,

        // Back face
        12, 14, 13,
        12, 15, 14,

        // Left face
        16, 18, 17,
        16, 19, 18,

        // Bottom face
        20, 22, 21,
        20, 23, 22,
    };

    // Create and bind vao
    glGenVertexArrays(1, &m_cubeVao);
    glBindVertexArray(m_cubeVao);

    // Create and bind position vbo
    glGenBuffers(1, &m_cubeVertexDataVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexDataVbo);

    // Create and bind indices vbo
    glGenBuffers(1, &m_cubeIndicesVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndicesVbo);

    // Fill position buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof data, data, GL_STATIC_DRAW);

    // Position attrib
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    // Color attrib
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(numOfVertices * 3 * sizeof(GLfloat)));

    // Fill indices buffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices, GL_STATIC_DRAW);

    // Cleanup
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SceneWidget::recalcModelMatrix()
{
    m_modelMatrix = glm::mat4();
    m_modelMatrix *= glm::translate(glm::mat4(), m_modelTranslate);
    m_modelMatrix *= glm::rotate(glm::mat4(), glm::radians(m_modelRotate.z), glm::vec3(0, 0, 1));
    m_modelMatrix *= glm::rotate(glm::mat4(), glm::radians(m_modelRotate.y), glm::vec3(0, 1, 0));
    m_modelMatrix *= glm::rotate(glm::mat4(), glm::radians(m_modelRotate.x), glm::vec3(1, 0, 0));
    m_modelMatrix *= glm::scale(glm::mat4(), m_modelScale);

    emit modelMatrixChanged(m_modelMatrix);
    updateMvpMatrix();
}

void SceneWidget::recalcViewMatrix()
{
    m_viewMatrix = glm::lookAt(m_viewPosition, m_viewTarget, m_viewUpVec);
    emit viewMatrixChanged(m_viewMatrix);
    updateMvpMatrix();
}

void SceneWidget::updateMvpMatrix()
{
    switch (m_currentSpace)
    {
    case Space::RenderedImage:
    {
        m_gridMvpMatrix = m_projectionMatrix * m_viewMatrix;
        m_mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
        break;
    }
    case Space::View:
    {
        const glm::mat4 perspective = glm::perspective(glm::radians(90.0f), m_aspect, 0.1f, 30.0f);
        m_gridMvpMatrix = perspective * m_viewMatrix;
        m_mvpMatrix = perspective * m_viewMatrix * m_modelMatrix;
        break;
    }
    case Space::World:
    {
        const glm::mat4 view = glm::lookAt(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 perspective = glm::perspective(glm::radians(90.0f), m_aspect, 0.1f, 30.0f);
        m_gridMvpMatrix = perspective * view;
        m_mvpMatrix = perspective * view * m_modelMatrix;
        break;
    }
    case Space::Model:
    {
        const glm::mat4 view = glm::lookAt(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 perspective = glm::perspective(glm::radians(90.0f), m_aspect, 0.1f, 30.0f);
        m_gridMvpMatrix = perspective * view;
        m_mvpMatrix = perspective * view;
        break;
    }
    default:
        throw std::runtime_error("Unknown space");
    }


    glUseProgram(m_program);
    glUniformMatrix4fv(m_mvpMatrixUnif, 1, GL_FALSE, glm::value_ptr(m_mvpMatrix));
    glUseProgram(0);
    update();
}

void SceneWidget::keyPressEvent(QKeyEvent *event)
{
    constexpr float scale = 1.0f;
    glm::vec3 forward = glm::normalize(m_viewTarget - m_viewPosition);
    glm::vec3 right = glm::normalize(glm::cross(forward, m_viewUpVec));
    glm::vec3 upward = glm::cross(right, forward);

    switch (event->key())
    {
    case Qt::Key_Z:
        m_viewPosition += scale * forward;
        m_viewTarget += scale * forward;
        recalcViewMatrix();
        break;

    case Qt::Key_S:
        m_viewPosition -= scale * forward;
        m_viewTarget -= scale * forward;
        recalcViewMatrix();
        break;

    case Qt::Key_D:
        m_viewPosition += scale * right;
        m_viewTarget += scale * right;
        recalcViewMatrix();
        break;

    case Qt::Key_Q:
        m_viewPosition -= scale * right;
        m_viewTarget -= scale * right;
        recalcViewMatrix();
        break;

    case Qt::Key_A:
        m_viewPosition += scale * upward;
        m_viewTarget += scale * upward;
        recalcViewMatrix();
        break;

    case Qt::Key_W:
        m_viewPosition -= scale * upward;
        m_viewTarget -= scale * upward;
        recalcViewMatrix();
        break;

    case Qt::Key_0:
        m_currentSpace = Space::Model;
        updateMvpMatrix();
        break;

    case Qt::Key_1:
        m_currentSpace = Space::World;
        updateMvpMatrix();
        break;

    case Qt::Key_2:
        m_currentSpace = Space::View;
        updateMvpMatrix();
        break;

    case Qt::Key_3:
        m_currentSpace = Space::RenderedImage;
        updateMvpMatrix();
        break;

    default:
        QOpenGLWidget::keyPressEvent(event);
    }
}
