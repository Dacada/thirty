#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/struct.h>

/*
 * Generic prespective camera functions. Initialize camera, either from a BOGLE
 * file or from parameters. The view matrix is simply derived from the model
 * matrix.
 */

enum cameraType {
        CAMERA_BASIC,
        CAMERA_FPS
};

struct camera {
        float aspect;
        float near, far;
        float fov;
        enum cameraType type;
};

struct basicCamera {
        struct camera base;
};

struct fpsCamera {
        struct camera base;
        float pitch, yaw;
        vec3s position;
};

void *camera_new(enum cameraType type)
        __attribute__((leaf))
        __attribute__((malloc))
        __attribute__((nonnull));

void camera_init(struct camera *cam, float aspect,
                 float near, float far, float fov, enum cameraType type)
        __attribute__((access (write_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void camera_initFromFile(struct camera *cam, FILE *f, enum cameraType type)
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

#endif /* CAMERA_H */
