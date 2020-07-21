#ifndef INPUT_HELPERS_H
#define INPUT_HELPERS_H

#include <cglm/struct.h>

struct fpsCameraController {
        bool freefly;
        float move_sensitivity;
        float look_sensitivity;
        const struct object *camera_obj;
        struct fpsCamera *camera;
};

void fpsCameraController_init(struct fpsCameraController *ctrl,
                              float move, float look,
                              const struct object *camera);

void fpsCameraController_move(const struct fpsCameraController *ctrl,
                              vec2s direction, float timeDelta);

void fpsCameraController_look(const struct fpsCameraController *ctrl,
                              vec2s direction, float timeDelta);

#endif /* INPUT_HELPERS */
