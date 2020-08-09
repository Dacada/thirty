#include <scene.h>
#include <object.h>
#include <componentCollection.h>
#include <geometry.h>
#include <camera.h>
#include <material.h>
#include <light.h>
#include <dsutils.h>
#include <util.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <cglm/struct.h>

#define BOGLE_MAGIC_SIZE 5
#define OBJECT_TREE_NUMBER_BASE 10

__attribute__((access (write_only, 2, 1)))
__attribute__((access (read_only, 3)))
__attribute__((nonnull))
static void buildpathObj(const size_t destsize, char *const dest,
                         const char *const file) {
        const size_t len = pathjoin(destsize, dest, 3, ASSETSPATH,
                                     "scenes", file);
        if (len + 3 - 1 >= destsize) {
                die("Path to scene file too long.\n");
        }
        strcpy(dest+len-2, ".bgl");
}

__attribute__((access (write_only, 1, 2)))
__attribute__((access (read_only, 3)))
__attribute__((access (read_write, 8)))
__attribute__((nonnull))
static void parse_objects(struct growingArray *objects, unsigned nobjs,
                          struct scene *scene,
                          unsigned ncams, unsigned ngeos, unsigned nmats,
                          unsigned nlights, FILE *const f) {
        for (unsigned i=0; i<nobjs; i++) {
                struct object *obj = growingArray_append(objects);
                obj->idx = objects->length;  // idx 0 is root
                object_initFromFile(obj, scene,
                                    ncams, ngeos, nmats, nlights, f);
        }
}

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

void scene_initFromFile(struct scene *const scene,
                        const char *const filename) {
        char path[PATH_MAX];
        buildpathObj(PATH_MAX, path, filename);
        if (!accessible(path, true, false, false)) {
                bail("Cannot read scene file.\n");
        }

        FILE *const f = sfopen(path, "rb");
        struct {
                uint8_t magic[BOGLE_MAGIC_SIZE];
                uint8_t version;
                uint32_t ncams;
                uint32_t ngeos;
                uint32_t nmats;
                uint32_t nlights;
                uint32_t nobjs;
        } header;

        sfread(&header.magic, sizeof(uint8_t), BOGLE_MAGIC_SIZE, f);
        if (strncmp((char*)(header.magic), "BOGLE", BOGLE_MAGIC_SIZE) != 0) {
                bail("Malformatted scene file: %s\n", path);
        }

        sfread(&header.version, sizeof(header.version), 1, f);
        if (header.version != 0) {
                bail("Unsupported scene file version: %d "
                     "(support only 0)\n", header.version);
        }

        sfread(&header.ncams, sizeof(header.ncams), 1, f);
        sfread(&header.ngeos, sizeof(header.ngeos), 1, f);
        sfread(&header.nmats, sizeof(header.nmats), 1, f);
        sfread(&header.nlights, sizeof(header.nlights), 1, f);
        sfread(&header.nobjs, sizeof(header.nobjs), 1, f);

        sfread(scene->globalAmbientLight.raw,
               sizeof(*scene->globalAmbientLight.raw),
               sizeof(scene->globalAmbientLight) /
               sizeof(*scene->globalAmbientLight.raw), f);

#define LOAD_DATA(n, which, baseType)                                   \
        do {                                                            \
                for (unsigned i=0; i<(n); i++) {                        \
                        uint8_t type;                                   \
                        sfread(&type, sizeof(type), 1, f);              \
                        struct which *comp =                            \
                                componentCollection_create(             \
                                        (baseType) + type);             \
                        which##_initFromFile(comp, f, (baseType) + type); \
                }                                                       \
        } while (0)

        LOAD_DATA(header.ncams, camera, COMPONENT_CAMERA);
        LOAD_DATA(header.ngeos, geometry, COMPONENT_GEOMETRY);
        LOAD_DATA(header.nmats, material, COMPONENT_MATERIAL);
        LOAD_DATA(header.nlights, light, COMPONENT_LIGHT);
#undef LOAD_DATA

        growingArray_init(&scene->objects,
                          sizeof(struct object), header.nobjs);

        object_initEmpty(&scene->root, scene, "root");
        scene->root.idx = 0;
        parse_objects(&scene->objects, header.nobjs, scene,
                      header.ncams, header.ngeos, header.nmats,
                      header.nlights, f);
        parse_object_tree(scene, f);

        const int c = fgetc(f);
        if (c != EOF) {
                bail("Malformated file, trash at the end, I'm being "
                     "very strict so I won't just ignore it.\n");
        }

        fclose(f);
}

struct object *scene_createObject(struct scene *scene,
                                  const char *const name,
                                  const size_t parent_idx) {
        struct object *const child = growingArray_append(&scene->objects);
        size_t child_idx = scene->objects.length;  // idx 0 is root
        object_initEmpty(child, scene, name);
        child->idx = child_idx;
        struct object *const parent = scene_getObjectFromIdx(
                scene, parent_idx);
        object_addChild(parent, child);
        return child;
}

struct object *scene_getObjectFromIdx(struct scene *const scene,
                                      const size_t object_idx) {
        if (object_idx == 0) {
                return &scene->root;
        }
        return growingArray_get(&scene->objects, object_idx-1);
}

size_t scene_setSkybox(struct scene *const scene,
                       const char *const basename) {
        struct geometry *geo = componentCollection_create(
                COMPONENT_GEOMETRY);
        geometry_initSkybox(geo, basename);
        size_t geoIdx = geo->base.idx;

        struct material_skybox *mat = componentCollection_create(
                COMPONENT_MATERIAL_SKYBOX);
        material_skybox_initFromName(mat, basename);
        size_t matIdx = mat->base.base.idx;

        struct object *const skybox = scene_createObject(
                scene, basename, 0);
        object_setSkybox(skybox, geoIdx, matIdx);

        return skybox->idx;
}

void scene_free(struct scene *const scene) {
        object_free(&scene->root);
        growingArray_foreach_START(&scene->objects, struct object *, object)
                object_free(object);
        growingArray_foreach_END;
        growingArray_destroy(&scene->objects);
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

__attribute__((access (read_write, 1)))
__attribute__((access (read_only, 2)))
__attribute__((access (write_only, 4)))
__attribute__((access (write_only, 5)))
__attribute__((access (read_write, 6)))
__attribute__((access (read_write, 7)))
__attribute__((nonnull))
static void gatherObjectTree(
        struct growingArray *const objects, const struct object *const object,
        const mat4s parentModel,
        size_t *const cameraIdx,
        size_t *const skyboxIdx,
        struct growingArray *const lightIdxs,
        struct growingArray *const shaders) {

        // Apply model matrix
        const mat4s model = glms_mat4_mul(parentModel, object->model);

        // Add to objects array
        struct objectModelAndDistance *const objmod =
                growingArray_append(objects);
        objmod->object = object;
        objmod->model = model;
        
        // Detect camera
        const struct camera *cameraComp = (struct camera*)
                componentCollection_get(&object->components, COMPONENT_CAMERA);
        if (cameraComp != NULL && cameraComp->main) {
                *cameraIdx = objects->length - 1;
        }

        // Detect skybox
        const struct component *materialComp = componentCollection_get(
                &object->components, COMPONENT_MATERIAL);
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
                struct object *child = scene_getObjectFromIdx(
                        object->scene, *child_idx);
                gatherObjectTree(objects, child, model,
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

        const struct componentCollection *comps1 = &obj1->object->components;
        const struct componentCollection *comps2 = &obj2->object->components;

        const struct geometry *geo1 = (struct geometry*)
                componentCollection_get(comps1, COMPONENT_GEOMETRY);
        const struct geometry *geo2 = (struct geometry*)
                componentCollection_get(comps2, COMPONENT_GEOMETRY);
        
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

        const struct material *mat1 = (struct material*)
                componentCollection_get(comps1, COMPONENT_MATERIAL);
        const struct material *mat2 = (struct material*)
                componentCollection_get(comps2, COMPONENT_MATERIAL);

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
        gatherObjectTree(&objects, &scene->root, GLMS_MAT4_IDENTITY,
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
        const struct camera *const cameraComp = (struct camera*)
                componentCollection_get(&camera->object->components,
                                        COMPONENT_CAMERA);
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
                        const struct light *const light = (struct light*)
                                componentCollection_get(
                                        &objMod->object->components,
                                        COMPONENT_LIGHT);
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
        const struct material *skyboxMaterial = (struct material*)
                componentCollection_get(&skybox->object->components,
                                        COMPONENT_MATERIAL);
        material_bindTextures(skyboxMaterial);

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
