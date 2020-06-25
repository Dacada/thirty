#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/struct.h>

/*
 * FPS camera functions. Passing NULL to any parameter (except the first three)
 * of camera_init uses a hard-coded default. The rest of the functions do as
 * their name says: Generate the view matrix, generate the
 * perspective/projection matrix, move the character, look around with the
 * mouse, and change the level of zoom.
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

void camera_init(struct camera *restrict cam, float width, float height,
                 const vec3s *position, const vec3s *worldup,
                 const float *yaw, const float *pitch,
                 const float *near, const float *far);

mat4s camera_viewMatrix(const struct camera *restrict cam);
mat4s camera_projectionMatrix(const struct camera *restrict cam);

void camera_move(struct camera *restrict cam,
                 enum camera_movement mov, float deltaTime, bool freefly);

void camera_look(struct camera *restrict cam,
                 float xoffset, float yoffset, bool constrain_pitch);

void camera_zoom(struct camera *restrict cam, float offset);

#endif /* CAMERA_H */
