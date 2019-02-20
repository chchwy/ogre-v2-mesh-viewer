#include "transformwidget.h"
#include "ui_transformwidget.h"

TransformWidget::TransformWidget(QWidget* parent) : QWidget(parent),
    ui(new Ui::TransformWidget)
{
    ui->setupUi(this);

    auto doubleChanged = qOverload<double>(&QDoubleSpinBox::valueChanged);
    connect(ui->tx, doubleChanged, this, &TransformWidget::positionChanged);
    connect(ui->ty, doubleChanged, this, &TransformWidget::positionChanged);
    connect(ui->tz, doubleChanged, this, &TransformWidget::positionChanged);

    connect(ui->rx, doubleChanged, this, &TransformWidget::rotationChanged);
    connect(ui->ry, doubleChanged, this, &TransformWidget::rotationChanged);
    connect(ui->rz, doubleChanged, this, &TransformWidget::rotationChanged);

    connect(ui->sx, doubleChanged, this, &TransformWidget::scaleChanged);
    connect(ui->sy, doubleChanged, this, &TransformWidget::scaleChanged);
    connect(ui->sz, doubleChanged, this, &TransformWidget::scaleChanged);
}

TransformWidget::~TransformWidget()
{
    delete ui;
}

void TransformWidget::sceneNodeSelected(Ogre::SceneNode* node)
{
    mCurrentNode = node;

    if (node == nullptr)
        return;

    Ogre::Vector3 pos = node->getPosition();

    QSignalBlocker b1(ui->tx);
    QSignalBlocker b2(ui->ty);
    QSignalBlocker b3(ui->tz);
    ui->tx->setValue(pos.x);
    ui->ty->setValue(pos.y);
    ui->tz->setValue(pos.z);
    
    const Ogre::Quaternion quaternion = node->getOrientation();
    Ogre::Matrix3 matrix3;
    quaternion.ToRotationMatrix(matrix3);

    Ogre::Radian rx, ry, rz;
    matrix3.ToEulerAnglesXYZ(rx, ry, rz);

    QSignalBlocker b4(ui->rx);
    QSignalBlocker b5(ui->ry);
    QSignalBlocker b6(ui->rz);
    ui->rx->setValue(rx.valueDegrees());
    ui->ry->setValue(ry.valueDegrees());
    ui->rz->setValue(rz.valueDegrees());

    Ogre::Vector3 scale = node->getScale();

    QSignalBlocker b7(ui->sx);
    QSignalBlocker b8(ui->sy);
    QSignalBlocker b9(ui->sz);
    ui->sx->setValue(scale.x);
    ui->sy->setValue(scale.y);
    ui->sz->setValue(scale.z);
}

void TransformWidget::positionChanged()
{
    if (mCurrentNode)
    {
        mCurrentNode->setPosition(Ogre::Real(ui->tx->value()), 
                                  Ogre::Real(ui->ty->value()),
                                  Ogre::Real(ui->tz->value()));
    }
}

void TransformWidget::rotationChanged()
{
    if (mCurrentNode)
    {
        Ogre::Matrix3 matrix3;
        matrix3.FromEulerAnglesXYZ(Ogre::Degree(ui->rx->value()),
                                   Ogre::Degree(ui->ry->value()),
                                   Ogre::Degree(ui->rz->value()));
        Ogre::Quaternion q;
        q.FromRotationMatrix(matrix3);
        mCurrentNode->setOrientation(q);
    }
}

void TransformWidget::scaleChanged()
{
    if (mCurrentNode)
    {
        mCurrentNode->setScale(Ogre::Real(ui->sx->value()),
                               Ogre::Real(ui->sy->value()),
                               Ogre::Real(ui->sz->value()));
    }
}
