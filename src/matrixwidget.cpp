#include "matrixwidget.h"
#include <QGridLayout>
#include <QLocale>

MatrixWidget::MatrixWidget(QWidget *parent) : QWidget(parent)
{
    QGridLayout *grid = new QGridLayout(this);

    for (size_t col = 0; col < 4; ++col)
    {
        for (size_t row = 0; row < 4; ++row)
        {
            m_labels[col][row] = new QLabel(this);
            grid->addWidget(m_labels[col][row], row, col, 1, 1);
        }
    }

    this->setLayout(grid);
    updateDisplay();
}

MatrixWidget::~MatrixWidget()
{

}

void MatrixWidget::updateDisplay()
{
    for (size_t col = 0; col < 4; ++col)
    {
        for (size_t row = 0; row < 4; ++row)
        {
            QLabel &lbl = *m_labels[col][row];
            QString txt = QLocale::system().toString(m_matrix[col][row], 'f', m_precision);
            lbl.setText(txt);
        }
    }
}
