# ogre-v2-mesh-viewer

A simple Ogre3D mesh viewer based on v2.1 which can import/export obj models 

## How to compile

1. Have Qt 5 framework installed (Qt 5.7+ is recommended), you can download it [here][0].
2. Have a working Ogre3D 2.1 copy.
3. You can either set an user-level `OGREHOME` environment variable and put the path of your Ogre 2.1 folder, or set the path directly in `ogre-v2-mesh-viewer.pro` line 9.
4. Open `ogre-v2-mesh-viewer.pro` in QtCreator and presse `RUN`.

## TODO

- [ ] Improve performance by converting obj data to ogre mesh directly. (Currently it uses mesh.xml as intermediate data format)
- [x] Automatically generate normals if there are no normal vectors in obj

[0]: https://www1.qt.io/download-open-source/ "Qt download"