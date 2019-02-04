# Ogre3D mesh viewer/converter

A simple Ogre3D mesh viewer based on v2.1 which can import/export obj models.

## How to compile

1. Have your Qt 5 SDK installed (recommend Qt 5.6+), download it [here][0].
2. Have a working Ogre3D 2.1 copy.
3. Set the path to Ogre sdk folder. You can either set an user-level `OGREHOME` environment variable, or set it directly in `ogre-v2-mesh-viewer.pro` line 9.
4. Open `ogre-v2-mesh-viewer.pro` in QtCreator and presse `RUN`.

## TODO

- [ ] Improve performance by converting obj data to ogre mesh directly. (Currently it uses mesh.xml as intermediate data format)
- [x] Automatically generate normals if there are no normal vectors in obj
- [ ] glTF import/export
- [ ] Support DotScene

[0]: https://www.qt.io/download-qt-installer "Qt download"

## Known issues

- [ ] Normals will be broken when converting a v2-mesh to obj format.

## Third Party Libraries

- Ogre3D: <https://www.ogre3d.org/>
- Qt: <https://www.qt.io/>
- Magus Toolkit: <https://github.com/spookyboo/Magus>
- tinyobjloader: <https://github.com/syoyo/tinyobjloader> 
