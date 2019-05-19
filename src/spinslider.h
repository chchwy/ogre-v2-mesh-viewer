#ifndef SPINSLIDER_H
#define SPINSLIDER_H

#include <QObject>

class QSlider;
class QDoubleSpinBox;

class SpinSlider : public QObject
{
    Q_OBJECT
public:
    explicit SpinSlider(QSlider* slider, QDoubleSpinBox* spin);

    void setValue(double d);

signals:
    void valueChanged(double value);

private:
    void sliderValueChanged(int value);
    void spinValueChanged(double d);

    QSlider* mSlider = nullptr;
    QDoubleSpinBox* mSpin = nullptr;
};

#endif // SPINSLIDER_H
