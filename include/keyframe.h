#ifndef KEYFRAME_H
#define KEYFRAME_H

#include <cglm/struct.h>
#include <stdio.h>

struct keyframe {
        float timestamp;
        vec3s rootOffset;
        size_t nbones;
        versors *relativeBoneRotations;
};

void keyframe_initFromFile(struct keyframe *keyframe, FILE *f, size_t nbones)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

void keyframe_free(struct keyframe *keyframe)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif
