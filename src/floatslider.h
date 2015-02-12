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

    QString suffix() const { return m_suffix; }
    void setSuffix(const QString &suffix) { m_suffix = suffix; }

    float scaledValue() const { return value() * scale(); }
    QString getStringRep() const;

signals:
    void scaledValueChanged(float scaledValue);
    void stringValueChanged(QString strVal);

private slots:
    void onValueChanged(int value);

private:
    float m_scale = 0.1;
    int m_precision = 1;
    QString m_suffix;
};

#endif // FLOATSLIDER_H
