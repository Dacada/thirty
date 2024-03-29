#include <thirty/scene.h>
#include <thirty/util.h>
#include <thirty/asyncLoader.h>

#define BOGLE_MAGIC_SIZE 5
#define OBJECT_TREE_NUMBER_BASE 10

__attribute__((access (read_write, 1)))
__attribute__((access (read_write, 2)))
__attribute__((nonnull))
static void parse_object_tree(struct scene *const scene, FILE *const f) {
        struct stack stack;
        stack_init(&stack, OBJECT_TREE_MAXIMUM_DEPTH, sizeof(size_t));
        
        size_t currentObjectIdx = 0;
        size_t lastParsedObjectIdx = 0;
        for(;;) {
                int c = fgetc(f);
                if (c == EOF) {
                        bail("Unexpected end of file or error.\n");
                } else if (isdigit(c)) {
                        size_t newObjectIdx = 0;
                        while (isdigit(c)) {
                                newObjectIdx *= OBJECT_TREE_NUMBER_BASE;
                                newObjectIdx += (unsigned int)c - '0';
                                c = fgetc(f);
                        }
                        ungetc(c, f);
                        newObjectIdx++;

                        struct object *parent = scene_getObjectFromIdx(
                                scene, currentObjectIdx);
                        struct object *child = scene_getObjectFromIdx(
                                scene, newObjectIdx);
                        object_addChild(parent, child);
                        
                        lastParsedObjectIdx = newObjectIdx;
                } else if (isspace(c)) {
                } else if (c == '{') {
                        size_t *idx = stack_push(&stack);
                        *idx = currentObjectIdx;
                        currentObjectIdx = lastParsedObjectIdx;
                } else if (c == '}') {
                        size_t *idx = stack_pop(&stack);
                        currentObjectIdx = *idx;
                } else if (c == '\0') {
                        break;
                } else {
                        bail("Unexpected character in file.\n");
                }
        }
        stack_destroy(&stack);
}

static bool loadRootObj(struct scene *const scene, void *args) {
        (void)args;
        object_initEmpty(&scene->root, scene->game, scene->idx, "root", &scene->components);
        scene->root.idx = 0;
        return true;
}

static bool createComponentCollection(struct scene *const scene, void *args) {
        (void)args;
        componentCollection_initCollection(&scene->components);
        return true;
}

static void scene_initBasic(struct scene *const scene, struct game *const game) {
        scene->loading = false;
        scene->loaded = false;
        scene->game = game;
        growingArray_init(&scene->loadSteps, sizeof(struct scene_loadStep), 4);
        growingArray_init(&scene->freePtrs, sizeof(void*), 4);
        scene_addLoadingStep(scene, createComponentCollection, NULL);
        scene_addLoadingStep(scene, loadRootObj, NULL);
}

struct bogleFileLoadArgs {
        struct {
                uint8_t magic[BOGLE_MAGIC_SIZE];
                uint8_t version;
                uint32_t ncams;
                uint32_t ngeos;
                uint32_t nmats;
                uint32_t nlights;
                uint32_t nanims;
                uint32_t nobjs;
        } header;
        FILE *f;
        size_t idxOffset;
};

static void parse_objects(struct bogleFileLoadArgs *args, struct growingArray *objects,
                          struct varSizeGrowingArray *components, struct game *game,
                          size_t scene) {
        for (unsigned i=0; i<args->header.nobjs; i++) {
                struct object *obj = growingArray_append(objects);
                obj->idx = objects->length;  // idx 0 is root
                object_initFromFile(obj, game, components, scene, args->idxOffset,
                                    args->header.ncams, args->header.ngeos,
                                    args->header.nmats, args->header.nlights,
                                    args->header.nanims, args->f);
        }
}

static bool loadBogleFileObjects(struct scene *const scene, void *const vargs) {
        struct bogleFileLoadArgs *args = vargs;

        growingArray_init(&scene->objects, sizeof(struct object), args->header.nobjs);

        // TODO: Read the chunk of the file needed all at once (async) and parse it into the objects later
        parse_objects(args, &scene->objects, &scene->components, scene->game, scene->idx);
        // TODO: Read the chunk of the file needed all at once (async) and parse it into the object tree later
        parse_object_tree(scene, args->f);

        const int c = fgetc(args->f);
        if (c != EOF) {
                bail("Malformated file, trash at the end, I'm being very strict so I won't just ignore it.\n");
        }

        fclose(args->f);
        free(args);

        return true;
}

static bool loadBogleFile(struct scene *const scene, void *const vargs) {
        char *const filename = vargs;

        struct bogleFileLoadArgs *args = smalloc(sizeof(*args));
        
        args->idxOffset = componentCollection_currentOffset(&scene->components);
        args->f = sfopen(filename, "r");

        sfread(&args->header.magic, sizeof(uint8_t), BOGLE_MAGIC_SIZE, args->f);
        if (strncmp((char*)(args->header.magic), "BOGLE", BOGLE_MAGIC_SIZE) != 0) {
                bail("Malformatted scene file\n");
        }

        sfread(&args->header.version, sizeof(args->header.version), 1, args->f);
        if (args->header.version != 0) {
                bail("Unsupported scene file version: %d "
                     "(support only 0)\n", args->header.version);
        }

        sfread(&args->header.ncams, sizeof(args->header.ncams), 1, args->f);
        sfread(&args->header.ngeos, sizeof(args->header.ngeos), 1, args->f);
        sfread(&args->header.nmats, sizeof(args->header.nmats), 1, args->f);
        sfread(&args->header.nlights, sizeof(args->header.nlights), 1, args->f);
        sfread(&args->header.nanims, sizeof(args->header.nanims), 1, args->f);
        sfread(&args->header.nobjs, sizeof(args->header.nobjs), 1, args->f);

        sfread(scene->globalAmbientLight.raw,
               sizeof(*scene->globalAmbientLight.raw),
               sizeof(scene->globalAmbientLight) /
               sizeof(*scene->globalAmbientLight.raw), args->f);
        
#define LOAD_DATA(n, which, baseType)                                   \
        for (unsigned i=0; i<(n); i++) {                                \
                uint8_t type;                                           \
                sfread(&type, sizeof(type), 1, args->f);                \
                struct which *comp = componentCollection_create(&scene->components, scene->game, (baseType) + type); \
                which##_initFromFile(comp, args->f, (baseType) + type, &scene->components); \
        }

        LOAD_DATA(args->header.ncams, camera, COMPONENT_CAMERA);
        LOAD_DATA(args->header.ngeos, geometry, COMPONENT_GEOMETRY);
        LOAD_DATA(args->header.nmats, material, COMPONENT_MATERIAL);
        LOAD_DATA(args->header.nlights, light, COMPONENT_LIGHT);
        LOAD_DATA(args->header.nanims, animationCollection, COMPONENT_ANIMATIONCOLLECTION);
#undef LOAD_DATA

        scene_addLoadingStep(scene, loadBogleFileObjects, args);
        
        return true;
}

struct loadBasicArgs {
        vec4s globalAmbientLight;
        size_t initialObjectCapacity;
};

static bool loadBasic(struct scene *const scene, void *vargs) {
        struct loadBasicArgs *args = vargs;
        
        growingArray_init(&scene->objects, sizeof(struct object),
                          args->initialObjectCapacity);

        scene->globalAmbientLight = args->globalAmbientLight;
        return true;
}

void scene_init(struct scene *const scene, struct game *const game,
                const vec4s globalAmbientLight,
                const size_t initialObjectCapacity) {
        scene_initBasic(scene, game);

        struct loadBasicArgs *args = smalloc(sizeof(*args));
        args->globalAmbientLight = globalAmbientLight;
        args->initialObjectCapacity = initialObjectCapacity;
        
        scene_addLoadingStep(scene, loadBasic, args);
} 

void scene_initFromFile(struct scene *const scene,
                        struct game *const game,
                        const char *filename) {
        scene_initBasic(scene, game);
        scene_addLoadingStep(scene, loadBogleFile, sstrdup(filename));
}

static void prepareLoadingProcess(struct scene *const scene) {
        growingArray_init(&scene->loadingStack, sizeof(struct scene_loadStep), scene->loadSteps.length);
        asyncLoader_init();

        struct scene_loadStep *steps = smallocarray(scene->loadSteps.length, sizeof(struct scene_loadStep));
        long i=0;
        growingArray_foreach_START(&scene->loadSteps, struct scene_loadStep*, step) {
                steps[i++] = *step;
        } growingArray_foreach_END;
        
        for (long j=i-1; j>=0; j--) {
                struct scene_loadStep *step = growingArray_append(&scene->loadingStack);
                *step = steps[j];
        }
        free(steps);
        
        scene->loading = true;
        scene->loaded = false;
        scene->totalSizeFinishedAsyncLoad = 0;
}

bool scene_load(struct scene *const scene) {
        if (scene->loaded) {
                return true;
        }
        if (!scene->loading) {
                prepareLoadingProcess(scene);
        }

        while (scene->loadingStack.length > 0) {
                struct scene_loadStep step = *(struct scene_loadStep*)growingArray_peek(&scene->loadingStack);
                growingArray_pop(&scene->loadingStack);
                if (!step.cb(scene, step.args)) {
                        break;
                }
        }

        if (scene->loadingStack.length > 0) {
                return false;
        }
        
        growingArray_destroy(&scene->loadingStack);
        return true;
}

static void addMustFreePtr(struct scene *scene, void *ptr) {
        void **pptr = growingArray_append(&scene->freePtrs);
        *pptr = ptr;
}

void scene_unload(struct scene *const scene) {
        object_free(&scene->root);
        growingArray_foreach_START(&scene->objects, struct object *, object)
                object_free(object);
        growingArray_foreach_END;
        growingArray_destroy(&scene->objects);
        componentCollection_freeCollection(&scene->components);
        scene->loading = false;
        scene->loaded = false;
}

bool scene_awaitAsyncLoaders(struct scene *const scene) {
        size_t size;
        bool done = !asyncLoader_await(&size);
        scene->totalSizeFinishedAsyncLoad += size;

        if (size != 0) {
                struct eventBrokerSceneLoadProgress args = {
                        .current = scene->totalSizeFinishedAsyncLoad,
                        .total = asyncLoader_totalSize(),
                };
                eventBroker_fire(EVENT_BROKER_SCENE_LOAD_PROGRESS, &args);
        }

        if (done) {
                asyncLoader_destroy();
                scene->loading = false;
                scene->loaded = true;
                return true;
        }

        return false;
}

void scene_addLoadingStep(struct scene *const scene, const scene_loadCallback cb, void *const args) {
        struct scene_loadStep *step;
        if (scene->loading) {
                step = growingArray_append(&scene->loadingStack);
        } else {
                step = growingArray_append(&scene->loadSteps);
        }
        
        step->cb = cb;
        step->args = args;
        
        if (!scene->loading && args != NULL) {
                addMustFreePtr(scene, args);
        }
}


struct object *scene_createObject(struct scene *scene,
                                  const char *const name,
                                  const size_t parent_idx) {
        struct object *const child = growingArray_append(&scene->objects);
        size_t child_idx = scene->objects.length;  // idx 0 is root
        object_initEmpty(child, scene->game, scene->idx, name, &scene->components);
        child->idx = child_idx;
        struct object *const parent = scene_getObjectFromIdx(
                scene, parent_idx);
        object_addChild(parent, child);
        return child;
}

void scene_removeObject(struct scene *const scene,
                        struct object *const object) {
        assert(object->idx > 0);
        assert(object->scene == scene->idx);
        struct object *const parent = scene_getObjectFromIdx(scene, object->parent);
        object_removeChild(parent, object);
        growingArray_foreach_START(&object->children, struct object *, child) {
                object_addChild(parent, child);
        } growingArray_foreach_END;

        object_free(object);
        growingArray_remove(&scene->objects, object->idx-1);
}

mat4s scene_getObjectAbsoluteTransform(struct scene *scene,
                                       const struct object *object) {
        struct transform *trans = object_getComponent(object, COMPONENT_TRANSFORM);
        if (trans == NULL) {
                return GLMS_MAT4_IDENTITY;
        }

        mat4s model = trans->model;
        while (object->idx != 0) {
                const struct object *parent = scene_getObjectFromIdx(scene, object->parent);
                struct transform *parentTrans = object_getComponent(parent, COMPONENT_TRANSFORM);
                if (parentTrans == NULL) {
                        return GLMS_MAT4_IDENTITY;
                }
                
                mat4s parentModel = parentTrans->model;
                model = glms_mat4_mul(parentModel, model);
                object = parent;
        }
        
        return model;
}

size_t scene_idxByName(const struct scene *scene, const char *name) {
        growingArray_foreach_START(&scene->objects, struct object*, obj)
                if (strcmp(name, obj->name) == 0) {
                        return growingArray_foreach_idx+1;
                }
        growingArray_foreach_END;
        return 0;
}

struct object *scene_getObjectFromIdx(struct scene *const scene,
                                      const size_t object_idx) {
        if (object_idx == 0) {
                return &scene->root;
        }
        return growingArray_get(&scene->objects, object_idx-1);
}

const struct object *scene_getObjectFromIdxConst(
        const struct scene *const scene,
        const size_t object_idx) {
        
        if (object_idx == 0) {
                return &scene->root;
        }
        return growingArray_get(&scene->objects, object_idx-1);
}

size_t scene_setSkybox(struct scene *const scene, const char *const basename) {
        struct object *const skybox = scene_createObject(
                scene, basename, 0);
        
        struct geometry *geo = componentCollection_create(
                &scene->components, scene->game, COMPONENT_GEOMETRY);
        geometry_initSkyboxCube(geo, basename);
        object_setComponent(skybox, &geo->base);

        struct material_skybox *mat = componentCollection_create(
                &scene->components, scene->game, COMPONENT_MATERIAL_SKYBOX);
        material_skybox_initFromName(mat, basename, &scene->components);
        object_setComponent(skybox, &mat->base.base);

        return skybox->idx;
}

void scene_free(struct scene *const scene) {
        growingArray_foreach_START(&scene->freePtrs, void**, ptr)
                free(*ptr);
        growingArray_foreach_END;
        growingArray_destroy(&scene->loadSteps);
        growingArray_destroy(&scene->freePtrs);
}


/// Rendering code ///

// Initial counts for some lists
#define STARTING_OBJECT_COUNT 16
#define STARTING_LIGHT_COUNT 8
#define STARTING_SHADER_COUNT 1

// This struct is used to hold a reference to an object along with its global
// model matrix after taking into account its parents, and its distance to the
// camera. The model matrix is passed to the shader for rendering, while the
// distance to the camera is used for sorting the objects by distance to camera
// before rendering.
struct objectModelAndDistance {
        const struct object *object;
        mat4s model;
        float distanceToCamera;
};

// Used to determine shader order and for checking equality between shaders.
__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull (1, 2)))
static int cmpshdr(const void *const item1,
                   const void *const item2,
                   void *const args) {
        const enum shaders *const shader1 = item1;
        const enum shaders *const shader2 = item2;
        (void)args;

        return (int)*shader1 - (int)*shader2;
}

__attribute__((access (read_only, 1)))
__attribute__((access (read_write, 2)))
__attribute__((access (read_only, 3)))
__attribute__((access (write_only, 5)))
__attribute__((access (write_only, 6)))
__attribute__((access (read_write, 7)))
__attribute__((access (read_write, 8)))
__attribute__((nonnull))
static void gatherObjectTree(const struct scene *const scene,
                             struct growingArray *const objects,
                             const struct object *const object,
                             const mat4s parentModel,
                             size_t *const cameraIdx,
                             size_t *const skyboxIdx,
                             struct growingArray *const lightIdxs,
                             struct growingArray *const shaders) {

        // Apply model matrix
        const struct transform *const trans = object_getComponent(
                object, COMPONENT_TRANSFORM);
        const mat4s model = glms_mat4_mul(parentModel, trans->model);

        // Add to objects array
        struct objectModelAndDistance *const objmod =
                growingArray_append(objects);
        objmod->object = object;
        objmod->model = model;
        
        // Detect camera
        const struct camera *cameraComp = object_getComponent(
                object, COMPONENT_CAMERA);
        if (cameraComp != NULL && cameraComp->main) {
                *cameraIdx = objects->length - 1;
        }

        // Detect skybox
        const struct component *materialComp = object_getComponent(
                object, COMPONENT_MATERIAL);
        if (materialComp != NULL &&
            materialComp->type == COMPONENT_MATERIAL_SKYBOX) {
                *skyboxIdx = objects->length - 1;
        }

        // Detect light
        if (componentCollection_hasComponent(
                    &object->components, COMPONENT_LIGHT)) {
                size_t *const lightIdx = growingArray_append(lightIdxs);
                *lightIdx = objects->length - 1;
        }

        // Detect shader
        if (materialComp != NULL) {
                const enum shaders shader = ((const struct material*)
                                             materialComp)->shader;
                if (!growingArray_contains(shaders, cmpshdr, &shader)) {
                        enum shaders *shdrptr = growingArray_append(shaders);
                        *shdrptr = shader;
                }
        }

        // Recurse
        growingArray_foreach_START(&object->children, size_t*, child_idx)
                const struct object *child = scene_getObjectFromIdxConst(
                        scene, *child_idx);
                gatherObjectTree(scene, objects, child, model,
                                 cameraIdx, skyboxIdx, lightIdxs, shaders);
        growingArray_foreach_END;
}

// Compare function used to sort objects for rendering. We want objects with
// the same material to be grouped together, order objects by distance to the
// camera, etc.
static int cmpobj(const void *const item1,
                  const void *const item2,
                  void *const args) {
        (void)args;
        
        const struct objectModelAndDistance *const obj1 = item1;
        const struct objectModelAndDistance *const obj2 = item2;

        const struct geometry *geo1 = object_getComponent(
                obj1->object, COMPONENT_GEOMETRY);
        const struct geometry *geo2 = object_getComponent(
                obj2->object, COMPONENT_GEOMETRY);
        
        const bool hasgeo1 = geo1 != NULL;
        const bool hasgeo2 = geo2 != NULL;
        
        // No need to do anything for objects without geometry which won't be
        // rendered. Just put them all at the end so we can finish early when
        // we find the first one.
        if (!hasgeo1 && !hasgeo2) {
                return 0;
        }
        if (!hasgeo1 && hasgeo2) {
                return 1;
        }
        if (hasgeo1 && !hasgeo2) {
                return -1;
        }

        const struct material *mat1 = object_getComponent(
                obj1->object, COMPONENT_MATERIAL);
        const struct material *mat2 = object_getComponent(
                obj2->object, COMPONENT_MATERIAL);

        const bool isSkybox1 = mat1->base.type == COMPONENT_MATERIAL_SKYBOX;
        const bool isSkybox2 = mat2->base.type == COMPONENT_MATERIAL_SKYBOX;

        // The skybox object needs to be rendered last.
        if (isSkybox1 && !isSkybox2) {
                return 1;
        }
        if (!isSkybox1 && isSkybox2) {
                return -1;
        }

        const bool trans1 = material_isTransparent(mat1);
        const bool trans2 = material_isTransparent(mat2);

        // Non transparent objects first
        if (!trans1 && trans2) {
                return -1;
        }
        if (trans1 && !trans2) {
                return 1;
        }

        const enum shaders shader1 = mat1->shader;
        const enum shaders shader2 = mat2->shader;

        // Group by shader
        const int c = cmpshdr(&shader1, &shader2, NULL);
        if (c != 0) {
                return c;
        }

        // Group by material
        if (mat1 < mat2) {
                return -1;
        }
        if (mat1 > mat2) {
                return 1;
        }

        const float dist1 = obj1->distanceToCamera;
        const float dist2 = obj2->distanceToCamera;

        // Objects closer to the camera first
        if (dist1 < dist2) {
                return -1;
        }
        if (dist1 > dist2) {
                return 1;
        }

        return 0;
}

void scene_update(struct scene *const scene, const float timeDelta) {
        struct stack stack;
        stack_init(&stack, scene->objects.length+1, sizeof(size_t));
        size_t *ptr = stack_push(&stack);
        *ptr = 0;

        while (!stack_empty(&stack)) {
                size_t *objIdx = stack_pop(&stack);
                struct object *obj = scene_getObjectFromIdx(scene, *objIdx);
                object_update(obj, timeDelta);

                growingArray_foreach_START(&obj->children, size_t *, chldIdx)
                        ptr = stack_push(&stack);
                        *ptr = *chldIdx;
                growingArray_foreach_END;
        }
        stack_destroy(&stack);
}

void scene_draw(const struct scene *const scene) {
        // Prepare data structures to hold a list of objects, of lights and of
        // shaders. We will need them later. Keep them prepared so that on each
        // draw we use the same list and we don't have to reinitialize it
        // again.
        static bool first = true;
        static struct growingArray objects;
        static struct growingArray lightIdxs;
        static struct growingArray shaders;
        if (first) {
                growingArray_init(&objects,
                                  sizeof(struct objectModelAndDistance),
                                  STARTING_OBJECT_COUNT);
                growingArray_init(&lightIdxs,
                                  sizeof(size_t),
                                  STARTING_LIGHT_COUNT);
                growingArray_init(&shaders,
                                  sizeof(enum shaders),
                                  STARTING_SHADER_COUNT);
                first = false;
        }

        // We also need to identify the camera and the skybox.
        size_t cameraIdx = 0;
        size_t skyboxIdx = 0;

        // Populate 'objects' array, calculate model matrices, gather shaders,
        // lights, main camera and skybox.
        gatherObjectTree(scene, &objects, &scene->root, GLMS_MAT4_IDENTITY,
                         &cameraIdx, &skyboxIdx, &lightIdxs, &shaders);

        struct objectModelAndDistance *camera = growingArray_get(
                &objects, cameraIdx);
        struct objectModelAndDistance *skybox = growingArray_get(
                &objects, skyboxIdx);

        // Calculate the distance to the main camera for each object
        const vec4s cameraPosition = camera->model.col[3];
        growingArray_foreach_START(&objects, struct objectModelAndDistance*,
                                   objMod)
                const vec4s objectPosition = objMod->model.col[3];
                objMod->distanceToCamera = glms_vec4_distance(
                        cameraPosition, objectPosition);
        growingArray_foreach_END;

        // Get view and projection matrices from main camera
        const struct camera *const cameraComp =
                object_getComponent(camera->object, COMPONENT_CAMERA);
        assert(cameraComp != NULL);
        assert(cameraComp->main);
        const mat4s view = camera_viewMatrix(cameraComp, camera->model);
        const mat4s projection = camera_projectionMatrix(cameraComp);

        // Update lighting for all shaders
        growingArray_foreach_START(&shaders, enum shaders*, shader)
                shader_use(*shader);
                for (size_t i=0; i<lightIdxs.length; i++) {
                        const size_t *const objModIdx = growingArray_get(
                                &lightIdxs, i);
                        const struct objectModelAndDistance *const objMod =
                                growingArray_get(&objects, *objModIdx);
                        const struct light *const light =
                                object_getComponent(
                                        objMod->object, COMPONENT_LIGHT);
                        assert(light != NULL);
                        light_updateShader(light, i, view,
                                           objMod->model, *shader);
                }
                light_updateShaderDisabled(lightIdxs.length, *shader);
                light_updateGlobalAmbient(*shader, scene->globalAmbientLight);
        growingArray_foreach_END;

        // No environment mapping yet, just the skybox, so make sure the
        // texture for the environment slot is loaded since some objects might
        // use it.
        const struct material *skyboxMaterial = object_getComponent(
                skybox->object, COMPONENT_MATERIAL);
        if (skyboxMaterial != NULL) {
                // Will be NULL when there's no skybox object. In that case, no
                // skybox material, no environment texture, hope there's no
                // object who wants to use it.
                material_bindTextures(skyboxMaterial);
        }

        // Sort objects by render order
        growingArray_sort(&objects, cmpobj, NULL);

        // Render everything. We keep a state of the rendering process with
        // renderStage, material and shader. They are changed automatically by
        // the call, thanks to the fact that our objects are sorted. So, for
        // example, when it first encounters a transparent object it will
        // switch the render stage from opaque to transparent.
        enum renderStage renderStage = RENDER_OPAQUE_OBJECTS;
        const struct material *material = NULL;  // last material used
        enum shaders shader = SHADER_TOTAL;  // last shader used
        growingArray_foreach_START(&objects, struct objectModelAndDistance*,
                                   objMod)
                if (!object_draw(objMod->object, objMod->model,
                                 view, projection,
                                 &renderStage, &material, &shader)) {
                        break;
                }
        growingArray_foreach_END;

        // Cleanup
        glDisable(GL_BLEND);
        glDepthFunc(GL_LESS);
        growingArray_clear(&objects);
        growingArray_clear(&lightIdxs);
        growingArray_clear(&shaders);
}
