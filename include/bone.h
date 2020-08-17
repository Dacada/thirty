#ifndef BONE_H
#define BONE_H

#include <cglm/struct.h>
#include <stdio.h>

struct bone {
        vec3s positionRelative;
        versors rotationRelative;
        size_t parent;
        mat4s absoluteTransform;
        mat4s bindPoseInv;
};

void bone_initFromFile(struct bone *bone, FILE *f)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

#endif
