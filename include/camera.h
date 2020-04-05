#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/struct.h>

/*
 * FPS camera functions. Passing NULL to any parameter (except the first) of
 * camera_init uses a hard-coded default. The rest of the functions do as their
 * name says: Generate the view matrix, move the character, look around with
 * the mouse, and change the level of zoom.
 */

enum camera_movement {
        camera_FORWARD,
        camera_BACKWARD,
        camera_LEFT,
        camera_RIGHT
};

struct camera {
        vec3s position, front, right, up, world_up;
        float yaw, pitch; // radians
        float movement_speed, look_sensitivity, zoom_level;
};

void camera_init(struct camera *cam, const vec3s *position,
                 const vec3s *up, const float *yaw, const float *pitch);

mat4s camera_viewMatrix(const struct camera *const cam);

void camera_move(struct camera *cam,
                 enum camera_movement mov, float deltaTime, bool freefly);

void camera_look(struct camera *cam,
                 float xoffset, float yoffset, bool constrain_pitch);

void camera_zoom(struct camera *cam, float offset);

#endif /* CAMERA_H */
