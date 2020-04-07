# Format of my custom data format: BOGLE (version 0)

BOGLE meaning Blender to OpenGL Exporter.

The extension is .bgl :)

All numbers are little-endian encoded.

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

* `vertlen*5*4` bytes -> The vertex data, more on that below. Floats.

* `indlen*4` bytes -> The index data. Unsigned integers.

## Vertices

Vertices are outlined in the typical OpenGl fashion: 3 floats for the
vertex coordinate, 2 floats for the texture coordinate.

## That's it?

Yes. More will be added in the future. Immediately in the future,
normal data. And maybe at some point an entire scene instead of just
individual objects.
