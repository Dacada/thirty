# Format of my custom data format: BOGLE (version 0)

BOGLE meaning Blender to OpenGL Exporter. The extension is .bgl

All data is little-endian encoded. All matrices are in column-major order.

The following shows the overall structure of the file:

```
FILE HEADER

CAMERA DATA
GLOBAL AMBIENT LIGHT

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

* `1 uint32` -> Number of geometries defined.

* `1 uint32` -> Number of materials defined.

* `1 uint32` -> Number of lights defined.

* `1 uint32` -> Number of object instances defined.

## Skybox

The skybox is a cube texture, composed of 6 images, which have the following
layout:

```
     top
left front  right back
     bottom
```

The base name should be the value of the "skybox" property on the scene's
world.

A single name is used, which is appended `_top`, `_left`, `_bottom`, `_front`,
`_right` or `_back` to retrieve the corresponding texture.

* * `1 uint32` -> Number of characters in the scene's skybox
  filename. `skybox_nchars`. Of course the next property will be empty if this
  is zero, meaning no skybox.
  
* `skybox_nchars uint8` -> The actual filename of the scene's skybox. This is
  what will be appended `_top`, etc. To actually retrieve each texture.

## Camera

One and only one object instance should be assigned as the camera.

* `1 uint32` -> Screen width. (Blender: Output properties -> Resolution X)

* `1 uint32` -> Screen height. (Blender: Output properties -> Resolution Y)

* `1 float` -> Near clip. (Blender: Camera's Object Data Properties -> Clip
  Start)

* `1 float` -> Far clip. (Blender: Camera's Object Data Properties -> Clip End)

* `1 floar` -> Field of View (radians) (Blender: Camera's Object Data
  Properties -> Field of View)

## Global Ambient Light

In Blender, this is the world's color.

* `4 float` -> Color to add as global ambient light when shading.

## Geometry header

Each geometry should be assigned to one or more objects.

* `1 uint32` -> `vertlen` The number of vertices in the object.

* `1 uint32` -> `indlen` The number of indices in the object.

## Geometry data

* `vertlen vertices` -> The vertex data, more on that below.

* `indlen uint32` -> The index data.

### Vertices

Vertices are outlined as follows: 3 floats for the vertex coordinate, 2 floats
for the texture coordinate, 3 floats for the normal, 3 floats for the tangent
and 3 floats for the binormal.

Therefore the vertex data structure consists of `14 float`.

## Materials

Each material should be assigned to one or more object instances. Special
grouped shaders exist which represent the material. These shaders should be
used in the material, since they make it easy to gather these values.

* `1 uint8` -> Material type. Reserved in case there's more than one
  material. Should be 0.

* `1 uint8` -> Shader type. Reserved in case there's more than one
  shader. Should be 0.

The following fields would then depend on the kind of material.

* `4 float` -> Ambient color.

* `4 float` -> Emissive color.

* `4 float` -> Diffuse color.

* `4 float` -> Specular color.

* `4 float` -> Reflectance. Unused for now.

* `1 float` -> Opacity. The entire object will be applied this alpha. 1 for
  completely opaque.

* `1 float` -> Specular power. Shininess.

* `1 float` -> Index of refraction. Unused for now.

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

* `4 float` -> Color of the light. Found in the light object's data.

* `1 float` -> Constant attenuation of the light. From Blender, it's always 0.

* `1 float` -> Linear attenuation of the light. From Blender, it's calculated
  from the light's "distance" (light data > custom distance). The formula is `l
  = 1000/d - d`.
  
* `1 float` -> Quadratic attenuation of the light. From Blender, it's always 1.

* `1 float` -> Intensity of the light. Found in the light object's data as the
  power.

* `1 uint8` -> Type of light:

  - 0 means Spot Light
  
  - 1 means Directional (Sun) Light
  
  - 2 means Point Light
  
* `1 float` -> Angle of the light in radians (only meaningful for spot
  lights). Found in the light object's data, under Spot Shape.

Non meaningful fields like angle for a non spot light must still be present
with any valid value.

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

* `1 uint8` -> Whether it is the camera. 0 if it isn't, any other value if it
  is. One and only one object should be the camera.
  
* `1 uint32` -> Geometry index: The first geometry defined in the file is
  index 1. Index 0 means no geometry.
  
* `1 uint32` -> Material index: Same strategy as the geometry index, 0 means no
  material. All objects with a geometry must have a material, otherwise crashes
  may occur.
  
* `1 uint32` -> Light index: Same strategy as with geometry and
  material. Generally, a light object doesn't have a geometry or material.
  
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
