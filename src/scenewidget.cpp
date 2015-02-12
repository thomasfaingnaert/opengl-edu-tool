#include "scenewidget.h"
#include <QSurfaceFormat>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

SceneWidget::SceneWidget(QWidget *parent) :
    QOpenGLWidget(parent), m_modelScale(1.0f, 1.0f, 1.0f)
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

    // Enable face culling
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);

    initProgram();
    initData();
}

void SceneWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_program);
    glBindVertexArray(m_vao);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(0));

    glBindVertexArray(0);
    glUseProgram(0);
}

void SceneWidget::resizeGL(int w, int h)
{
    // Adjust viewport
    glViewport(0, 0, w, h);

    // Adjust perspective matrix
    float aspect = static_cast<float>(w) / h;

    recalcModelMatrix();
    m_viewMatrix = glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));
    m_projectionMatrix = glm::perspective(90.0f, aspect, 0.01f, 100.0f);

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
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Create and bind position vbo
    glGenBuffers(1, &m_vertexDataVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexDataVbo);

    // Create and bind indices vbo
    glGenBuffers(1, &m_indicesVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indicesVbo);

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
    m_modelMatrix *= glm::rotate(glm::mat4(), m_modelRotate.z, glm::vec3(0, 0, 1));
    m_modelMatrix *= glm::rotate(glm::mat4(), m_modelRotate.y, glm::vec3(0, 1, 0));
    m_modelMatrix *= glm::rotate(glm::mat4(), m_modelRotate.x, glm::vec3(1, 0, 0));
    m_modelMatrix *= glm::scale(glm::mat4(), m_modelScale);

    updateMvpMatrix();
}

void SceneWidget::updateMvpMatrix()
{
    m_mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;

    glUseProgram(m_program);
    glUniformMatrix4fv(m_mvpMatrixUnif, 1, GL_FALSE, glm::value_ptr(m_mvpMatrix));
    glUseProgram(0);
}
