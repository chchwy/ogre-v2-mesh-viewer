xcopy /y "%OGREHOME%\bin\Debug\OgreMain_d.dll"                "dbin"
xcopy /y "%OGREHOME%\bin\Debug\OgreHlmsPbs_d.dll"             "dbin"
xcopy /y "%OGREHOME%\bin\Debug\OgreHlmsUnlit_d.dll"           "dbin"
xcopy /y "%OGREHOME%\bin\Debug\OgreOverlay_d.dll"             "dbin"
xcopy /y "%OGREHOME%\bin\Debug\RenderSystem_Direct3D11_d.dll" "dbin"

xcopy /y "%OGREHOME%\bin\Release\OgreMain.dll"                "bin"
xcopy /y "%OGREHOME%\bin\Release\OgreHlmsPbs.dll"             "bin"
xcopy /y "%OGREHOME%\bin\Release\OgreHlmsUnlit.dll"           "bin"
xcopy /y "%OGREHOME%\bin\Release\OgreOverlay.dll"             "bin"
xcopy /y "%OGREHOME%\bin\Release\RenderSystem_Direct3D11.dll" "bin"

pushd .
call "%QTDIR%\bin\qtenv2.bat"
popd


"%QTDIR%\bin\windeployqt.exe" "bin\ogre-v2-mesh-viewer.exe"
"%QTDIR%\bin\windeployqt.exe" "dbin\ogre-v2-mesh-viewer.exe"

PAUSE