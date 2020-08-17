#ifndef SKELETON_H
#define SKELETON_H

#include <shader.h>
#include <cglm/struct.h>
#include <stdio.h>

struct skeleton {
        mat4s model;
        size_t nbones;
        struct bone *bones;
        size_t *boneOrder;  // bone indices, if bones are accessed in this
                            // order, then each parent will always be accessed
                            // before their children
};

void skeleton_initFromFile(struct skeleton *skel, FILE *f)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

void skeleton_initFromRelativeRotations(struct skeleton *skel,
                                        const struct skeleton *base,
                                        const versors *relativeRotations,
                                        vec3s rootOffset)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((access (read_only, 3)))
        __attribute__((nonnull));

void skeleton_bindBones(const struct skeleton *skel, enum shaders shader)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

void skeleton_free(struct skeleton *skel)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif
