# Format of my custom data format: BOGLE (version 0)

BOGLE meaning Blender to OpenGL Exporter. The extension is .bgl

All data is little-endian encoded. All matrices are in column-major order.

The following shows the overall structure of the file:

```
FILE HEADER

GLOBAL AMBIENT LIGHT

CAMERA 1 DATA
...
CAMERA N DATA

GEOMETRY 1 HEADER
GEOMETRY 1 DATA
...
GEOMETRY N HEADER
GEOMETRY N DATA

MATERIAL 1 DATA
...
MATERIAL N DATA

LIGHT 1 DATA
...
LIGHT N DATA

ANIMATION COLLECTION 1 DATA
ANIMATION 1 DATA
FRAME 1 DATA
...
ANIMATION FRAME N DATA
...
ANIMATION N DATA
...
ANIMATION COLLECTION N DATA

INSTANCE DEFINITIONS
SCENE TREE
```

## Nomenclature

Type sizes are given in C, similar to the `stdint.h` header file. Floating
point numbers are always 4 byte floats.

For example, an unsigned 8 bit integer would be given as `uint8`, and `float`
always stands for a 4 byte floating point number.

As specified above, everything is little-endian encoded.

## Header

* `5 uint8` -> File Signature ("BOGLE" 0x42 0x4f 0x47 0x4c 0x45)

* `1 uint8` -> Version number. Unsigned.

* `1 uint32` -> Number of cameras defined.

* `1 uint32` -> Number of geometries defined.

* `1 uint32` -> Number of materials defined.

* `1 uint32` -> Number of lights defined.

* `1 uint32` -> Number of animations defined.

* `1 uint32` -> Number of object instances defined.

## Global Ambient Light

In Blender, this is the world's color.

* `4 float` -> Color to add as global ambient light when shading.

## Cameras

There may be many camera components assigned, but only one can have the main camera flag set.

* `1 uint8` -> Camera type. 0 for a basic camera, 1 for an fps camera. The
  exporter always uses 1 for now.
  
* `1 uint32` -> Length of the camera's name. `namelen`

* `namelen uint8` -> Camera's name.

* `1 uint32` -> Screen width. (Blender: Output properties -> Resolution X)

* `1 uint32` -> Screen height. (Blender: Output properties -> Resolution Y)

* `1 float` -> Near clip. (Blender: Camera's Object Data Properties -> Clip
  Start)

* `1 float` -> Far clip. (Blender: Camera's Object Data Properties -> Clip End)

* `1 floar` -> Field of View (radians) (Blender: Camera's Object Data
  Properties -> Field of View)
  
* `1 uint8` -> Main camera flag: This is the active camera. Only one camera can
  be the active camera.

## Geometry header

Each geometry should be assigned to one or more objects.

* `1 uint8` -> Geometry type. Always 0 for now.
  
* `1 uint32` -> Length of the geometry's name. `namelen`

* `namelen uint8` -> Geometry's name.

* `1 uint32` -> `vertlen` The number of vertices in the object.

* `1 uint32` -> `indlen` The number of indices in the object.

## Geometry data

* `vertlen vertices` -> The vertex data, more on that below.

* `indlen uint32` -> The index data.

### Vertices

Vertices are outlined as follows (all in object space):

* `3 floats` -> Vertex position.

* `2 floats` -> Texture coordinates.

* `3 floats` -> Normal.

* `3 floats` -> Tangent.

* `3 floats` -> Binormal.

* `3 uint32` -> Indices of bones that affect this vertex. They match with the
  indices of the bones in the skeleton.

* `3 float` -> Weight of how much the vertex is affected by the corresponding
  bone.

## Materials

Each material should be assigned to one or more object instances. Special
grouped shaders exist which represent the material. These shaders should be
used in the material, since they make it easy to gather these values.

* `1 uint8` -> Material type. Reserved in case there's more than one
  material. Should be 0.

* `1 uint8` -> Shader type. Reserved in case there's more than one
  shader. Should be 0.
  
* `1 uint32` -> Length of the material's name. `namelen`

* `namelen uint8` -> Material's name.

The following fields would then depend on the kind of material.

* `4 float` -> Ambient color.

* `4 float` -> Emissive color.

* `4 float` -> Diffuse color.

* `4 float` -> Specular color.

* `1 float` -> Opacity. The entire object will be applied this alpha. 1 for
  completely opaque.

* `1 float` -> Specular power. Shininess.

* `1 float` -> Reflectance. Power of the reflectance of the material. Zero
  means the material doesn't reflect at all. 1 means that it's completely
  reflective. Any value in between mixes the reflection effect with the regular
  color.

* `1 float` -> Refraction. Power of the refraction of the material. Zero means
  it doesn't refract at all. 1 means it's completely refractive. Any value in
  between mixes the refraction effect with the regular color.

* `1 float` -> Index of refraction. Used for refractive materials. The division
  between the refractive index of the material the light is coming from and
  this material. Air's is 1.00 and glass is 1.52 so for a refractive glass
  material this value should be 1.00/1.52

* `1 float` -> Bump intensity. Scale values of a bump map, if there's any.

* `1 float` -> Specular scale. Scale values from the specular power map, if
  there's any.

* `1 float` -> Alpha threshold. Pixels with alpha below this value will be
  discarded. With this, "cut-off" objects with holes can be made without having
  to render them in the transparency pass.

* `1 uint8` -> Alpha blending mode. If nonzero, this is a semitransparent
  texture.

Next, each texture is defined. It's defined as simply the name of the texture
file, without extension (will look for a png). Before each name comes the
number of characters in the name, or 0 if there's no texture. In which case
there should be 0 characters (nothing) and then continue to define the next
texture.

* `1 uint32` -> Number of characters in the texture name. `nchars`

* `nchars uint8` -> The name of the png file with the texture.

The supported textures are:

* Ambient

* Emissive

* Diffuse

* Specular

* Specular Power

* Normal

* Bump

* Opacity

Normal and bump textures are mutually exclusive.

## Lights

Each light should be assigned to one or more object instances.

* `1 uint8` -> Type of light:

  - 0 means Spot Light
  
  - 1 means Directional (Sun) Light
  
  - 2 means Point Light
  
* `1 uint32` -> Length of the light's name. `namelen`

* `namelen uint8` -> Light's name.

* `4 float` -> Color of the light. Found in the light object's data.

* `1 float` -> Constant attenuation of the light. From Blender, it's always 0.

* `1 float` -> Linear attenuation of the light. From Blender, it's calculated
  from the light's "distance" (light data > custom distance). The formula is `l
  = 1000/d - d`.
  
* `1 float` -> Quadratic attenuation of the light. From Blender, it's always 1.

* `1 float` -> Intensity of the light. Found in the light object's data as the
  power.
  
* `1 float` -> Angle of the light in radians (only meaningful for spot
  lights). Found in the light object's data, under Spot Shape.

Non meaningful fields like angle for a non spot light must still be present
with any valid value.

## Animations

Animations are ordered into collections. Each collection can have many
animations. Then each collection is associated to one or more objects. An
animation collection is all the animations that can be applied to an object as
well as the skeleton that the actual animations are played on.

### Animation collection

* `1 uint8` -> Type of animation collection. Always 0.
  
* `1 uint32` -> Length of the collection's name. `namelen`

* `namelen uint8` -> Animation collection's name.

* `1 uint32` -> How many animations are in this collection. `nanimations`

### Skeleton

Then follows the definition of the skeleton this animations are actually
animating. The definition is simply an offset from the object's origin and then
a collection of bones.

* `16 float` -> Transform of the skeleton relative to the object it's being
  applied to.

* `1 uint32` -> Number of bones in the skeleton. `nbones`

Then follow `nbones` bone definitions. These define the rotation of each bone
in the skeleton's bind pose relative to their parent, or in the case of the
root, to the object's origin.

#### Bone

* `3 floats` -> Bone's position

* `4 floats` -> Bone's rotation, an (x,y,z,w) quaternion (normalized)

* `1 uint32` -> Parent index. Zero if no parent, one if parent is the first
  defined bone, etc.

Then follow `nanimations` animations

### Animation

* `1 uint32` -> Length of the name of the animation. `namelen`

* `namelen uint8` -> Animation's name.

* `1 uint32` -> How many keyframes are in this animation. `nkeyframes`

Then follow `nkeyframes` keyframes

### Animation keyframe

All keyframes must be ordered by timestamp.

* `1 float` -> Timestamp of this keyframe

* `3 floats` -> Offset of the root bone relative to the skeleton's bind pose.

`nbones` times the following:

* `4 floats` -> Bone's rotation, an (x,y,z,w) quaternion (normalized) relative
  to the parent bone (or to the object's origin in the case of the root bone).

## Instance definitions

Here each object instance is defined and assigned a position, rotation and
scale (relative to their parent). They are also possibly assigned a geometry, a
material, a light and to exactly one of them the camera. For the camera
instance and the light instances, the meaning of position, rotation and scale
is different:

The camera object's position is the camera position. Whatever euclidean
rotation results from the rotation axis and angle, the X axis is the pitch and
Y the yaw. Z is ignored (this camera can't roll). The camera's scale is
ignored.

A light's position is the light's position for point and spot lights and
ignored for directional lights. The rotation is the rotation of the default
direction vector, which is beaming straight down (0, 0, -1) for directional and
spot lights. And ignored for point lights. The scale is always ignored.
  
* `1 uint32` -> Length of the object's name. `namelen`

* `namelen uint8` -> Object's name.

* `1 uint32` -> Camera index: The first camera defined in the file is
  index 1. Index 0 means not a camera.
  
* `1 uint32` -> Geometry index: Same strategy as the camera index, 0 means no
  geometry.
  
* `1 uint32` -> Material index: Same strategy as the others, 0 means no
  material. All objects with a geometry must have a material, otherwise crashes
  may occur.
  
* `1 uint32` -> Light index: Same strategy as with the others. Generally, a
  light or camera objects doesn't have a geometry or material.
  
* `1 uint32` -> Animation collection index: Which animation collection, if any,
  is associated to this object.
  
* `16 floats` -> Transformation matrix.

## Scene tree

This is a string of characters (one `uint8` each) specifying how the object
tree should be formed.

An example:

```
implied root
   /  |  \
  0   1   2
 / \  |   |
 3 4  8   9
 |
 5
/ \ 
6 7
```

Would become (spaces optional and ignored):

`0 { 3 { } { 5 { } { 6 { } 7 { } } } 4 { } } 1 { 8 { } } 2 { } { 9 { } }\0`

Basically `{` means going down a level and `}` going up a level. This should be
defined in such an order that you're always in a node that has been
defined. You start at the implied root, then go down and define 0, then go down
and define 3, etc. You can't go directly from the root down to define 5 when 0
and 3 haven't been defined.

Maximum 256 levels of depth. A 0 character marks the end of the string (as in
the example).
