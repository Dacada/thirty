#include <camera.h>
#include <util.h>
#include <cglm/struct.h>
#include <stdint.h>

void camera_init(struct camera *const cam, const float aspect,
                 const float near, const float far, const float fov) {
        cam->aspect = aspect;
        cam->near = near;
        cam->far = far;
        cam->fov = fov;
}

void camera_initFromFile(struct camera *const cam, FILE *const f) {
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

        camera_init(cam, (float)width / (float)height, near, far, fov);
}

mat4s camera_viewMatrix(const struct camera *const cam, const mat4s model) {
        (void)cam;
        return glms_mat4_inv(model);
}

mat4s camera_projectionMatrix(const struct camera *const cam) {
        return glms_perspective(cam->fov, cam->aspect, cam->near, cam->far);
}
