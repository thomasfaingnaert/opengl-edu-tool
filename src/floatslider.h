#ifndef FLOATSLIDER_H
#define FLOATSLIDER_H

#include <QSlider>

class FloatSlider : public QSlider
{
public:
    explicit FloatSlider(QWidget *parent);
    ~FloatSlider();
};

#endif // FLOATSLIDER_H
