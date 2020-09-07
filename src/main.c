#include <inputHelpers.h>
#include <game.h>
#include <physicalWorld.h>
#include <font.h>
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
                } else if (key == GLFW_KEY_SPACE) {
                        size_t idx = componentCollection_idxByName(
                                "physF", COMPONENT_PHYSICALENTITY);
                        assert(idx > 0);
                        idx -= 1;
                        struct physicalEntity *phys =
                                componentCollection_compByIdx(idx);
                        phys->collider.type = COLLIDER_NONE;
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
                snprintf(title, TITLE_BUFFER_SIZE, "[%d] Physics (%s)", fps,
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
        static const vec4s black = GLMS_VEC4_BLACK_INIT;
        static const vec4s white = GLMS_VEC4_ONE_INIT;
        static const vec4s red = {.x = 1, .y = 0, .z = 0, .w = 1};
        static const vec4s green = {.x = 0, .y = 1, .z = 0, .w = 1};
        static const vec4s blue = {.x = 0, .y = 0, .z = 1, .w = 1};
        
        struct game *game = smalloc(sizeof(struct game));
        game_init(game, SCREEN_WIDTH, SCREEN_HEIGHT, 1);

        struct physicalWorld *world = physicalWorld_create();
        physicalWorld_init(world);
        
        struct scene *scene = game_createScene(game);
        scene_init(scene, game, globalAmbientLight, 4);
        game_setCurrentScene(game, scene->idx);

        // Create spheres
        struct object *sphere1 = scene_createObject(scene, "sphere1", 0);
        struct transform *sphere1Trans = object_getComponent(
                sphere1, COMPONENT_TRANSFORM);
        transform_translate(sphere1Trans, (vec3s){.x=0.7F, .y=20, .z=-50});
        transform_scale(sphere1Trans, (vec3s){.x=2, .y=2, .z=2});
        struct object *sphere2 = scene_createObject(scene, "sphere2", 0);
        struct transform *sphere2Trans = object_getComponent(
                sphere2, COMPONENT_TRANSFORM);
        transform_translate(sphere2Trans, (vec3s){.x=-.05F, .y=10, .z=-50});
        
        struct geometry *geo = componentCollection_create(
                game, COMPONENT_GEOMETRY);
        geometry_initIcosphere(geo, "sphere_geo", 3);
        object_setComponent(sphere1, &geo->base);
        object_setComponent(sphere2, &geo->base);
        
        struct material_uber *mat1 = componentCollection_create(
                game, COMPONENT_MATERIAL_UBER);
        material_uber_initDefaults(mat1, "sphere1_material", SHADER_UBER);
        mat1->emissiveColor = red;
        material_setTexture((struct material*)mat1,
                            MATERIAL_TEXTURE_EMISSIVE, "wood");
        object_setComponent(sphere1, &mat1->base.base);
        
        struct material_uber *mat2 = componentCollection_create(
                game, COMPONENT_MATERIAL_UBER);
        material_uber_initDefaults(mat2, "sphere2_material", SHADER_UBER);
        mat2->emissiveColor = blue;
        material_setTexture((struct material*)mat2,
                            MATERIAL_TEXTURE_EMISSIVE, "wood");
        object_setComponent(sphere2, &mat2->base.base);

        struct physicalEntity *phys1 = componentCollection_create(
                game, COMPONENT_PHYSICALENTITY);
        physicalEntity_init(phys1, 2, "phys1");
        physicalWorld_addEntity(world, phys1->base.idx);
        object_setComponent(sphere1, &phys1->base);
        physicalEntity_setColliderSphere(phys1, 2);

        struct physicalEntity *phys2 = componentCollection_create(
                game, COMPONENT_PHYSICALENTITY);
        physicalEntity_init(phys2, 1, "phys2");
        physicalWorld_addEntity(world, phys2->base.idx);
        object_setComponent(sphere2, &phys2->base);
        physicalEntity_setColliderSphere(phys2, 1);

        // Create cubes
        struct object *cube = scene_createObject(scene, "cube", 0);
        struct object *cube2 = scene_createObject(scene, "cube2", 0);
        struct transform *cubeTrans = object_getComponent(
                cube, COMPONENT_TRANSFORM);
        transform_translate(cubeTrans, (vec3s){.x=0, .y=5, .z=-50});
        struct geometry *cubeGeo = componentCollection_create(
                game, COMPONENT_GEOMETRY);
        geometry_initCube(cubeGeo, "cube_geo");
        object_setComponent(cube, &cubeGeo->base);
        object_setComponent(cube2, &cubeGeo->base);
        struct material_uber *cubeMat = componentCollection_create(
                game, COMPONENT_MATERIAL_UBER);
        material_uber_initDefaults(cubeMat, "cube_mat", SHADER_UBER);
        cubeMat->emissiveColor = green;
        material_setTexture(&cubeMat->base,
                            MATERIAL_TEXTURE_EMISSIVE, "wood");
        object_setComponent(cube, &cubeMat->base.base);
        struct physicalEntity *physCube = componentCollection_create(
                game, COMPONENT_PHYSICALENTITY);
        physicalEntity_init(physCube, 1.5, "cube_phys");
        physicalWorld_addEntity(world, physCube->base.idx);
        object_setComponent(cube, &physCube->base);
        physicalEntity_setColliderAABB(physCube, (vec3s){.x=1, .y=1, .z=1});

        struct transform *cube2Trans = object_getComponent(
                cube2, COMPONENT_TRANSFORM);
        transform_translate(cube2Trans, (vec3s){.x=0, .y=15, .z=-50});
        transform_scale(cube2Trans, (vec3s){.x=2, .y=2, .z=2});
        struct material_uber *cube2Mat = componentCollection_create(
                game, COMPONENT_MATERIAL_UBER);
        material_uber_initDefaults(cube2Mat, "cube2_mat", SHADER_UBER);
        cube2Mat->emissiveColor = red;
        material_setTexture(&cube2Mat->base,
                            MATERIAL_TEXTURE_EMISSIVE, "wood");
        object_setComponent(cube2, &cube2Mat->base.base);
        struct physicalEntity *physCube2 = componentCollection_create(
                game, COMPONENT_PHYSICALENTITY);
        physicalEntity_init(physCube2, 3, "cube_phys");
        physicalWorld_addEntity(world, physCube2->base.idx);
        object_setComponent(cube2, &physCube2->base);
        physicalEntity_setColliderAABB(physCube2, (vec3s){.x=2, .y=2, .z=2});

        // Create floor
        struct object *floor = scene_createObject(scene, "floor", 0);
        struct transform *floorTransform = object_getComponent(
                floor, COMPONENT_TRANSFORM);
        transform_translate(floorTransform, (vec3s){.x=0, .y=-15, .z=-50});
        transform_rotateX(floorTransform, glm_rad(-90));
        transform_scale(floorTransform, (vec3s){.x=10, .y=10, .z=10});
        struct geometry *floorGeo = componentCollection_create(
                game, COMPONENT_GEOMETRY);
        geometry_initPlane(floorGeo, "floor_geo");
        object_setComponent(floor, &floorGeo->base);
        struct material_uber *floorMat = componentCollection_create(
                game, COMPONENT_MATERIAL_UBER);
        material_uber_initDefaults(floorMat, "floor_mat", SHADER_UBER);
        floorMat->emissiveColor = white;
        material_setTexture(&floorMat->base,
                            MATERIAL_TEXTURE_EMISSIVE, "wood");
        object_setComponent(floor, &floorMat->base.base);
        struct physicalEntity *physF = componentCollection_create(
                game, COMPONENT_PHYSICALENTITY);
        physicalEntity_init(physF, 10000, "physF");
        physF->kinetic = true;
        physicalWorld_addEntity(world, physF->base.idx);
        object_setComponent(floor, &physF->base);
        physicalEntity_setColliderPlane(physF,
                                        (vec3s){.x=0, .y=1, .z=0}, -15);

        // Create camera
        struct object *camera = scene_createObject(scene, "camera", 0);

        struct camera *cam = componentCollection_create(
                game, COMPONENT_CAMERA_FPS);
        camera_init(cam, "camera", (float)SCREEN_WIDTH/(float)SCREEN_HEIGHT,
                    CAMERA_NEAR, CAMERA_FAR, CAMERA_FOV, true,
                    COMPONENT_CAMERA_FPS);
        object_setComponent(camera, &cam->base);

        fpsCameraController_init(&cam_ctrl, movement_speed, look_sensitivity,
                                 camera);

        // Create UI
        ui_addQuad(game->ui, 10,10, 200,60, 0.0F, "ui_test_texture");
        struct font *font = ui_getFont("CutiveMono-Regular", 24, "latin-1");
        font_load(font);
        ui_addText(game->ui, 35,45, 0.1F, (const unsigned char*)"Something.",
                   font, black);

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
