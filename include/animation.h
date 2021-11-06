#ifndef ANIMATION_H
#define ANIMATION_H

#include <skeleton.h>

/*
 * Represents a single animation. These are contaned and managed by an
 * animationCollection.
 */

struct animation {
        char *name;
        size_t nkeyframes;
        struct keyframe *keyframes;
};

/*
 * Initialize an animation from a BOGLE file positioned at the correct offset.
 */
void animation_initFromFile(struct animation *anim, FILE *f, size_t nbones)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

/*
 * Use animation data and a timestamp to create a posed skeleton for the right
 * animation frame and bind its bones to the given shader.
 */
void animation_bindBones(const struct animation *anim,
                         const struct skeleton *skel,
                         float timestamp, enum shaders shader)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull (2)));

/*
 * Free resources used by an animation, deinitializing it.
 */
void animation_free(struct animation *anim)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif
