QT += widgets

message(Qt version: $$[QT_VERSION])

DEFINES += APP_VERSION_NUMBER=\\\"0.1.0\\\"

OGREHOME = $$(OGREHOME)
isEmpty(OGREHOME) {
    OGREHOME = "C:/SDK/OgreSDK/Ogre/build/sdk"
}

message(OGRE_HOME: $$OGREHOME)

PRECOMPILED_HEADER = src/stdafx.h

HEADERS = \
    src/mainwindow.h \
    src/ogremanager.h \ 
    src/ogrewidget.h \ 
    src/lightwidget.h \
    src/tiny_obj_loader.h \
    src/objimporter.h \
    src/cameramanager.h \
    src/OgreXML/OgreXMLMeshSerializer.h \
    src/OgreXML/tinyxml.h \
    src/OgreXML/tinystr.h \
    src/objexporter.h \
	src/scopeguard.h \
    src/batchconversiondialog.h


SOURCES = \
    src/main.cpp \
    src/mainwindow.cpp \
    src/ogremanager.cpp \ 
    src/ogrewidget.cpp \ 
    src/lightwidget.cpp \
    src/objimporter.cpp \
    src/cameramanager.cpp \
    src/OgreXML/OgreXMLMeshSerializer.cpp \
    src/OgreXML/tinyxml.cpp \
    src/OgreXML/tinystr.cpp \
    src/OgreXML/tinyxmlerror.cpp \
    src/OgreXML/tinyxmlparser.cpp \
    src/objexporter.cpp \
    src/batchconversiondialog.cpp


INCLUDEPATH += "src"
INCLUDEPATH += "src/OgreXML"
INCLUDEPATH += "$$OGREHOME/include"
INCLUDEPATH += "$$OGREHOME/include/OGRE"
INCLUDEPATH += "$$OGREHOME/include/OGRE/Hlms/Pbs"
INCLUDEPATH += "$$OGREHOME/include/OGRE/Hlms/Unlit"

DEFINES += TIXML_USE_STL

Debug:LIBS += -L"$$OGREHOME/lib/debug"
Debug:LIBS += -L"$$OGREHOME/lib/debug/opt"
Release:LIBS += -L"$$OGREHOME/lib/release"
Release:LIBS += -L"$$OGREHOME/lib/release/opt"

CONFIG(debug, debug|release):LIBS += \
    -lopengl32 \
    -lOgreMain_d \
    -lOgreHlmsPbs_d \
    -lOgreHlmsUnlit_d \
    -lOgreOverlay_d

CONFIG(release, debug|release):LIBS += \
    -lopengl32 \
    -lOgreMain \
    -lOgreHlmsPbs \
    -lOgreHlmsUnlit \
    -lOgreOverlay


Release:DESTDIR = ./bin
Debug:DESTDIR = ./dbin
target.path = $$[QTDIR]/
INSTALLS += target

FORMS += \
    src/mainwindow.ui \
    src/lightwidget.ui \
    src/batchconversiondialog.ui

