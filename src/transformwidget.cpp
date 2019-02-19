#include "transformwidget.h"
#include "ui_transformwidget.h"

TransformWidget::TransformWidget(QWidget* parent) : QWidget(parent),
    ui(new Ui::TransformWidget)
{
    ui->setupUi(this);
}

TransformWidget::~TransformWidget()
{
    delete ui;
}

void TransformWidget::sceneNodeSelected(Ogre::SceneNode* node)
{
    if (node == nullptr)
        return;

    Ogre::Vector3 pos = node->getPosition();

    QSignalBlocker b1(ui->tx);
    QSignalBlocker b2(ui->ty);
    QSignalBlocker b3(ui->tz);
    ui->tx->setValue(pos.x);
    ui->ty->setValue(pos.y);
    ui->tz->setValue(pos.z);
    
    const Ogre::Quaternion quaternion = node->_getDerivedOrientation();
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
