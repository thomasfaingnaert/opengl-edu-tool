#include "scenewidget.h"
#include <QSurfaceFormat>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

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

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    initProgram();
    initData();
}

void SceneWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_program);
    glBindVertexArray(m_vao);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
    glUseProgram(0);
}

void SceneWidget::resizeGL(int w, int h)
{
    // Adjust viewport
    glViewport(0, 0, w, h);
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
        -0.5f, -0.5f, +0.0f,
        +0.5f, -0.5f, +0.0f,
        +0.0f, +0.5f, +0.0f,
    };

    // Create and bind vao
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Create and bind vbo
    glGenBuffers(1, &m_positionVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_positionVbo);

    // Fill buffer & associate with attrib
    glBufferData(GL_ARRAY_BUFFER, sizeof data, data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));

    // Cleanup
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
