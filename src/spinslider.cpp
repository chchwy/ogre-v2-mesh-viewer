#include "spinslider.h"

#include <QSlider>
#include <QDoubleSpinBox>

SpinSlider::SpinSlider(QSlider* slider, QDoubleSpinBox* spin) : QObject(slider)
{
    Q_ASSERT(slider);
    Q_ASSERT(spin);

    mSlider = slider;
    mSpin = spin;

    mSlider->setRange(0, 100);
    mSpin->setRange(0, 1);

    setValue(1.0);

    auto doubleSpinSignal = static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    connect(mSpin, doubleSpinSignal, this, &SpinSlider::spinValueChanged);
    connect(mSlider, &QSlider::valueChanged, this, &SpinSlider::sliderValueChanged);
}

void SpinSlider::setValue(double d)
{
    QSignalBlocker s1(mSlider);
    mSlider->setValue(d * mSlider->maximum());

    QSignalBlocker s2(mSpin);
    mSpin->setValue(d);
}

void SpinSlider::sliderValueChanged(int value)
{
    QSignalBlocker b1(mSpin);
    mSpin->setValue(double(value) / mSlider->maximum());

    emit valueChanged(mSpin->value());
}

void SpinSlider::spinValueChanged(double d)
{
    QSignalBlocker b1(mSlider);
    mSlider->setValue(d * mSlider->maximum());

    emit valueChanged(d);
}
