#include "cameramanager.h"


CameraManager::CameraManager( Ogre::Camera* cam )
{
    setCamera( cam );
    setMode( CM_BLENDER );
}

CameraManager::~CameraManager()
{}
