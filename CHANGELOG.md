# ChangeLog

# v0.4.2

- Fix a crash when loading a new model
- Upgrade `tinygltf` to better support glTF 2.0 spec 

# v0.4.1

General bug fixes

# v0.4.0

## Basic HLMS editing
- Texture: diffuse, background diffuse, normal, roughness, metalness
- Diffuse color, background diffuse color, specular color
- Transparent value & mode
- Solid or wireframe
- Two-sided on/off

## Mesh states:
- Show/hide bounding boxes
- Switch visibility

## Other changes
- Fixed a crash when selecting the pistol at the beginning and then loading another model
- Fixed a crash when the material drop-down list emit an index of -1
- Change the clear color from sky blue to black
- Create a new Pbs Datablock for the mesh which doesn't come with materials
- Showing irradiance backgrounds in the beginning
- Using GL3+ as the default rendersystem due to some issues in D3D11 rendersystem

# v0.3.3

- Re-implement Save as Ogre Mesh. Now you can save all meshes of the scene along with Hlms materials
- Don't show the progress bar for each mesh when doing batch conversion
- Give a new default Ogre3D config file
- Remove the temporary v1 mesh while the conversion to v2 mesh is done

# v0.3.2

- Enabled Hi-Dpi support
- Switching between backgrounds: environment/irradiance/black
- Generating correct AABB bounding boxes when importing a wavefront obj

# v0.3.1

- Speed up the process of Wavefront Obj to Ogre3D mesh conversion
- Added a Batch conversion tool to convert multiple obj to ogre mesh at once
- Improved camera panning by to the distance from camera to target object
- Fixed: app hangs when closing the select-folder dialog without selecting any folder

# v0.3.0

### Features 

- Support glTF format
- Added Scene tree panel
- Added transformation panel (translation, rotation and scale)
- Support load multiple mesh files from a folder





