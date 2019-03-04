#include "stdafx.h"

#include "cameramanager.h"


CameraManager::CameraManager(Ogre::Camera* cam)
{
    setCamera(cam);
    setMode(CM_BLENDER);
}

CameraManager::~CameraManager()
{}

void CameraManager::reset()
{
    mCameraNode->setPosition(0, 3, 0);
    mCameraNode->resetOrientation();
    mCamera->setPosition(Ogre::Vector3(0, 0, 10));
}

void CameraManager::setCamera(Ogre::Camera* cam)
{
    mCamera = cam;
    mCamera->detachFromParent();

    mCameraNode = mCamera->getSceneManager()->getRootSceneNode()->createChildSceneNode();
    mCameraNode->attachObject(mCamera);
    mCameraNode->setPosition(0, 3, 0);
    mCameraNode->resetOrientation();

    mCamera->setPosition(Ogre::Vector3(0, 0, 10));
    mCamera->lookAt(Ogre::Vector3(0, 0, 0));
    mCamera->setNearClipDistance(0.1f);
    mCamera->setFarClipDistance(10000.0f);
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
    if (pt == Ogre::PT_PERSPECTIVE)
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

bool CameraManager::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
    if (mMode == CM_FLY)
    {
        // build our acceleration vector based on keyboard input composite
        Ogre::Vector3		accel = Ogre::Vector3::ZERO;
        if (mGoingForward)	accel += mCamera->getDirection();
        if (mGoingBack)		accel -= mCamera->getDirection();
        if (mGoingRight)	accel += mCamera->getRight();
        if (mGoingLeft)		accel -= mCamera->getRight();
        if (mGoingUp)		accel += mCamera->getUp();
        if (mGoingDown)		accel -= mCamera->getUp();

        // if accelerating, try to reach top speed in a certain time
        Ogre::Real topSpeed = mShiftDown ? mTopSpeed * 20 : mTopSpeed;
        if (accel.squaredLength() != 0)
        {
            accel.normalise();
            mVelocity += accel * topSpeed * evt.timeSinceLastFrame * 10;
        }
        // if not accelerating, try to stop in a certain time
        else mVelocity -= mVelocity * evt.timeSinceLastFrame * 10;

        Ogre::Real tooSmall = std::numeric_limits<Ogre::Real>::epsilon();

        // keep camera velocity below top speed and above epsilon
        if (mVelocity.squaredLength() > topSpeed * topSpeed)
        {
            mVelocity.normalise();
            mVelocity *= topSpeed;
        }
        else if (mVelocity.squaredLength() < tooSmall * tooSmall)
            mVelocity = Ogre::Vector3::ZERO;

        if (mVelocity != Ogre::Vector3::ZERO) mCamera->move(mVelocity * evt.timeSinceLastFrame);
    }
    return true;
}

void CameraManager::keyPress(const QKeyEvent* evt)
{
    if (mMode == CM_FLY)
    {
        if (evt->key() == Qt::Key_W)
        {
            mGoingForward = true;
        }
        else if (evt->key() == Qt::Key_A)
        {
            mGoingLeft = true;
        }
        else if (evt->key() == Qt::Key_S)
        {
            mGoingBack = true;
        }
        else if (evt->key() == Qt::Key_D)
        {
            mGoingRight = true;
        }
    }
    if (evt->key() == Qt::Key_Shift)
    {
        mShiftDown = true;
    }
    if (mMode == CM_BLENDER)
    {
        Ogre::Vector3 accel = Ogre::Vector3::ZERO;
        if (evt->key() == Qt::Key_W)
        {
            accel += mCamera->getDirection();
        }
        else if (evt->key() == Qt::Key_A)
        {
            accel -= mCamera->getRight();
        }
        else if (evt->key() == Qt::Key_S)
        {
            accel -= mCamera->getDirection();
        }
        else if (evt->key() == Qt::Key_D)
        {
            accel += mCamera->getRight();
        }
        if (accel != Ogre::Vector3::ZERO)
        {
            accel.normalise();
            mCamera->move(accel * 1.5);
        }

        numpadViewSwitch(evt);
    }
}

void CameraManager::keyRelease(const QKeyEvent* evt)
{
    if (evt->key() == Qt::Key_W)
        mGoingForward = false;
    else if (evt->key() == Qt::Key_A)
        mGoingLeft = false;
    else if (evt->key() == Qt::Key_S)
        mGoingBack = false;
    else if (evt->key() == Qt::Key_D)
        mGoingRight = false;
    if (evt->key() == Qt::Key_Shift)
        mShiftDown = false;
}

void CameraManager::mouseMove(Ogre::Vector2 mousePos)
{
    if (mMode == CM_FLY)
    {
        mCamera->yaw(Ogre::Degree(-mousePos.x * 0.15f));
        mCamera->pitch(Ogre::Degree(-mousePos.y * 0.15f));
    }
    if (mMode == CM_BLENDER || mMode == CM_ORBIT)
    {
        if (mOrbiting && !mShiftDown)
        {
            rotate(mousePos.x, mousePos.y);
            if (mCurrentView != VI_USER)
                mCurrentView = VI_USER;
        }
        else if ((mOrbiting && mShiftDown) && mMode == CM_BLENDER)
        {
            pan(mousePos.x, mousePos.y);
        }
    }
}

void CameraManager::mouseWheel(const QWheelEvent* evt)
{
    mMouseWheelDelta = evt->delta();
    //qDebug() << (uint64_t)mCamera << ", " << (uint64_t)mTarget;
    mDistFromTarget = (mCamera->getPosition() - mTarget->_getDerivedPositionUpdated()).length();
    mCamera->moveRelative(Ogre::Vector3(0, 0, -mMouseWheelDelta * 0.0004f * mDistFromTarget));
}

void CameraManager::mousePress(const QMouseEvent* evt)
{
    if (mMode == CM_BLENDER || mMode == CM_ORBIT)
    {
        if (evt->button() == Qt::MiddleButton || evt->button() == Qt::LeftButton)
        {
            mOrbiting = true;
        }
    }
}

void CameraManager::mouseRelease(const QMouseEvent* evt)
{
    if (mMode == CM_BLENDER || mMode == CM_ORBIT)
    {
        if (evt->button() == Qt::MiddleButton || evt->button() == Qt::LeftButton)
        {
            mOrbiting = false;
        }
    }
}

void CameraManager::rotate(int x, int y)
{
    mCameraNode->yaw(Ogre::Degree(-x * 0.4f), Ogre::Node::TS_PARENT);
    mCameraNode->pitch(Ogre::Degree(-y * 0.4f));
}

void CameraManager::pan(float x, float y)
{
    Ogre::Vector3 transVector(-x, y, 0);
    if (mTarget)
    {
        mDistFromTarget = (mCamera->getPosition() - mTarget->_getDerivedPositionUpdated()).length();

        if (mTarget->numAttachedObjects() > 0 && mTarget->getAttachedObject(0))
            transVector *= mTarget->getAttachedObject(0)->getWorldRadius() * (mDistFromTarget / 10000.0f);
        else
            transVector *= (mDistFromTarget / 500.f);
    }
    mCameraNode->translate(transVector, Ogre::Node::TS_LOCAL);
}
