#ifndef CAMERA_H
#define CAMERA_H

#include <thirty/component.h>
#include <thirty/dsutils.h>
#include <cglm/struct.h>

/*
 * This component has a generic prespective camera.
 */

struct camera {
        struct component base;
        bool main;
        float aspect;
        float near, far;
        float fov;
};

/*
 * Initialize a camera with given parameters.
 */
void camera_init(struct camera *cam, const char *name, float aspect,
                 float near, float far, float fov, bool main,
                 enum componentType type)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((nonnull));

/*
 * Initialize a camera from a BOGLE file positioned at the correct offset.
 */
size_t camera_initFromFile(struct camera *cam, FILE *f, enum componentType type,
                           struct varSizeGrowingArray *components)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((access (read_write, 4)))
        __attribute__((nonnull));

/*
 * Obtain a camera's view matrix.
 */
mat4s camera_viewMatrix(const struct camera *cam, mat4s model)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Obtain a camera's projection matrix.
 */
mat4s camera_projectionMatrix(const struct camera *cam)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

/*
 * Free resources used by a camera, deinitializing it.
 */
void camera_free(struct camera *cam)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#define CAMERA_MAXIMUM_SIZE sizeof(struct camera)

#endif /* CAMERA_H */
