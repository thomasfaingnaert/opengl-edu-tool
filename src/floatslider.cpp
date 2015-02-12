#include "floatslider.h"
#include <QLocale>

FloatSlider::FloatSlider(QWidget *parent) :
    QSlider(parent)
{
    connect(this, &QSlider::valueChanged, this, &FloatSlider::onValueChanged);
}

FloatSlider::~FloatSlider()
{

}

void FloatSlider::onValueChanged(int)
{
    emit scaledValueChanged(scaledValue());
    emit stringValueChanged(getStringRep());
}

QString FloatSlider::getStringRep() const
{
    QLocale sysLocale = QLocale::system();
    return sysLocale.toString(scaledValue(), 'f', m_precision) + suffix();
}

void FloatSlider::init(float scale, int precision, QString suffix)
{
    setScale(scale);
    setPrecision(precision);
    setSuffix(suffix);
    emit stringValueChanged(getStringRep());
}
