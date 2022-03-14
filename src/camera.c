#include <thirty/camera.h>
#include <thirty/util.h>

void camera_init(struct camera *const cam, const char *const name,
                 const float aspect, const float near, const float far,
                 const float fov, const bool main,
                 const enum componentType type) {
        assert(type == COMPONENT_CAMERA);
        
        component_init((struct component*)cam, name);
        
        cam->base.type = type;
        cam->main = main;
        cam->aspect = aspect;
        cam->near = near;
        cam->far = far;
        cam->fov = fov;
}

size_t camera_initFromFile(struct camera *const cam, FILE *const f,
                           const enum componentType type,
                           struct varSizeGrowingArray *const components) {
        assert(type == COMPONENT_CAMERA);
        (void)components;
        
        uint32_t width;
        uint32_t height;
        float near;
        float far;
        float fov;
        uint8_t main;

        char *name = strfile(f);

        sfread(&width, sizeof(width), 1, f);
        sfread(&height, sizeof(height), 1, f);
        sfread(&near, sizeof(near), 1, f);
        sfread(&far, sizeof(far), 1, f);
        sfread(&fov, sizeof(fov), 1, f);
        sfread(&main, sizeof(main), 1, f);

        camera_init(cam, name, (float)width / (float)height,
                    near, far, fov, main, type);
        free(name);

        return sizeof(struct camera);
}

mat4s camera_viewMatrix(const struct camera *const cam, mat4s model) {
        assert(cam->base.type == COMPONENT_CAMERA);
        return glms_mat4_inv(model);
}

mat4s camera_projectionMatrix(const struct camera *const cam) {
        assert(cam->base.type == COMPONENT_CAMERA);
        return glms_perspective(cam->fov, cam->aspect, cam->near, cam->far);
}

void camera_free(struct camera *cam) {
        assert(cam->base.type == COMPONENT_CAMERA);
        component_free((struct component*)cam);
}
