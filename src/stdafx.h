
#pragma once

// std headers
#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <fstream>
#include <array>
#include <memory>

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
#include "OgreWindow.h"
#include "OgreCamera.h"
#include "OgreItem.h"
#include "OgreSceneManager.h"
#include "OgreSceneNode.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"
#include "Vao/OgreIndexBufferPacked.h"
#include "Vao/OgreVertexBufferPacked.h"

#include "OgreHlmsManager.h"

// common qt headers
#include <QString>
#include <QWidget>
#include <QDebug>

#include "scopeguard.h"
