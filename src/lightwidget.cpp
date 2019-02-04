#include "lightwidget.h"
#include "ui_lightwidget.h"

#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreLight.h"
#include "OgreItem.h"
#include "OgreMatrix3.h"

#include <QDebug>
#include <QColorDialog>
#include <QInputDialog>
#include <QSettings>

#include "ogremanager.h"


double getMapDoubleValue(QMap<QString, QVariant>& m, QString key, double defaultValue)
{
    if (m.contains(key))
    {
        return m[key].toDouble();
    }
    return defaultValue;
}

QColor ToQColor(Ogre::ColourValue c)
{
    return QColor(c.r * 255, c.g * 255, c.b * 255);
}

Ogre::ColourValue toOgreColor(QColor c)
{
    return Ogre::ColourValue(c.redF(), c.greenF(), c.blueF());
}

void setButtonIconColour(QPushButton* button, QColor c)
{
    QPixmap pix(80, 15);
    pix.fill(c);

    button->setIconSize(QSize(80, 15));
    button->setIcon(pix);
}

Ogre::Vector3 EulerToDirection(float x, float y, float z)
{
    Ogre::Degree dx(x);
    Ogre::Degree dy(y);
    Ogre::Degree dz(z);

    Ogre::Matrix3 m3;
    m3.FromEulerAnglesXYZ(Ogre::Radian(dx), Ogre::Radian(dy), Ogre::Radian(dz));

    Ogre::Vector3 dir = m3 * Ogre::Vector3(0, -1, 0);
    return dir;
}

LightWidget::LightWidget(QWidget* parent) : QWidget(parent)
{
    ui = new Ui::LightWidget;
    ui->setupUi(this);
}

LightWidget::~LightWidget()
{
    delete ui;
}

void LightWidget::init(OgreManager* ogreMgr)
{
    Q_ASSERT(ogreMgr);

    mSceneManager = ogreMgr->sceneManager();

    bool b = loadFromSettings();
    if (b == false)
    {
        //createDefaultLights();
    }

    auto it = mSceneManager->getMovableObjectIterator(Ogre::LightFactory::FACTORY_TYPE_NAME);
    while (it.hasMoreElements())
    {
        Ogre::Light* light = (Ogre::Light*)it.getNext();
        ui->lightList->addItem(QString::fromStdString(light->getName()));
    }

    mSceneManager->setAmbientLight(Ogre::ColourValue(0.3f, 0.5f, 0.7f) * 0.1f * 0.75f,
                                   Ogre::ColourValue(0.6f, 0.45f, 0.3f) * 0.065f * 0.75f,
                                   Ogre::Vector3(0, -1, 0) + Ogre::Vector3::UNIT_Y * 0.2f);
    connections();

    ui->lightList->setCurrentRow(0);
}

void LightWidget::connections()
{
    connect(ui->lightList, &QListWidget::currentItemChanged, this, &LightWidget::currentLightChanged);

    auto doubleSpinChanged = static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    connect(ui->positionX, doubleSpinChanged, this, &LightWidget::positionChanged);
    connect(ui->positionY, doubleSpinChanged, this, &LightWidget::positionChanged);
    connect(ui->positionZ, doubleSpinChanged, this, &LightWidget::positionChanged);

    connect(ui->directionX, doubleSpinChanged, this, &LightWidget::directionChanged);
    connect(ui->directionY, doubleSpinChanged, this, &LightWidget::directionChanged);
    connect(ui->directionZ, doubleSpinChanged, this, &LightWidget::directionChanged);

    connect(ui->diffusePushButton, &QPushButton::clicked, this, &LightWidget::colorChanged);
    connect(ui->SpecularPushButton, &QPushButton::clicked, this, &LightWidget::colorChanged);

    auto comboSignal = static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged);
    connect(ui->lightTypeCombo, comboSignal, this, &LightWidget::typeChanged);

    connect(ui->addLightButton, &QPushButton::clicked, this, &LightWidget::addLight);
    connect(ui->removeLightButton, &QPushButton::clicked, this, &LightWidget::removeLight);

    connect(ui->rangeSpin, doubleSpinChanged, this, &LightWidget::lightAttenuationChanged);
    connect(ui->constantSpin, doubleSpinChanged, this, &LightWidget::lightAttenuationChanged);
    connect(ui->linearSpin, doubleSpinChanged, this, &LightWidget::lightAttenuationChanged);
    connect(ui->quadraticSpin, doubleSpinChanged, this, &LightWidget::lightAttenuationChanged);

    connect(ui->innerAngleSpin, doubleSpinChanged, this, &LightWidget::lightAngleChanged);
    connect(ui->outerAngleSpin, doubleSpinChanged, this, &LightWidget::lightAngleChanged);
    connect(ui->falloffSpin, doubleSpinChanged, this, &LightWidget::lightAngleChanged);
}

void LightWidget::hideEvent(QHideEvent* evt)
{
    writeToSettings();
}

Ogre::Light* LightWidget::getLightByName(std::string lightName)
{
    auto it = mSceneManager->getMovableObjectIterator(Ogre::LightFactory::FACTORY_TYPE_NAME);
    while (it.hasMoreElements())
    {
        //m_howMuchEntities = m_howMuchEntities + 1;
        Ogre::Light* light = (Ogre::Light*)it.getNext();

        if (light->getName() == lightName)
            return light;
    }

    Q_ASSERT(false);
    return nullptr;
}

bool LightWidget::loadFromSettings()
{
    QSettings settings("OgreV2ModelViewer", "OgreV2ModelViewer");

    settings.beginGroup("lights");

    if (settings.allKeys().empty())
    {
        return false;
    }

    for (const QString& strName : settings.allKeys())
    {
        QMap<QString, QVariant> attriMap = settings.value(strName).toMap();
        createLight(strName, attriMap);
    }

    return true;
}

void LightWidget::createLight(QString name, QMap<QString, QVariant> attri)
{
    Ogre::Light::LightTypes theType = (Ogre::Light::LightTypes)attri["type"].toUInt();

    Ogre::Vector3 pos;
    pos.x = attri["posX"].toFloat();
    pos.y = attri["posY"].toFloat();
    pos.z = attri["posZ"].toFloat();

    Ogre::Vector3 dir;
    dir.x = attri["dirX"].toFloat();
    dir.y = attri["dirY"].toFloat();
    dir.z = attri["dirZ"].toFloat();

    Ogre::ColourValue dc;
    dc.r = attri["dcR"].toFloat();
    dc.g = attri["dcG"].toFloat();
    dc.b = attri["dcB"].toFloat();

    Ogre::ColourValue sc;
    sc.r = attri["scR"].toFloat();
    sc.g = attri["scG"].toFloat();
    sc.b = attri["scB"].toFloat();

    float attenRange = getMapDoubleValue(attri, "attenRange", 23.0);
    float attenConstant = getMapDoubleValue(attri, "attenConstant", 0.5);
    float attenLinear = getMapDoubleValue(attri, "attenLinear", 0.0);
    float attenQuadratic = getMapDoubleValue(attri, "attenQuadratic", 0.5);

    float innerAngle = getMapDoubleValue(attri, "innerAngle", 30);
    float outerAngle = getMapDoubleValue(attri, "outerAngle", 40);
    float falloff = getMapDoubleValue(attri, "falloff", 1.0);

    Ogre::SceneNode *rootNode = mSceneManager->getRootSceneNode();
    Ogre::Light* light = mSceneManager->createLight();

    Ogre::SceneNode* lightNode = rootNode->createChildSceneNode();
    lightNode->attachObject(light);
    light->setName(name.toStdString());
    light->setDiffuseColour(dc);
    light->setSpecularColour(sc);
    light->setType(theType);
    lightNode->setPosition(pos);
    light->setDirection(EulerToDirection(dir.x, dir.y, dir.z));
    light->setCustomParameter(0, Ogre::Vector4(0, 0, 0, 0));
    //light->setAttenuationBasedOnRadius( 1000.0f, 0.02f );
    light->setAttenuation(attenRange, attenConstant, attenLinear, attenQuadratic);
    light->setSpotlightInnerAngle(Ogre::Degree(innerAngle));
    light->setSpotlightOuterAngle(Ogre::Degree(outerAngle));
    light->setSpotlightFalloff(falloff);

    Ogre::Item *item = mSceneManager->createItem("Sphere1000.mesh",
                                                 Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                                 Ogre::SCENE_DYNAMIC);
    item->setCastShadows(false);

    /*
    Ogre::SceneNode* sphereNode = lightNode->createChildSceneNode( Ogre::SCENE_DYNAMIC );
    sphereNode->setScale( 4, 4, 4 );
    sphereNode->setPosition( 0, 0, 0 );
    sphereNode->attachObject( item );
    */
    //ui->lightList->addItem( QString::fromStdString( light->getName() ) );
}

void LightWidget::writeToSettings()
{
    auto it = mSceneManager->getMovableObjectIterator(Ogre::LightFactory::FACTORY_TYPE_NAME);
    while (it.hasMoreElements())
    {
        Ogre::Light* light = (Ogre::Light*)it.getNext();
        saveLight(light);
        //qDebug() << light->getName().c_str();
    }
}

void LightWidget::saveLight(Ogre::Light* light)
{
    QSettings settings("DisplaySweet", "OgreModelViewer");
    settings.beginGroup("lights");

    QMap< QString, QVariant > attri;

    attri["type"] = (uint)light->getType();

    attri["posX"] = light->getParentSceneNode()->getPosition().x;
    attri["posY"] = light->getParentSceneNode()->getPosition().y;
    attri["posZ"] = light->getParentSceneNode()->getPosition().z;

    attri["dirX"] = light->getCustomParameter(0).x;
    attri["dirY"] = light->getCustomParameter(0).y;
    attri["dirZ"] = light->getCustomParameter(0).z;

    Ogre::ColourValue dc = light->getDiffuseColour();
    attri["dcR"] = dc.r;
    attri["dcG"] = dc.g;
    attri["dcB"] = dc.b;

    Ogre::ColourValue sc = light->getSpecularColour();
    attri["scR"] = sc.r;
    attri["scG"] = sc.g;
    attri["scB"] = sc.b;

    attri["powerScale"] = light->getPowerScale();

    attri["attenRange"] = light->getAttenuationRange();
    attri["attenConstant"] = light->getAttenuationConstant();
    attri["attenLinear"] = light->getAttenuationLinear();
    attri["attenQuadratic"] = light->getAttenuationQuadric();

    attri["innerAngle"] = light->getSpotlightInnerAngle().valueDegrees();
    attri["outerAngle"] = light->getSpotlightOuterAngle().valueDegrees();
    attri["falloff"] = light->getSpotlightFalloff();

    settings.setValue(QString::fromStdString(light->getName()), attri);
}


void LightWidget::createDefaultLights()
{
    auto sceneManager = mSceneManager;
    Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode();

    Ogre::Light *light = sceneManager->createLight();
    Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
    light->setName("Dir 1");
    lightNode->attachObject(light);
    light->setPowerScale(1.0f);
    light->setType(Ogre::Light::LT_DIRECTIONAL);
    light->setDirection(Ogre::Vector3(-1, -1, -1).normalisedCopy());
    light->setCustomParameter(0, Ogre::Vector4(0, 0, 0, 0));

    light = sceneManager->createLight();
    lightNode = rootNode->createChildSceneNode();
    lightNode->attachObject(light);
    light->setName("Spot 1 warm");
    light->setDiffuseColour(0.8f, 0.4f, 0.2f); //Warm
    light->setSpecularColour(0.8f, 0.4f, 0.2f);
    light->setPowerScale(Ogre::Math::PI);
    light->setType(Ogre::Light::LT_SPOTLIGHT);
    lightNode->setPosition(-100.0f, 100.0f, 100.0f);
    light->setDirection(Ogre::Vector3(1, -1, -1).normalisedCopy());
    light->setAttenuationBasedOnRadius(1000.0f, 0.02f);
    light->setCustomParameter(0, Ogre::Vector4(0, 0, 0, 0));

    light = sceneManager->createLight();
    lightNode = rootNode->createChildSceneNode();
    lightNode->attachObject(light);
    light->setName("Spot 2 cold");
    light->setDiffuseColour(0.2f, 0.4f, 0.8f); //Cold
    light->setSpecularColour(0.2f, 0.4f, 0.8f);
    light->setPowerScale(Ogre::Math::PI);
    light->setType(Ogre::Light::LT_SPOTLIGHT);
    lightNode->setPosition(100.0f, 100.0f, -100.0f);
    light->setDirection(Ogre::Vector3(-1, -1, 1).normalisedCopy());
    light->setAttenuationBasedOnRadius(1000.0f, 0.02f);
    light->setCustomParameter(0, Ogre::Vector4(0, 0, 0, 0));
}

void LightWidget::currentLightChanged()
{
    QListWidgetItem* curItem = ui->lightList->currentItem();
    if (curItem == nullptr)
        return;

    std::string strLightName = curItem->text().toStdString();
    Ogre::Light* light = getLightByName(strLightName);
    mCurrentLight = light;
    if (light == nullptr)
    {
        return;
    }

    QSignalBlocker b7(ui->lightTypeCombo);
    int index = (int)light->getType();
    ui->lightTypeCombo->setCurrentIndex(index);

    QSignalBlocker b1(ui->positionX);
    QSignalBlocker b2(ui->positionY);
    QSignalBlocker b3(ui->positionZ);

    Ogre::Vector3 pos = light->getParentSceneNode()->getPosition();
    ui->positionX->setValue(pos.x);
    ui->positionY->setValue(pos.y);
    ui->positionZ->setValue(pos.z);

    QSignalBlocker b4(ui->directionX);
    QSignalBlocker b5(ui->directionY);
    QSignalBlocker b6(ui->directionZ);

    Ogre::Vector3 dir = light->getDirection();
    Ogre::Vector4 eulerAngles = light->getCustomParameter(0);
    ui->directionX->setValue(eulerAngles.x);
    ui->directionY->setValue(eulerAngles.y);
    ui->directionZ->setValue(eulerAngles.z);

    Ogre::ColourValue dcolour = light->getDiffuseColour();
    setButtonIconColour(ui->diffusePushButton, ToQColor(dcolour));

    Ogre::ColourValue scolour = light->getSpecularColour();
    setButtonIconColour(ui->SpecularPushButton, ToQColor(scolour));

    QSignalBlocker b9(ui->rangeSpin);
    QSignalBlocker b10(ui->constantSpin);
    QSignalBlocker b11(ui->linearSpin);
    QSignalBlocker b12(ui->quadraticSpin);

    ui->rangeSpin->setValue(light->getAttenuationRange());
    ui->constantSpin->setValue(light->getAttenuationConstant());
    ui->linearSpin->setValue(light->getAttenuationLinear());
    ui->quadraticSpin->setValue(light->getAttenuationQuadric());

    QSignalBlocker b13(ui->innerAngleSpin);
    QSignalBlocker b14(ui->outerAngleSpin);
    QSignalBlocker b15(ui->falloffSpin);
    ui->innerAngleSpin->setValue(light->getSpotlightInnerAngle().valueDegrees());
    ui->outerAngleSpin->setValue(light->getSpotlightOuterAngle().valueDegrees());
    ui->falloffSpin->setValue(light->getSpotlightFalloff());
}

void LightWidget::positionChanged(double v)
{
    if (mCurrentLight == nullptr)
    {
        return;
    }

    QObject* s = QObject::sender();

    Ogre::Vector3 pos = mCurrentLight->getParentSceneNode()->getPosition();
    if (s == ui->positionX)
    {
        pos.x = v;
    }
    else if (s == ui->positionY)
    {
        pos.y = v;
    }
    else if (s == ui->positionZ)
    {
        pos.z = v;
    }

    mCurrentLight->getParentSceneNode()->setPosition(pos);
}

void LightWidget::directionChanged(double v)
{
    if (mCurrentLight == nullptr)
    {
        return;
    }

    QObject* s = QObject::sender();

    //Ogre::Vector3 dir = mCurrentLight->getDirection();
    float x = ui->directionX->value();
    float y = ui->directionY->value();
    float z = ui->directionZ->value();

    Ogre::Vector3 dir = EulerToDirection(x, y, z);

    mCurrentLight->setDirection(dir);
    mCurrentLight->setCustomParameter(0, Ogre::Vector4(x, y, z, 0));

    saveLight(mCurrentLight);
    qDebug() << "Direct: " << dir.x << ", " << dir.y << ", " << dir.z;
}

void LightWidget::colorChanged()
{
    if (mCurrentLight == nullptr)
    {
        return;
    }

    QObject* s = QObject::sender();
    Ogre::ColourValue oldColour;
    if (s == ui->diffusePushButton)
    {
        oldColour = mCurrentLight->getDiffuseColour();
    }
    else if (s == ui->SpecularPushButton)
    {
        oldColour = mCurrentLight->getSpecularColour();
    }

    QColorDialog colorDialog(ToQColor(oldColour), this);
    int ret = colorDialog.exec();

    if (ret == QDialog::Rejected)
    {
        return;
    }

    QColor newColour = colorDialog.selectedColor();

    if (s == ui->diffusePushButton)
    {
        mCurrentLight->setDiffuseColour(toOgreColor(newColour));
        setButtonIconColour(ui->diffusePushButton, newColour);
    }
    else if (s == ui->SpecularPushButton)
    {
        mCurrentLight->setSpecularColour(toOgreColor(newColour));
        setButtonIconColour(ui->SpecularPushButton, newColour);
    }
}

void LightWidget::typeChanged(int index)
{
    if (mCurrentLight == nullptr)
    {
        return;
    }

    mCurrentLight->setType((Ogre::Light::LightTypes)index);
}

void LightWidget::powerScaleChanged(float v)
{
    if (mCurrentLight == nullptr)
    {
        return;
    }

    mCurrentLight->setPowerScale(v);
}

void LightWidget::addLight()
{
    /*
    QString strLightName = QInputDialog::getText( this, "Light name", " Light Name: " );
    if ( strLightName.isEmpty() )
    {
        return;
    }
    */
    static int lightCount = 0;
    QString strLightName = QString("Light %1").arg(lightCount);
    lightCount++;

    Ogre::SceneNode *rootNode = mSceneManager->getRootSceneNode();
    Ogre::Light* light = mSceneManager->createLight();

    Ogre::SceneNode* lightNode = rootNode->createChildSceneNode();
    lightNode->attachObject(light);
    light->setName(strLightName.toStdString());
    light->setDiffuseColour(0.8f, 0.4f, 0.2f);
    light->setSpecularColour(0.8f, 0.4f, 0.2f);
    light->setPowerScale(Ogre::Math::PI);
    light->setType(Ogre::Light::LT_DIRECTIONAL);
    lightNode->setPosition(-100.0f, 100.0f, 100.0f);
    light->setDirection(Ogre::Vector3(1, -1, -1).normalisedCopy());
    light->setAttenuationBasedOnRadius(1000.0f, 0.02f);
    light->setCustomParameter(0, Ogre::Vector4(0, 0, 0, 0));

    ui->lightList->addItem(QString::fromStdString(light->getName()));
    ui->lightList->setCurrentRow(std::max(ui->lightList->count() - 1, 0));

}

void LightWidget::removeLight()
{
    QListWidgetItem* curItem = ui->lightList->currentItem();
    if (curItem == nullptr)
    {
        return;
    }

    std::string strLightName = curItem->text().toStdString();

    Ogre::Light* light = getLightByName(strLightName);
    Ogre::SceneNode* rootNode = mSceneManager->getRootSceneNode();

    rootNode->removeAndDestroyChild(light->getParentSceneNode());

    QSettings settings("OgreV2ModelViewer", "OgreV2ModelViewer");
    settings.beginGroup("lights");
    settings.remove(QString::fromStdString(light->getName()));

    int row = ui->lightList->currentRow();
    QListWidgetItem* itemToDelete = ui->lightList->takeItem(row);
    delete itemToDelete;
}

void LightWidget::lightAttenuationChanged()
{
    QListWidgetItem* curItem = ui->lightList->currentItem();
    if (curItem == nullptr)
    {
        return;
    }
    std::string strLightName = curItem->text().toStdString();
    Ogre::Light* light = getLightByName(strLightName);

    float range = ui->rangeSpin->value();
    float constant = ui->constantSpin->value();
    float linear = ui->linearSpin->value();
    float quadratic = ui->quadraticSpin->value();

    light->setAttenuation(range, constant, linear, quadratic);
}

void LightWidget::lightAngleChanged()
{
    QListWidgetItem* curItem = ui->lightList->currentItem();
    if (curItem == nullptr)
    {
        return;
    }
    std::string strLightName = curItem->text().toStdString();
    Ogre::Light* light = getLightByName(strLightName);

    Ogre::Degree inner(ui->innerAngleSpin->value());
    Ogre::Degree outer(ui->outerAngleSpin->value());
    float falloff = ui->falloffSpin->value();

    light->setSpotlightInnerAngle(inner);
    light->setSpotlightOuterAngle(outer);
    light->setSpotlightFalloff(falloff);
}
