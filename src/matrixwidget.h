#ifndef MATRIXWIDGET_H
#define MATRIXWIDGET_H

#include <QLabel>
#include <QWidget>
#include <glm/glm.hpp>

class MatrixWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MatrixWidget(QWidget *parent = 0);
    ~MatrixWidget();

signals:
    void matrixChanged(const glm::mat4 &matrix);

public slots:
    void setMatrix(const glm::mat4 &matrix) { m_matrix = matrix; updateDisplay(); emit matrixChanged(matrix); }

private:
    glm::mat4 m_matrix;
    QLabel *m_labels[4][4];
    void updateDisplay();
    static constexpr int m_precision = 4;
};

#endif // MATRIXWIDGET_H
