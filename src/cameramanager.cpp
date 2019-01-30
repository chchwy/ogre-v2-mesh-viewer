#include "cameramanager.h"


CameraManager::CameraManager(Ogre::Camera* cam)
{
    setCamera(cam);
    setMode(CM_BLENDER);
}

CameraManager::~CameraManager()
{}

void CameraManager::setCamera(Ogre::Camera* cam)
{
    mCamera = cam;
    mCameraNode = mCamera->getSceneManager()->getRootSceneNode()->createChildSceneNode();
    mCamera->detachFromParent();
    mCameraNode->attachObject(mCamera);
    mCamera->setPosition(Ogre::Vector3(0, 5, 15));
    mCamera->lookAt(Ogre::Vector3(0, 0, 0));
    mCamera->setNearClipDistance(2.f);
    mCamera->setFarClipDistance(15000.0f);
    mCamera->setAutoAspectRatio(true);
}

void CameraManager::setTarget(Ogre::SceneNode* target)
{
    if (target != mTarget)
    {
        mTarget = target;
        if (target)
        {
            setYawPitchDist(Ogre::Degree(0), Ogre::Degree(15), 150);
        }
        else
        {
            mCamera->setAutoTracking(false);
        }
    }
}

void CameraManager::manualStop()
{
    if (mMode == CM_FLY)
    {
        mGoingForward = false;
        mGoingBack = false;
        mGoingLeft = false;
        mGoingRight = false;
        mGoingUp = false;
        mGoingDown = false;
        mVelocity = Ogre::Vector3::ZERO;
    }
}

void CameraManager::setYawPitchDist(Ogre::Radian yaw, Ogre::Radian pitch, Ogre::Real dist)
{
    mCamera->setPosition(mTarget->_getDerivedPositionUpdated());
    mCamera->setOrientation(mTarget->_getDerivedOrientationUpdated());
    mCamera->yaw(yaw);
    mCamera->pitch(-pitch);
    mCamera->moveRelative(Ogre::Vector3(0, 0, dist));
}

void CameraManager::setMode(CameraMode mode)
{
    if (mMode != CM_BLENDER && mode == CM_BLENDER)
    {
        //qDebug() << "Target=" << (void*)mTarget;
        setTarget(mTarget ? mTarget : mCamera->getSceneManager()->getRootSceneNode());
        setYawPitchDist(Ogre::Degree(0), Ogre::Degree(15), 150);
    }
    else if (mMode != CM_FLY && mode == CM_FLY)
    {
        mCamera->setAutoTracking(false);
        mCamera->setFixedYawAxis(true);
    }

    if (mTarget == nullptr)
    {
        setTarget(mTarget ? mTarget : mCamera->getSceneManager()->getRootSceneNode());
    }
    mMode = mode;
}

void CameraManager::setProjectionType(Ogre::ProjectionType pt)
{
    if (pt == Ogre::PT_ORTHOGRAPHIC)
    {
        /// @todo: Make orthographic projection work properly.
    }
    else if (pt == Ogre::PT_PERSPECTIVE)
    {
        mCamera->setCustomProjectionMatrix(false);
    }
    mCamera->setProjectionType(pt);
}

void CameraManager::setView(View newView)
{
    switch (newView)
    {
    case VI_TOP:
        mCameraNode->setOrientation(sqrt(0.5), -sqrt(0.5), 0, 0);
        break;
    case VI_BOTTOM:
        mCameraNode->setOrientation(sqrt(0.5), sqrt(0.5), 0, 0);
        break;
    case VI_LEFT:
        mCameraNode->setOrientation(sqrt(0.5), 0, -sqrt(0.5), 0);
        break;
    case VI_RIGHT:
        mCameraNode->setOrientation(sqrt(0.5), 0, sqrt(0.5), 0);
        break;
    case VI_FRONT:
        mCameraNode->setOrientation(1, 0, 0, 0);
        break;
    case VI_BACK:
        setView(VI_FRONT); // Recursion
        mCameraNode->setOrientation(0, 0, 1, 0);
    }
    mCurrentView = newView;
}

void CameraManager::rotatePerspective(Direction dir)
{
    Ogre::Radian amount = Ogre::Radian(Ogre::Degree(15));
    switch (dir)
    {
    case DR_FORWARD:
        mCameraNode->rotate(Ogre::Vector3(1, 0, 0), -amount);
        break;
    case DR_BACKWARD:
        mCameraNode->rotate(Ogre::Vector3(1, 0, 0), amount);
        break;
    case DR_LEFT:
        mCameraNode->rotate(Ogre::Vector3(0, 1, 0), -amount, Ogre::Node::TS_WORLD);
        break;
    case DR_RIGHT:
        mCameraNode->rotate(Ogre::Vector3(0, 1, 0), amount, Ogre::Node::TS_WORLD);
        break;
    }
}

void CameraManager::numpadViewSwitch(const QKeyEvent* evt)
{
    bool ctrl = evt->modifiers().testFlag(Qt::ControlModifier);
    bool numpad = evt->modifiers().testFlag(Qt::KeypadModifier);
    if (numpad)
    {
        switch (evt->key())
        {
        case Qt::Key_1:
            setView(ctrl ? VI_BACK : VI_FRONT);
            break;
        case Qt::Key_2:
            rotatePerspective(DR_BACKWARD);
            break;
        case Qt::Key_3:
            setView(ctrl ? VI_LEFT : VI_RIGHT);
            break;
        case Qt::Key_4:
            rotatePerspective(DR_LEFT);
            break;
        case Qt::Key_5:
            setProjectionType((mCamera->getProjectionType() == Ogre::PT_PERSPECTIVE) ? Ogre::PT_ORTHOGRAPHIC : Ogre::PT_PERSPECTIVE);
            break;
        case Qt::Key_6:
            rotatePerspective(DR_RIGHT);
            break;
        case Qt::Key_7:
            setView(ctrl ? VI_BOTTOM : VI_TOP);
            break;
        case Qt::Key_8:
            rotatePerspective(DR_FORWARD);
            break;
        }
    }
}
