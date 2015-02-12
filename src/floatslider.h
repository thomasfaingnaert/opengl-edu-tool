#ifndef FLOATSLIDER_H
#define FLOATSLIDER_H

#include <QSlider>
#include <QString>

class FloatSlider : public QSlider
{
    Q_OBJECT

public:
    explicit FloatSlider(QWidget *parent = 0);
    ~FloatSlider();

    float scale() const { return m_scale; }
    void setScale(float scale) { m_scale = scale; }

    int precision() const { return m_precision; }
    void setPrecision(int precision) { m_precision = precision; }

    float scaledValue() const { return value() * scale(); }
    QString getStringRep() const;

signals:
    void scaledValueChanged(float scaledValue);
    void stringValueChanged(QString strVal);

private slots:
    void onValueChanged(int value);

private:
    float m_scale = 0.01;
    int m_precision = 2;
};

#endif // FLOATSLIDER_H
