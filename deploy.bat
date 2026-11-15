set OGREHOME=D:\ogre\ogre-next\build\sdk
set OGREDEPS=D:\ogre\ogre-next-deps\build\ogredeps
set QTDIR=C:\Qt\6.8.3\msvc2022_64

xcopy /y "%OGREHOME%\bin\Debug\OgreNextMain_d.dll"                "dbin"
xcopy /y "%OGREHOME%\bin\Debug\OgreNextHlmsPbs_d.dll"             "dbin"
xcopy /y "%OGREHOME%\bin\Debug\OgreNextHlmsUnlit_d.dll"           "dbin"
xcopy /y "%OGREHOME%\bin\Debug\OgreNextOverlay_d.dll"             "dbin"
xcopy /y "%OGREHOME%\bin\Debug\RenderSystem_Direct3D11_d.dll" "dbin"
xcopy /y "%OGREHOME%\bin\Debug\RenderSystem_GL3Plus_d.dll"    "dbin"
xcopy /y "%OGREDEPS%\bin\Debug\amd_ags_x64.dll"               "dbin"

xcopy /y "%OGREHOME%\bin\Release\OgreNextMain.dll"                "bin"
xcopy /y "%OGREHOME%\bin\Release\OgreNextHlmsPbs.dll"             "bin"
xcopy /y "%OGREHOME%\bin\Release\OgreNextHlmsUnlit.dll"           "bin"
xcopy /y "%OGREHOME%\bin\Release\OgreNextOverlay.dll"             "bin"
xcopy /y "%OGREHOME%\bin\Release\RenderSystem_Direct3D11.dll" "bin"
xcopy /y "%OGREHOME%\bin\Release\RenderSystem_GL3Plus.dll"    "bin"
xcopy /y "%OGREDEPS%\bin\Release\amd_ags_x64.dll"             "bin"

pushd .
call "%QTDIR%\bin\qtenv2.bat"
popd


"%QTDIR%\bin\windeployqt.exe" "bin\ogre-v2-mesh-viewer.exe"
"%QTDIR%\bin\windeployqt.exe" "dbin\ogre-v2-mesh-viewer.exe"

PAUSE