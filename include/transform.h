#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <component.h>
#include <cglm/struct.h>
#include <stdio.h>

/*
 * This component implements the transform (model) matrix of an object. Every
 * object should have this component.
 */

struct transform {
        struct component base;
        mat4s model;
};

void transform_init(struct transform *trans, mat4s model)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

void transform_initFromFile(struct transform *trans, FILE *f,
                            enum componentType type)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

/*
 * Translate object model matrix by given vector.
 */
void transform_translate(struct transform *trans, vec3s delta)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Translate object model matrix by given X component.
 */
void transform_translateX(struct transform *trans, float delta)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Translate object model matrix by given Y component.
 */
void transform_translateY(struct transform *trans, float delta)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Translate object model matrix by given Z component.
 */
void transform_translateZ(struct transform *trans, float delta)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Rotate object model matrix by given angle and axis.
 */
void transform_rotate(struct transform *trans, float angle, vec3s axis)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Rotate object model matrix by given X Euler angle.
 */
void transform_rotateX(struct transform *trans, float angle)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Rotate object model matrix by given Y Euler angle.
 */
void transform_rotateY(struct transform *trans, float angle)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Rotate object model matrix by given Z Euler angle.
 */
void transform_rotateZ(struct transform *trans, float angle)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Rotate object model matrix by given rotation matrix.
 */
void transform_rotateMat(struct transform *trans, mat4s rotation)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

/*
 * Scale object model matrix by given vector.
 */
void transform_scale(struct transform *trans, vec3s scale)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void transform_free(struct transform *trans)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif
