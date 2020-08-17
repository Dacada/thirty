#ifndef ANIMATION_H
#define ANIMATION_H

#include <skeleton.h>
#include <shader.h>
#include <stdio.h>

extern void(*animation_onAnimateSkeleton)(const struct skeleton *);

struct animation {
        char *name;
        size_t nkeyframes;
        struct keyframe *keyframes;
};

void animation_initFromFile(struct animation *anim, FILE *f, size_t nbones)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

void animation_bindBones(const struct animation *anim,
                         const struct skeleton *skel,
                         float timestamp, enum shaders shader)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull (2)));

void animation_free(struct animation *anim)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif
