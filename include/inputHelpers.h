#ifndef INPUT_HELPERS_H
#define INPUT_HELPERS_H

#include <scene.h>
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
 * calibrated. The camera object should be an object with a camera component
 * (must be FPS camera component).
 */
void fpsCameraController_init(struct fpsCameraController *const ctrl,
                              struct scene *const scene,
                              const float move, const float look)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_write, 2)))
        __attribute__((nonnull));

/*
 * Move the camera in the given 2D direction, taking into account the
 * timeDelta.
 */
void fpsCameraController_move(const struct fpsCameraController *ctrl,
                              vec2s direction, float timeDelta,
                              struct object *const camera_obj)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_write, 4)))
        __attribute__((nonnull));

/*
 * Pivot the camera in the given 2D direction, taking into account the
 * timeDelta.
 */
void fpsCameraController_look(const struct fpsCameraController *ctrl,
                              vec2s direction, float timeDelta,
                              struct object *const camera_obj)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_write, 4)))
        __attribute__((nonnull));

#endif /* INPUT_HELPERS */
