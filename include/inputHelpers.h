#ifndef INPUT_HELPERS_H
#define INPUT_HELPERS_H

#include <cglm/struct.h>
#include <stdbool.h>

struct fpsCameraController {
        bool freefly;
        float move_sensitivity;
        float look_sensitivity;
        const struct object *camera_obj;
        struct camera_fps *camera;
};

void fpsCameraController_init(struct fpsCameraController *ctrl,
                              float move, float look,
                              const struct object *camera)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 4)))
        __attribute__((nonnull));

void fpsCameraController_move(const struct fpsCameraController *ctrl,
                              vec2s direction, float timeDelta)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

void fpsCameraController_look(const struct fpsCameraController *ctrl,
                              vec2s direction, float timeDelta)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

#endif /* INPUT_HELPERS */
