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
CAMERAS
LIGHTS
```

## Header

* `5` bytes -> File Signature ("BOGLE" 0x42 0x4f 0x47 0x4c 0x45)

* `1` byte -> Version number (Version 0, so zero). Unsigned.

* `4` bytes -> Number of objects defined. Unsigned.

* `4` bytes -> Number of cameras defined. Unsigned.

* `4` bytes -> Number of lights defined. Unsigned.

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

* `vertlen*8*4` bytes -> The vertex data, more on that below. Floats. Can be
  empty if `vertlen` is 0.

* `indlen*4` bytes -> The index data. Unsigned integers. Can be empty is
  `indlen` is 0.

* `12` bytes -> Translation vector of the object.

* `12` bytes -> Rotation axis of the object.

* `4` bytes -> Rotation angle of the object.

* `12` bytes -> Scale vector of the object.

### Vertices

Vertices are outlined in the typical OpenGL fashion: 3 floats for the vertex
coordinate, 2 floats for the texture coordinate, 3 floats for the normal.

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

## Cameras

For each camera, this structure repeats:

* `12` bytes -> Position vector of the camera.

* `4` bytes -> Yaw. Float.

* `4` bytes -> Pitch. Float.

## Lights

For each light, this structure repeats:

* `12` bytes -> Position vector of the light.

* `16` bytes -> Ambient color of the light.

* `16` bytes -> Diffuse color of the light.

* `16` bytes -> Specular color of the light.

* `4` bytes -> Ambient power of the light.

* `4` bytes -> Diffuse power of the light.

* `4` bytes -> Specular power of the light.
