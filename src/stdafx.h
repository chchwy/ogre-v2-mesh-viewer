
#pragma once

// std headers
#include <string>
#include <vector>
#include <functional>

// windows header
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif 

// common ogre headers
#include "OgrePrerequisites.h"
#include "OgreString.h"
#include "OgreRoot.h"
#include "OgreRenderWindow.h"
#include "OgreCamera.h"
#include "OgreItem.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsTextureManager.h"

// common qt headers
#include <QString>
#include <QWidget>
#include <QDebug>

#include "scopeguard.h"
