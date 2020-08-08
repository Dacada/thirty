#ifndef CAMERA_H
#define CAMERA_H

#include <component.h>
#include <cglm/struct.h>

/*
 * Generic prespective camera functions. Initialize camera, either from a BOGLE
 * file or from parameters. The view matrix is simply derived from the model
 * matrix.
 */

struct camera {
        struct component base;
        bool main;
        float aspect;
        float near, far;
        float fov;
};

struct camera_basic {
        struct camera base;
};

struct camera_fps {
        struct camera base;
        float pitch, yaw;
        vec3s position;
};

void camera_init(struct camera *cam, float aspect,
                 float near, float far, float fov, bool main,
                 enum componentType type)
        __attribute__((access (write_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

size_t camera_initFromFile(struct camera *cam, FILE *f, enum componentType type)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((leaf))
        __attribute__((nonnull));

mat4s camera_viewMatrix(const struct camera *cam, mat4s model)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

mat4s camera_projectionMatrix(const struct camera *cam)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#define CAMERA_MAXIMUM_SIZE max(sizeof(struct camera_basic),\
                                sizeof(struct camera_fps))

#endif /* CAMERA_H */
