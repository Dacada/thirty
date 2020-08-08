#include <camera.h>
#include <component.h>
#include <util.h>
#include <cglm/struct.h>
#include <stdint.h>

void camera_init(struct camera *const cam, const float aspect,
                 const float near, const float far, const float fov,
                 const bool main,
                 const enum componentType type) {
        cam->base.type = type;
        cam->main = main;
        cam->aspect = aspect;
        cam->near = near;
        cam->far = far;
        cam->fov = fov;

        switch (type) {
        case COMPONENT_CAMERA_BASIC:
                break;
        case COMPONENT_CAMERA_FPS:
                ((struct camera_fps*)cam)->pitch = 0;
                ((struct camera_fps*)cam)->yaw = 0;
                ((struct camera_fps*)cam)->position = GLMS_VEC3_ZERO;
                break;
        case COMPONENT_GEOMETRY:
        case COMPONENT_LIGHT_DIRECTION:
        case COMPONENT_LIGHT_POINT:
        case COMPONENT_LIGHT_SPOT:
        case COMPONENT_MATERIAL_UBER:
        case COMPONENT_MATERIAL_SKYBOX:
        case COMPONENT_TOTAL:
        default:
                assert_fail();
        }
}

size_t camera_initFromFile(struct camera *const cam, FILE *const f,
                           const enum componentType type) {
        uint32_t width;
        uint32_t height;
        float near;
        float far;
        float fov;
        uint8_t main;

        sfread(&width, sizeof(width), 1, f);
        sfread(&height, sizeof(height), 1, f);
        sfread(&near, sizeof(near), 1, f);
        sfread(&far, sizeof(far), 1, f);
        sfread(&fov, sizeof(fov), 1, f);
        sfread(&main, sizeof(main), 1, f);

        camera_init(cam, (float)width / (float)height,
                    near, far, fov, main, type);
        
        switch (type) {
        case COMPONENT_CAMERA_BASIC:
                return sizeof(struct camera_basic);
        case COMPONENT_CAMERA_FPS:
                return sizeof(struct camera_fps);
        case COMPONENT_GEOMETRY:
        case COMPONENT_LIGHT_DIRECTION:
        case COMPONENT_LIGHT_POINT:
        case COMPONENT_LIGHT_SPOT:
        case COMPONENT_MATERIAL_UBER:
        case COMPONENT_MATERIAL_SKYBOX:
        case COMPONENT_TOTAL:
        default:
                assert_fail();
        }
}

mat4s camera_viewMatrix(const struct camera *const cam, mat4s model) {
        switch (cam->base.type) {
        case COMPONENT_CAMERA_BASIC:
                return glms_mat4_inv(model);
        case COMPONENT_CAMERA_FPS:
                ;
                vec3s eye = ((const struct camera_fps *)cam)->position;
                const float pitch = ((const struct camera_fps *)cam)->pitch;
                const float yaw = ((const struct camera_fps *)cam)->yaw;
                glm_vec3_add(model.col[3].raw, eye.raw, model.col[3].raw);
                model = glms_rotate_y(model, yaw);
                model = glms_rotate_x(model, pitch);
                return glms_mat4_inv(model);
        case COMPONENT_GEOMETRY:
        case COMPONENT_LIGHT_DIRECTION:
        case COMPONENT_LIGHT_POINT:
        case COMPONENT_LIGHT_SPOT:
        case COMPONENT_MATERIAL_UBER:
        case COMPONENT_MATERIAL_SKYBOX:
        case COMPONENT_TOTAL:
        default:
                assert_fail();
        }
}

mat4s camera_projectionMatrix(const struct camera *const cam) {
        return glms_perspective(cam->fov, cam->aspect, cam->near, cam->far);
}
