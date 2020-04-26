# Format of my custom data format: BOGLE (version 0)

BOGLE meaning Blender to OpenGL Exporter.

The extension is .bgl :)

All numbers are little-endian encoded. All matrices are in column-major order.

There's one header, then for each object an object header and object
data.

```
HEADER
OBJECT 1 HEADER
OBJECT 1 DATA
OBJECT 2 HEADER
OBJECT 2 DATA
...
OBJECT N HEADER
OBJECT N DATA
```

## Header

* `5` bytes -> File Signature ("BOGLE" 0x42 0x4f 0x47 0x4c 0x45)

* `1` byte -> Version number (Version 0, so zero). Unsigned.

* `4` bytes -> Number of objects defined. Unsigned.

## Object header

* `1` byte -> Number of characters in the name. If 0, there's no
  name. `namelen`. Unsigned.

* `4` bytes -> The number of vertices in the object. `vertlen`
  Unsigned.

* `4` bytes -> The number of indices in the object. `indlen` Unsigned.

## Object data

* `namelen` bytes -> The name of the object, if any.

* `vertlen*8*4` bytes -> The vertex data, more on that below. Floats.

* `indlen*4` bytes -> The index data. Unsigned integers.

* `12` bytes -> Translation vector of the object.

* `12` bytes -> Rotation axis of the object.

* `4` bytes -> Rotation angle of the object.

* `12` bytes -> Scale vector of the object.

## Vertices

Vertices are outlined in the typical OpenGL fashion: 3 floats for the vertex
coordinate, 2 floats for the texture coordinate, 3 floats for the normal.
