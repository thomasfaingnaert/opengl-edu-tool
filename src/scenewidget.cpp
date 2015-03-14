#include "scenewidget.h"
#include <QSurfaceFormat>
#include <QKeyEvent>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iterator>

SceneWidget::SceneWidget(QWidget *parent) :
    QOpenGLWidget(parent), m_modelScale(1.0f, 1.0f, 1.0f), m_viewPosition(10.0f, 10.0f, 10.0f), m_viewTarget(0.0f, 0.0f, 0.0f), m_viewUpVec(0.0f, 1.0f, 0.0f), m_currentSpace(Space::Model),
    m_worldCameraPosition(10.0f, 10.0f, 10.0f), m_worldCameraTarget(0.0f, 0.0f, 0.0f), m_worldCameraUpVec(0.0f, 1.0f, 0.0f), m_projectionNear(0.1f), m_projectionFar(30.0f), m_projectionFov(90.0f)
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
    recalcProjectionMatrix();
}

void SceneWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_program);

    // Draw cube (if not in ndc space)
    if (m_currentSpace != Space::NDC)
    {
        glBindVertexArray(m_cubeVao);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(0));
    }

    // Draw NDC coords (only in ndc space)
    else
    {
        glBindVertexArray(m_ndcVao);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(0));
    }

    // Use grid mvp matrix
    glUniformMatrix4fv(m_mvpMatrixUnif, 1, GL_FALSE, glm::value_ptr(m_gridMvpMatrix));

    // Draw grid
    glBindVertexArray(m_gridVao);
    glDrawArrays(GL_LINES, 0, 86);

    // Use frustum mvp matrix
    glUniformMatrix4fv(m_mvpMatrixUnif, 1, GL_FALSE, glm::value_ptr(m_frustumMvpMatrix));

    // Draw frustum (only in world space)
    if (m_currentSpace == Space::World)
    {
        glBindVertexArray(m_frustumVao);
        glDrawElements(GL_LINES, 32, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(0));
    }

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

    m_projectionMatrix = glm::perspective(glm::radians(m_projectionFov), m_aspect, m_projectionNear, m_projectionFar);

    // Adjust frustum
    updateFrustumData();

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
    initFrustumData();
    initNdcData();
}

void SceneWidget::updateFrustumData()
{
    // Recalculate data
    const float nearPlane = -m_projectionNear;
    const float farPlane = -m_projectionFar;
    const float height = 2 * nearPlane * tan(glm::radians(m_projectionFov / 2.0f));
    const float width = height * m_aspect;

    const float leftNear = -width / 2.0f;
    const float rightNear = width / 2.0f;
    const float bottomNear = -height / 2.0f;
    const float topNear = height / 2.0f;

    const float leftFar = leftNear * farPlane / nearPlane;
    const float rightFar = rightNear * farPlane / nearPlane;
    const float bottomFar = bottomNear * farPlane / nearPlane;
    const float topFar = topNear * farPlane / nearPlane;

    const GLfloat vertices[] =
    {
        0.0f, 0.0f, 0.0f,

        leftNear, topNear, nearPlane,
        rightNear, topNear, nearPlane,
        rightNear, bottomNear, nearPlane,
        leftNear, bottomNear, nearPlane,

        leftFar, topFar, farPlane,
        rightFar, topFar, farPlane,
        rightFar, bottomFar, farPlane,
        leftFar, bottomFar, farPlane,

        leftNear, topNear, nearPlane,
        rightNear, topNear, nearPlane,
        rightNear, bottomNear, nearPlane,
        leftNear, bottomNear, nearPlane,
    };

    // Update vertex VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_frustumVertexDataVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof vertices, vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SceneWidget::initFrustumData()
{
    const GLfloat colours[] =
    {
        0.50f, 0.50f, 0.50f,

        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,

        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,

        0.50f, 0.50f, 0.50f,
        0.50f, 0.50f, 0.50f,
        0.50f, 0.50f, 0.50f,
        0.50f, 0.50f, 0.50f,
    };

    const GLushort indices[] =
    {
        1, 5, 2, 6, 3, 7, 4, 8,
        1, 2, 3, 4, 5, 6, 7, 8,
        1, 4, 2, 3, 5, 8, 6, 7,
        0, 9, 0, 10, 0, 11, 0, 12,
    };

    // Create and bind vao
    glGenVertexArrays(1, &m_frustumVao);
    glBindVertexArray(m_frustumVao);

    // Create and bind position vbo
    glGenBuffers(1, &m_frustumVertexDataVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_frustumVertexDataVbo);

    // Fill position buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(colours), nullptr, GL_STATIC_DRAW);

    // Position attrib
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    // Create and bind colour vbo
    glGenBuffers(1, &m_frustumColorDataVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_frustumColorDataVbo);

    // Fill colour buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof colours, colours, GL_STATIC_DRAW);

    // Colour attrib
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    // Create and bind index buffer
    glGenBuffers(1, &m_frustumIndicesVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_frustumIndicesVbo);

    // Fill indices buffer
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices, GL_STATIC_DRAW);

    // Cleanup
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void SceneWidget::updateNdcData()
{
    // Data
    std::vector<glm::vec3> data =
    {
        /* POSITIONS */

        // Front face
        glm::vec3(-1.0f, -1.0f, +1.0f),
        glm::vec3(-1.0f, +1.0f, +1.0f),
        glm::vec3(+1.0f, +1.0f, +1.0f),
        glm::vec3(+1.0f, -1.0f, +1.0f),

        // Right face
        glm::vec3(+1.0f, -1.0f, +1.0f),
        glm::vec3(+1.0f, +1.0f, +1.0f),
        glm::vec3(+1.0f, +1.0f, -1.0f),
        glm::vec3(+1.0f, -1.0f, -1.0f),

        // Top face
        glm::vec3(-1.0f, +1.0f, +1.0f),
        glm::vec3(-1.0f, +1.0f, -1.0f),
        glm::vec3(+1.0f, +1.0f, -1.0f),
        glm::vec3(+1.0f, +1.0f, +1.0f),

        // Back face
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f, +1.0f, -1.0f),
        glm::vec3(+1.0f, +1.0f, -1.0f),
        glm::vec3(+1.0f, -1.0f, -1.0f),

        // Left face
        glm::vec3(-1.0f, -1.0f, +1.0f),
        glm::vec3(-1.0f, +1.0f, +1.0f),
        glm::vec3(-1.0f, +1.0f, -1.0f),
        glm::vec3(-1.0f, -1.0f, -1.0f),

        // Bottom face
        glm::vec3(-1.0f, -1.0f, +1.0f),
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(+1.0f, -1.0f, -1.0f),
        glm::vec3(+1.0f, -1.0f, +1.0f),
    };

    // Loop through every coordinate
    for (glm::vec3 &vertex : data)
    {
        // Convert to vec4
        glm::vec4 vert4(vertex, 1.0f);

        // Bring vertex in clip space
        vert4 = m_projectionMatrix * m_viewMatrix * m_modelMatrix * vert4;

        // Bring vertex into ndc-space
        vert4 /= vert4.w;

        // Convert back to vec3
        vertex.x = vert4.x;
        vertex.y = vert4.y;
        vertex.z = vert4.z;
    }

    // Update buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_ndcVertexDataVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, 3 * sizeof(GLfloat) * data.size(), data.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SceneWidget::initNdcData()
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
    glGenVertexArrays(1, &m_ndcVao);
    glBindVertexArray(m_ndcVao);

    // Create and bind position vbo
    glGenBuffers(1, &m_ndcVertexDataVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_ndcVertexDataVbo);

    // Fill position vbo
    glBufferData(GL_ARRAY_BUFFER, sizeof data, data, GL_STATIC_DRAW);

    // Create and bind indices vbo
    glGenBuffers(1, &m_ndcIndicesVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ndcIndicesVbo);

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
            colours.push_back(0.75f);
            colours.push_back(0.75f);
            colours.push_back(0.75f);

            colours.push_back(0.75f);
            colours.push_back(0.75f);
            colours.push_back(0.75f);
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
            colours.push_back(0.75f);
            colours.push_back(0.75f);
            colours.push_back(0.75f);

            colours.push_back(0.75f);
            colours.push_back(0.75f);
            colours.push_back(0.75f);
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

void SceneWidget::recalcProjectionMatrix()
{
    m_projectionMatrix = glm::perspective(glm::radians(m_projectionFov), m_aspect, m_projectionNear, m_projectionFar);
    updateFrustumData();
    emit projectionMatrixChanged(m_projectionMatrix);
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
        m_frustumMvpMatrix = m_projectionMatrix * m_viewMatrix * glm::inverse(m_viewMatrix);
        break;
    }
    case Space::NDC:
    {
        const glm::mat4 view = glm::lookAt(m_worldCameraPosition, m_worldCameraTarget, m_worldCameraUpVec);
        const glm::mat4 perspective = glm::perspective(glm::radians(90.0f), m_aspect, 0.1f, 200.0f);
        m_gridMvpMatrix = perspective * view;
        m_mvpMatrix = perspective * view;
        m_frustumMvpMatrix = perspective * view * glm::inverse(m_viewMatrix);
        break;
    }
    case Space::View:
    {
        const glm::mat4 perspective = glm::perspective(glm::radians(90.0f), m_aspect, 0.1f, 200.0f);
        m_gridMvpMatrix = perspective * m_viewMatrix;
        m_mvpMatrix = perspective * m_viewMatrix * m_modelMatrix;
        m_frustumMvpMatrix = perspective * m_viewMatrix * glm::inverse(m_viewMatrix);
        break;
    }
    case Space::World:
    {
        const glm::mat4 view = glm::lookAt(m_worldCameraPosition, m_worldCameraTarget, m_worldCameraUpVec);
        const glm::mat4 perspective = glm::perspective(glm::radians(90.0f), m_aspect, 0.1f, 200.0f);
        m_gridMvpMatrix = perspective * view;
        m_mvpMatrix = perspective * view * m_modelMatrix;
        m_frustumMvpMatrix = perspective * view * glm::inverse(m_viewMatrix);
        break;
    }
    case Space::Model:
    {
        const glm::mat4 view = glm::lookAt(m_worldCameraPosition, m_worldCameraTarget, m_worldCameraUpVec);
        const glm::mat4 perspective = glm::perspective(glm::radians(90.0f), m_aspect, 0.1f, 200.0f);
        m_gridMvpMatrix = perspective * view;
        m_mvpMatrix = perspective * view;
        m_frustumMvpMatrix = perspective * view * glm::inverse(m_viewMatrix);
        break;
    }
    default:
        throw std::runtime_error("Unknown space");
    }


    glUseProgram(m_program);
    glUniformMatrix4fv(m_mvpMatrixUnif, 1, GL_FALSE, glm::value_ptr(m_mvpMatrix));
    glUseProgram(0);
    updateNdcData();
    update();
}

void SceneWidget::keyPressEvent(QKeyEvent *event)
{
    constexpr float scale = 1.0f;
    glm::vec3 forward = glm::normalize(m_worldCameraTarget - m_worldCameraPosition);
    glm::vec3 right = glm::normalize(glm::cross(forward, m_worldCameraUpVec));
    glm::vec3 upward = glm::cross(right, forward);

    switch (event->key())
    {
    case Qt::Key_Z:
        m_worldCameraPosition += scale * forward;
        m_worldCameraTarget += scale * forward;
        updateMvpMatrix();
        break;

    case Qt::Key_S:
        m_worldCameraPosition -= scale * forward;
        m_worldCameraTarget -= scale * forward;
        updateMvpMatrix();
        break;

    case Qt::Key_D:
        m_worldCameraPosition += scale * right;
        m_worldCameraTarget += scale * right;
        updateMvpMatrix();
        break;

    case Qt::Key_Q:
        m_worldCameraPosition -= scale * right;
        m_worldCameraTarget -= scale * right;
        updateMvpMatrix();
        break;

    case Qt::Key_X:
        m_worldCameraPosition += scale * upward;
        m_worldCameraTarget += scale * upward;
        updateMvpMatrix();
        break;

    case Qt::Key_W:
        m_worldCameraPosition -= scale * upward;
        m_worldCameraTarget -= scale * upward;
        updateMvpMatrix();
        break;

    case Qt::Key_E:
        m_worldCameraTarget += scale * right;
        updateMvpMatrix();
        break;

    case Qt::Key_A:
        m_worldCameraTarget -= scale * right;
        updateMvpMatrix();
        break;

    case Qt::Key_R:
        m_worldCameraTarget += scale * upward;
        updateMvpMatrix();
        break;

    case Qt::Key_F:
        m_worldCameraTarget -= scale * upward;
        updateMvpMatrix();
        break;

    case Qt::Key_0:
        m_currentSpace = Space::Model;
        emit currentSpaceChanged(m_currentSpace);
        updateMvpMatrix();
        break;

    case Qt::Key_1:
        m_currentSpace = Space::World;
        emit currentSpaceChanged(m_currentSpace);
        updateMvpMatrix();
        break;

    case Qt::Key_2:
        m_currentSpace = Space::View;
        emit currentSpaceChanged(m_currentSpace);
        updateMvpMatrix();
        break;

    case Qt::Key_3:
        m_currentSpace = Space::NDC;
        emit currentSpaceChanged(m_currentSpace);
        updateMvpMatrix();
        break;

    case Qt::Key_4:
        m_currentSpace = Space::RenderedImage;
        emit currentSpaceChanged(m_currentSpace);
        updateMvpMatrix();
        break;

    default:
        QOpenGLWidget::keyPressEvent(event);
    }
}
