#include <inputHelpers.h>
#include <game.h>
#include <componentCollection.h>
#include <eventBroker.h>
#include <util.h>
#include <cglm/struct.h>
#include <GLFW/glfw3.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TITLE_BUFFER_SIZE 256
#define FRAME_PERIOD_FPS_REFRESH 10
#define CAMERA_PITCH_LIMIT_OFFSET 0.1F
#define STARTING_CAMERA_Z_POSITION 10.0F
#define CAMERA_NEAR 0.1F
#define CAMERA_FAR 100.0F
#define CAMERA_FOV 45.0F

static const vec4s globalAmbientLight = {.x=0.3F, .y=0.3F, .z=0.3F, .w=1.0F};
static const float movement_speed = 10.0F;
static const float look_sensitivity = 0.1F;

static struct fpsCameraController cam_ctrl;
static void processMouseInput(void *registerArgs, void *fireArgs) {
        struct game *game = registerArgs;
        struct eventBrokerMousePosition *args = fireArgs;
        const double xpos = args->xpos;
        const double ypos = args->ypos;
        
        const vec2s curr = {
                .x = (float)xpos,
                .y = (float)ypos,
        };

        static bool first = true;
        static vec2s prev;
        if (first) {
                prev = curr;
                first = false;
        }

        const vec2s offset = glms_vec2_sub(prev, curr);
        fpsCameraController_look(&cam_ctrl, offset, game_timeDelta(game));

        prev = curr;
}

static void processKeyboardInput(void *registerArgs, void *fireArgs) {
        struct game *game = registerArgs;
        (void)fireArgs;
        
        vec2s movement = {.x=0, .y=0};
        
        if (game_keyPressed(game, GLFW_KEY_LEFT) ||
            game_keyPressed(game, GLFW_KEY_A)) {
                movement.x -= 1;
        }
        if (game_keyPressed(game, GLFW_KEY_RIGHT) ||
            game_keyPressed(game, GLFW_KEY_D)) {
                movement.x += 1;
        }
        
        if (game_keyPressed(game, GLFW_KEY_UP) ||
            game_keyPressed(game, GLFW_KEY_W)) {
                movement.y += 1;
        }
        if (game_keyPressed(game, GLFW_KEY_DOWN) ||
            game_keyPressed(game, GLFW_KEY_S)) {
                movement.y -= 1;
        }

        movement = glms_vec2_normalize(movement);
        fpsCameraController_move(&cam_ctrl, movement, game_timeDelta(game));
}

static void processKeyboardEvent(void *registerArgs, void *fireArgs) {
        struct game *game = registerArgs;
        struct eventBrokerKeyboardEvent *args = fireArgs;
        
        const int key = args->key;
        const int action = args->action;
        //const int modifiers = args->modifiers;
                        
        if (action == GLFW_PRESS) {
                if (key == GLFW_KEY_ESCAPE) {
                        game_shouldStop(game);
                } else if (key == GLFW_KEY_F) {
                        cam_ctrl.freefly = !cam_ctrl.freefly;
                }
        }
}

static void updateTitle(void *registerArgs, void *fireArgs) {
        struct game *game = registerArgs;
        (void)fireArgs;
        
        static unsigned count = 0;
        if (count == FRAME_PERIOD_FPS_REFRESH) {
                const int fps = (const int)(1.0F/game_timeDelta(game));
                static char title[TITLE_BUFFER_SIZE];
                snprintf(title, TITLE_BUFFER_SIZE, "[%d] Temple (%s)", fps,
                         cam_ctrl.freefly ?
                         "Freefly camera" :
                         "FPS camera");
                game_updateWindowTitle(game, title);
                count = 0;
        } else {
                count++;
        }
}

static void freeGame(void *registerArgs, void *fireArgs) {
        struct game *game = registerArgs;
        (void)fireArgs;

        game_free(game);
        free(game);
}

int main(void) {
        struct game *game = smalloc(sizeof(struct game));
        game_init(game, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
        
        struct scene *scene = game_createScene(game);
        scene_init(scene, globalAmbientLight, 2);
        game_setCurrentScene(game, scene->idx);

        // Create sphere
        struct object *sphere = scene_createObject(scene, "sphere", 0);
        
        struct geometry *geo = componentCollection_create(COMPONENT_GEOMETRY);
        size_t geoIdx = geo->base.idx;
        geometry_initIcosphere(geo, "sphere_geo", 3);
        componentCollection_set(&sphere->components,
                                COMPONENT_GEOMETRY, geoIdx);
        
        struct material_uber *mat = componentCollection_create(
                COMPONENT_MATERIAL_UBER);
        size_t matIdx = mat->base.base.idx;
        material_uber_initDefaults(mat, "sphere_material", SHADER_UBER);
        vec4s white = GLMS_VEC4_ONE_INIT;
        mat->emissiveColor = white;
        material_setTexture((struct material*)mat,
                            MATERIAL_TEXTURE_EMISSIVE, "wood");
        componentCollection_set(&sphere->components,
                                COMPONENT_MATERIAL, matIdx);

        // Create camera
        struct object *camera = scene_createObject(scene, "camera", 0);
        struct transform *camTrans = componentCollection_get(
                &camera->components, COMPONENT_TRANSFORM);
        transform_translateZ(camTrans, STARTING_CAMERA_Z_POSITION);

        struct camera *cam = componentCollection_create(COMPONENT_CAMERA_FPS);
        size_t camIdx = cam->base.idx;
        camera_init(cam, "camera", (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT,
                    CAMERA_NEAR, CAMERA_FAR, CAMERA_FOV, true,
                    COMPONENT_CAMERA_FPS);
        componentCollection_set(&camera->components, COMPONENT_CAMERA, camIdx);

        fpsCameraController_init(&cam_ctrl, movement_speed, look_sensitivity,
                                 camera);

        // Register events
        eventBroker_register(processKeyboardInput, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_KEYBOARD_INPUT, game);
        eventBroker_register(processKeyboardEvent, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_KEYBOARD_EVENT, game);
        eventBroker_register(processMouseInput, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_MOUSE_POSITION, game);
        eventBroker_register(updateTitle, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_UPDATE, game);
        eventBroker_register(freeGame, EVENT_BROKER_PRIORITY_HIGH,
                             EVENT_BROKER_TEAR_DOWN, game);

        // Main loop
        game_run(game);
        return 0;
}
