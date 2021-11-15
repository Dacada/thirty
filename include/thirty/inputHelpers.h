#ifndef INPUT_HELPERS_H
#define INPUT_HELPERS_H

#include <thirty/scene.h>
#include <cglm/struct.h>

/*
 * This file contins various structs and functions to help with various input
 * handling tasks.
 */

/*
 * This is used for handling an FPS camera.
 */
struct fpsCameraController {
        bool freefly;
        float move_sensitivity;
        float look_sensitivity;
        struct scene *scene;
};

/*
 * Initialize the FPS Camera Controller with the given parameters. Move and
 * look are the corresponding sensitivities, which might need to be
 * calibrated.
 */
void fpsCameraController_init(struct fpsCameraController *const ctrl,
                              struct scene *const scene,
                              const float move, const float look)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

/*
 * Move the camera in the given 2D direction, taking into account the
 * timeDelta. The camera object is not modified. Returns the camera's new
 * position vector.
 */
vec3s fpsCameraController_move(const struct fpsCameraController *ctrl,
                               vec2s direction, float timeDelta,
                               const struct object *const camera_obj)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_only, 4)))
        __attribute__((nonnull));

/*
 * Pivot the camera in the given 2D direction, taking into account the
 * timeDelta. The camera object is not modified. Returns the camera's new yaw
 * and pitch as a 2D vector (yaw, pitch).
 */
vec2s fpsCameraController_look(const struct fpsCameraController *ctrl,
                              vec2s direction, float timeDelta,
                              const struct object *const camera_obj)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_only, 4)))
        __attribute__((nonnull));

#endif /* INPUT_HELPERS */
