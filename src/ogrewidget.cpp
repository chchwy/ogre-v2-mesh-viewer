/****************************************************************************
**
** Copyright (C) 2016 This file is generated by the Magus toolkit
** Copyright (C) 2016-2019 Matt Chiawen Chang
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "stdafx.h"
#include "ogrewidget.h"
#include "ogremanager.h"
#include "cameracontroller.h"

#include "OgreWindow.h"
#include "OgreCamera.h"

#include "Compositor/OgreCompositorManager2.h"
#include "OgreRenderSystem.h"
#include "OgreTimer.h"


OgreWidget::OgreWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_PaintOnScreen);
    setMinimumSize(240, 240);

    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    mBackground = Ogre::ColourValue(0.2f, 0.2f, 0.2f);
    mAbsolute = Ogre::Vector2::ZERO;
    mRelative = Ogre::Vector2::ZERO;
}

OgreWidget::~OgreWidget()
{
}

HGLRC OgreWidget::getCurrentGlContext()
{
#ifdef _WIN32
    return wglGetCurrentContext(); // Windows
#else
    return glXGetCurrentContext(); // Linux
#endif
    assert(false);
    return 0;
}

void OgreWidget::createRenderWindow(OgreManager* ogreManager)
{
    Q_ASSERT(ogreManager);
    mOgreManager = ogreManager;

    Ogre::Root* root = ogreManager->ogreRoot();
    Q_ASSERT(root);

    // Get render system and assign window handle
    Ogre::RenderSystem* renderSystem = root->getRenderSystem();
    Ogre::NameValuePairList parameters;

    // Reuse the glContext if available
    HGLRC glContext = 0;
    if (ogreManager->isRenderSystemGL())
    {
        parameters["currentGLContext"] = Ogre::String("false");
        glContext = ogreManager->getGlContext();
        if (glContext)
        {
            parameters["externalGLContext"] = Ogre::StringConverter::toString((size_t)(glContext));
            parameters["vsync"] = "No";
        }
    }

    std::string sWindowHandle = std::to_string((size_t)(winId()));;
    parameters["externalWindowHandle"] = sWindowHandle;
    parameters["parentWindowHandle"] = sWindowHandle;

    Ogre::ConfigOptionMap& cfgOpts = renderSystem->getConfigOptions();
    parameters["gamma"] = "true";
    parameters["FSAA"] = cfgOpts["FSAA"].currentValue;
    parameters["vsync"] = cfgOpts["VSync"].currentValue;

    mOgreRenderWindow = root->createRenderWindow("MainRenderWin",
                                                 width(), height(),
                                                 false, // full screen
                                                 &parameters);
    //mOgreRenderWindow->setVisible(true);

    // Determine whether the GL context can be reused
    if (ogreManager->isRenderSystemGL() && !glContext)
    {
        // Store the glContext in the ogre manager
        glContext = getCurrentGlContext();
        ogreManager->setGlContext(glContext);
    }
}

void OgreWidget::createCompositor()
{
    // Create camera
    mCamera = mOgreManager->sceneManager()->createCamera("MainCamera");
    mCamera->setAspectRatio(Ogre::Real(mOgreRenderWindow->getWidth()) / Ogre::Real(mOgreRenderWindow->getHeight()));
    mCameraController = new CameraController(mCamera);

    const Ogre::String workspaceName = "PbsMaterialsWorkspace";
    const Ogre::IdString workspaceNameHash = workspaceName;

    Ogre::CompositorManager2* compositorManager = mOgreManager->ogreRoot()->getCompositorManager2();
    compositorManager->addWorkspace(mOgreManager->sceneManager(), mOgreRenderWindow->getTexture(), mCamera, workspaceNameHash, true);

    mInitialized = true;
}

void OgreWidget::updateOgre(float timeSinceLastFrame)
{
    mTimeSinceLastFrame = timeSinceLastFrame;
}

QPaintEngine* OgreWidget::paintEngine() const
{
    // We don't want another paint engine to get in the way for our Ogre based paint engine.
    // So we return nothing.
    return 0;
}

void OgreWidget::paintEvent(QPaintEvent* e)
{
}

void OgreWidget::resizeEvent(QResizeEvent* e)
{
    if (e->isAccepted())
    {
        const QSize& newSize = e->size();
        if (mCamera && mOgreRenderWindow)
        {
            Ogre::Real aspectRatio = Ogre::Real(newSize.width()) / Ogre::Real(newSize.height());
            mCamera->setAspectRatio(aspectRatio);

            //mOgreRenderWindow->resize(newSize.width(), newSize.height());
            mOgreRenderWindow->windowMovedOrResized();
            //qDebug() << "Window resize=" << newSize << ", Ratio=" << aspectRatio;
        }
    }
}

void OgreWidget::keyPressEvent(QKeyEvent* ev)
{
    if (mInitialized)
        mCameraController->keyPress(ev);
}

void OgreWidget::keyReleaseEvent(QKeyEvent* ev)
{
    if (mInitialized)
        mCameraController->keyRelease(ev);
}

void OgreWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (mInitialized)
    {
        Ogre::Vector2 oldPos = mAbsolute;
        mAbsolute = Ogre::Vector2(e->pos().x(), e->pos().y());
        mRelative = mAbsolute - oldPos;
        mCameraController->mouseMove(mRelative);
    }
}

void OgreWidget::wheelEvent(QWheelEvent* e)
{
    if (mInitialized)
        mCameraController->mouseWheel(e);
}

void OgreWidget::mousePressEvent(QMouseEvent* e)
{
    if (mInitialized)
        mCameraController->mousePress(e);
}

void OgreWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (mInitialized)
        mCameraController->mouseRelease(e);
}

