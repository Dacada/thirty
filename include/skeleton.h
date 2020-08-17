#ifndef SKELETON_H
#define SKELETON_H

#include <keyframe.h>
#include <shader.h>
#include <cglm/struct.h>
#include <stdio.h>

/*
 * A skeleton containing bones for animation, as well as its location in
 * relation to the object it is animating.
 */

struct skeleton {
        mat4s model;
        size_t nbones;
        struct bone *bones;
        size_t *boneOrder;  // bone indices, if bones are accessed in this
                            // order, then each parent will always be accessed
                            // before their children
};

/*
 * Initialize a skeleton from a BOGLE file at the correct offset.
 */
void skeleton_initFromFile(struct skeleton *skel, FILE *f)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

/*
 * Initialize a skeleton from a keyframe, creating a new skeleton with each
 * bone's absolute matrices changed relatively by the keyframe but without
 * changing the base's bind inverse matrices.
 */
void skeleton_initFromKeyframe(struct skeleton *skel,
                               const struct skeleton *base,
                               const struct keyframe *keyframe)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((access (read_only, 3)))
        __attribute__((nonnull));

/*
 * Bind a skeleton's bones to the given shader, using absolute and bind inverse
 * matrices.
 */
void skeleton_bindBones(const struct skeleton *skel, enum shaders shader)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Free resources used by a skeleton, uninitializing it.
 */
void skeleton_free(struct skeleton *skel)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif
