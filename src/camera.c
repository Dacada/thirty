#include <camera.h>
#include <util.h>
#include <cglm/struct.h>
#include <stdint.h>

void *camera_new(const enum cameraType type) {
        switch (type) {
        case CAMERA_BASIC:
                return smalloc(sizeof(struct basicCamera));
        case CAMERA_FPS:
                return smalloc(sizeof(struct fpsCamera));
        default:
                assert_fail();
        }
}

void camera_init(struct camera *const cam, const float aspect,
                 const float near, const float far, const float fov,
                 const enum cameraType type) {
        cam->aspect = aspect;
        cam->near = near;
        cam->far = far;
        cam->fov = fov;
        cam->type = type;

        switch (type) {
        case CAMERA_BASIC:
                break;
        case CAMERA_FPS:
                ((struct fpsCamera*)cam)->pitch = 0;
                ((struct fpsCamera*)cam)->yaw = 0;
                ((struct fpsCamera*)cam)->position = GLMS_VEC3_ZERO;
                break;
        default:
                assert_fail();
        }
}

void camera_initFromFile(struct camera *const cam, FILE *const f,
                         const enum cameraType type) {
        uint32_t width;
        uint32_t height;
        float near;
        float far;
        float fov;

        sfread(&width, sizeof(width), 1, f);
        sfread(&height, sizeof(height), 1, f);
        sfread(&near, sizeof(near), 1, f);
        sfread(&far, sizeof(far), 1, f);
        sfread(&fov, sizeof(fov), 1, f);

        camera_init(cam, (float)width / (float)height, near, far, fov, type);
}

mat4s camera_viewMatrix(const struct camera *const cam, mat4s model) {
        switch (cam->type) {
        case CAMERA_BASIC:
                return glms_mat4_inv(model);
        case CAMERA_FPS:
                ;
                vec3s eye = ((const struct fpsCamera *)cam)->position;
                const float pitch = ((const struct fpsCamera *)cam)->pitch;
                const float yaw = ((const struct fpsCamera *)cam)->yaw;
                glm_vec3_add(model.col[3].raw, eye.raw, model.col[3].raw);
                model = glms_rotate_y(model, yaw);
                model = glms_rotate_x(model, pitch);
                return glms_mat4_inv(model);
        default:
                assert_fail();
        }
}

mat4s camera_projectionMatrix(const struct camera *const cam) {
        return glms_perspective(cam->fov, cam->aspect, cam->near, cam->far);
}
