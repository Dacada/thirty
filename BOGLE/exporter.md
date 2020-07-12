# Format of my custom data format: BOGLE (version 0)

BOGLE meaning Blender to OpenGL Exporter.

The extension is .bgl :)

All numbers are little-endian encoded. All matrices are in column-major order.

There's one header, then for each object an object header and object data. And
then the object tree. Then information about the lights and cameras.

```
HEADER
OBJECT 1 HEADER
OBJECT 1 DATA
OBJECT 2 HEADER
OBJECT 2 DATA
...
OBJECT N HEADER
OBJECT N DATA
OBJECT TREE
CAMERA
LIGHTS
GLOBAL AMBIENT LIGHT
MATERIALS
OBJECT'S MATERIALS
```

## Header

* `5` bytes -> File Signature ("BOGLE" 0x42 0x4f 0x47 0x4c 0x45)

* `1` byte -> Version number (Version 0, so zero). Unsigned.

* `4` bytes -> Number of objects defined. Unsigned.

* `4` bytes -> Number of lights defined. Unsigned.

* `4` bytes -> Number of materials defined. Unsigned.

## Object header

* `4` bytes -> The number of vertices in the object. `vertlen` Unsigned. Can be
  0 for objects with no geometry.

* `4` bytes -> The number of indices in the object. `indlen` Unsigned. Can be 0
  for objects with no geometry.

## Object data

* `32` bytes -> The name of the object. 31 characters at most for the name,
  with the last being a 0 to mark end of string. Examples: `SOME_NAME\0 ` or
  `THIS_IS_A_VEERY_VEERY_LOONG_NAME\0`. Characters after the 0 if any can be
  whatever.

* `vertlen*14*4` bytes -> The vertex data, more on that below. Floats. Can be
  empty if `vertlen` is 0.

* `indlen*4` bytes -> The index data. Unsigned integers. Can be empty is
  `indlen` is 0.

* `12` bytes -> Translation vector of the object.

* `12` bytes -> Rotation axis of the object.

* `4` bytes -> Rotation angle of the object.

* `12` bytes -> Scale vector of the object.

### Vertices

Vertices are outlined as follows: 3 floats for the vertex coordinate, 2 floats
for the texture coordinate, 3 floats for the normal, 3 floats for the tangent
and 3 floats for the binormal.

The texture coordinates will always be in the range [0, 1.0].

## Object tree

This is a string of characters (one byte each) specifying how the object tree
should be formed.

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

## Camera

Only one camera:

* `12` bytes -> Position vector of the camera.

* `4` bytes -> Yaw. Float.

* `4` bytes -> Pitch. Float.

## Lights

For each light, this structure repeats:

* `16` bytes -> Color of the light.

* `4` bytes -> Range of the light.

* `4` bytes -> Intensity of the light.

* `4` bytes -> Type of light:

  - 0 means Spot Light
  
  - 1 means Directional Light
  
  - 2 means Point Light
  
* `16` bytes -> Position of the light (only meaningful for point and spot
  lights)
  
* `16` bytes -> Direction of the light (only meaningful for spot and
  directional lights)
  
* `4` bytes -> Angle of the light in degrees (only meaningful for spot lights)

Non meaningful fields like angle for a non spot light must still be present
with any valid value.

## Global Ambient Light

For now, this is hard coded in the exporter script to be <0.1, 0.1, 0.1, 1.0>.

* `16` bytes -> Color to add as global ambient light when shading.

## Materials

For now only one kind of material exists, this section might need to be
expanded if more materials are added.

* `4` bytes -> Material type. Reserved in case there's more than one material. Should be 0.

* `4` bytes -> Shader type. Reserved in case there's more than one shader. Should be 0.

* `16` bytes -> Ambient color.

* `16` bytes -> Emissive color.

* `16` bytes -> Diffuse color.

* `16` bytes -> Specular color.

* `16` bytes -> Reflectance. Unused for now.

* `4` bytes -> Opacity. The entire object will be applied this alpha. 1 for completely opaque.

* `4` bytes -> Specular power. Shininess.

* `4` bytes -> Index of refraction. Unused for now.

* `4` bytes -> Bump intensity. Scale values of a bump map, if there's any.

* `4` bytes -> Specular scale. Scale values from the specular power map, if there's any.

* `4` bytes -> Alpha threshold. Unused for now.

Next, each texture is defined. It's defined as simply the name of the texture
file, without extension (will look for a png). Before each name comes the
number of characters in the name, or 0 if there's no texture. In which case
there should be 0 characters (nothing) and then continue to define the next
texture.

* `4` bytes -> Number of characters in the texture name. `nchars`

* `nchars` bytes -> The name of the png file with the texture.

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

## Object's Materials

As many entries as there are objects. Objects without geometry can have any
material as long as it's a valid one. They won't be rendered.

* `4` bytes -> Material index
