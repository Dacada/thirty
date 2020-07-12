#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/struct.h>

/*
 * FPS camera functions. Passing NULL to any parameter (except the first three)
 * of camera_init uses a hard-coded default. The rest of the functions do as
 * their name says: Generate the view matrix, generate the
 * perspective/projection matrix, move the camera, look around , and change the
 * level of zoom.
 */

enum camera_movement {
        camera_FORWARD,
        camera_BACKWARD,
        camera_LEFT,
        camera_RIGHT
};

struct camera {
        vec3s position, front, right, up, world_up;
        float width, height;
        float near, far;
        float yaw, pitch; // radians
        float movement_speed, look_sensitivity, zoom_level;
};

void camera_init(struct camera *cam, float width, float height,
                 const vec3s *position, const vec3s *worldup,
                 const float *yaw, const float *pitch,
                 const float *near, const float *far)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 4)))
        __attribute__((access (read_only, 5)))
        __attribute__((access (read_only, 6)))
        __attribute__((access (read_only, 7)))
        __attribute__((access (read_only, 8)))
        __attribute__((access (read_only, 9)))
        __attribute__((leaf))
        __attribute__((nonnull (1)));

void camera_initFromFile(struct camera *cam,
                         float width, float height, FILE *f)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 4)))
        __attribute__((leaf))
        __attribute__((nonnull));

mat4s camera_viewMatrix(const struct camera *cam)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

mat4s camera_projectionMatrix(const struct camera *cam)
        __attribute__((access (read_only, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void camera_move(struct camera *cam, enum camera_movement mov,
                 float deltaTime, bool freefly)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void camera_look(struct camera *cam, float xoffset, float yoffset,
                 bool constrain_pitch)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

void camera_zoom(struct camera *cam, float offset)
        __attribute__((access (read_write, 1)))
        __attribute__((leaf))
        __attribute__((nonnull));

#endif /* CAMERA_H */
