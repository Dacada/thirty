#ifndef ANIMATION_COLLECTION_H
#define ANIMATION_COLLECTION_H

#include <skeleton.h>
#include <component.h>
#include <shader.h>
#include <stdio.h>
#include <stdbool.h>

#define ANIMATION_FRAME_TO_TIMESTAMP(frame, framerate)  \
        ((float)(frame)/(float)(framerate))

struct animationCollection {
        struct component base;
        
        struct skeleton skeleton;
        
        size_t nanimations;
        struct animation *animations;

        bool running;
        size_t current;
        float time;
};

size_t animationCollection_initFromFile(struct animationCollection *col,
                                        FILE *f, enum componentType type)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

size_t animationCollection_idxByName(struct animationCollection *col,
                                     const char *name)
        __attribute__((access (read_write, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

void animationCollection_playAnimation(struct animationCollection *col,
                                       size_t anim)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void animationCollection_poseAnimation(struct animationCollection *col,
                                       size_t anima, float timestamp)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void animationCollection_setBindPose(struct animationCollection *col)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void animationCollection_bindBones(const struct animationCollection *col,
                                   enum shaders shader)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

void animationCollection_update(struct animationCollection *col)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void animationCollection_free(struct animationCollection *col)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif
