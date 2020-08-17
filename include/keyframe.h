#ifndef KEYFRAME_H
#define KEYFRAME_H

#include <cglm/struct.h>
#include <stdio.h>

/*
 * Represents an animation keyframe. They contain all the relative rotations of
 * the bones and the positional offset of the root bone for each
 * timestamp. Actial animation poses are calculated on the fly by interpolating
 * between two of these frames.
 */

struct keyframe {
        float timestamp;
        vec3s rootOffset;
        size_t nbones;
        versors *relativeBoneRotations;
};

/*
 * Initialize a keyframe from a BOGLE file positioned at the correct offset.
 */
void keyframe_initFromFile(struct keyframe *keyframe, FILE *f, size_t nbones)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

/*
 * Interpolate between the two given frames, initializing the third frame from
 * the result at the given timestamp. Timestamp must be between the two given
 * frames.
 */
void keyframe_initFromInterp(const struct keyframe *prev,
                             const struct keyframe *next,
                             struct keyframe *keyframe, float timestamp)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((access (write_only, 3)))
        __attribute__((nonnull));

/*
 * Free all resources used by a keyframe, deinitializing it.
 */
void keyframe_free(struct keyframe *keyframe)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif
