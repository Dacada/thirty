#include <scene.h>
#include <object.h>
#include <material.h>
#include <light.h>
#include <camera.h>
#include <util.h>
#include <dsutils.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#define BOGLE_MAGIC_SIZE 5
#define OBJECT_TREE_MAXIMUM_DEPTH 256
#define OBJECT_TREE_NUMBER_BASE 10

__attribute__((access (write_only, 2, 1)))
__attribute__((access (read_only, 3)))
__attribute__((nonnull))
static void buildpathObj(const size_t destsize, char *const dest,
                         const char *const file) {
        const size_t len = pathjoin(destsize, dest, 3, ASSETSPATH,
                                     "scenes", file);
        if (len + 3 - 1 >= destsize) {
                die("Path to geometry file too long.\n");
        }
        strcpy(dest+len-2, ".bgl");
}

__attribute__((access (read_write, 2)))
__attribute__((nonnull))
__attribute__((returns_nonnull))
static struct object *parse_objects(const unsigned nobjs,
                                    FILE *const f) {
        struct object *const objects = smallocarray(nobjs, sizeof(*objects));
        
        for (unsigned n=0; n<nobjs; n++) {
                object_init_fromFile(&objects[n], f);
        }

        return objects;
}

__attribute__((access (write_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull))
static bool assign_parent(void *const element,
                          void *const args) {
        struct object **child = element;
        struct object *parent = args;
        (*child)->parent = parent;
        return true;
}

__attribute__((access (write_only, 1)))
__attribute__((access (write_only, 2)))
__attribute__((access (read_write, 4)))
__attribute__((nonnull))
static void parse_object_tree(struct object *const root,
                              struct object *const objects,
                              const unsigned nobjs, FILE *const f) {
        struct growingArray *const children =
                smallocarray(nobjs+1, sizeof(*children));
        for (unsigned i=0; i<nobjs+1; i++) {
                growingArray_init(&children[i], sizeof(struct object*), 1);
        }
        
        struct stack stack;
        stack_init(&stack, OBJECT_TREE_MAXIMUM_DEPTH, sizeof(unsigned));
        
        unsigned currentObject = 0;
        unsigned newObject = 0;
        for(;;) {
                int c = fgetc(f);
                if (c == EOF) {
                        bail("Unexpected end of file or error.\n");
                } else if (isdigit(c)) {
                        newObject = 0;
                        while (isdigit(c)) {
                                newObject *= OBJECT_TREE_NUMBER_BASE;
                                newObject += (unsigned int)c - '0';
                                c = fgetc(f);
                        }
                        ungetc(c, f);
                        newObject += 1;
                        
                        const struct object **const obj =
                                growingArray_append(&children[currentObject]);
                        *obj = &objects[newObject-1];
                } else if (isspace(c)) {
                } else if (c == '{') {
                        unsigned *const num = stack_push(&stack);
                        *num = currentObject;
                        currentObject = newObject;
                } else if (c == '}') {
                        const unsigned *const num = stack_pop(&stack);
                        newObject = currentObject;
                        currentObject = *num;
                } else if (c == '\0') {
                        break;
                } else {
                        bail("Unexpected character in file.\n");
                }
        }

        stack_destroy(&stack);

        for (unsigned i=0; i<nobjs+1; i++) {
                growingArray_pack(&children[i]);

                struct object *obj;
                if (i == 0) {
                        obj = root;
                } else {
                        obj = &objects[i-1];
                }

                if (children[i].length > UINT_MAX) {
                        bail("Too many children.\n");
                }
                growingArray_foreach(&children[i], &assign_parent, obj);
                obj->children = children[i].data;
                obj->nchildren = (const unsigned int)children[i].length;
        }

        free(children);
}

__attribute__((access (read_write, 2)))
__attribute__((nonnull))
__attribute__((returns_nonnull))
static struct material *parse_materials(const size_t nmaterials,
                                 FILE *const f) {
        struct material *const materials =
                smallocarray(nmaterials, sizeof(*materials));

        for (size_t i=0; i<nmaterials; i++) {
                material_initFromFile(&materials[i], f);
                shader_use(materials[i].shader);
        }

        return materials;
}

__attribute__((access (write_only, 1, 2)))
__attribute__((access (write_only, 3)))
__attribute__((access (read_write, 4)))
__attribute__((nonnull))
static void parse_lights(struct light *const lights,
                         const unsigned nlights,
                         vec4s *const globalAmbientLight,
                         FILE *const f) {
        assert(nlights <= NUM_LIGHTS);
        
        for (unsigned n=0; n<nlights; n++) {
                light_initFromFile(&lights[n], f);
        }
        sfread(globalAmbientLight->raw, 4, 4, f);
}

__attribute__((access (read_only, 1)))
__attribute__((access (write_only, 2, 3)))
__attribute__((access (read_write, 4)))
__attribute__((nonnull))
static void assign_materials(struct material *const materials,
                             struct object *const objects,
                             const size_t nobjs,
                             FILE *const f) {
        for (size_t obj=0; obj<nobjs; obj++) {
                unsigned i;
                sfread(&i, 4, 1, f);
                objects[obj].material = &materials[i];
        }
}

__attribute__((access (read_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull))
static int shdreq(const void *const v1, const void *const v2,
                  void *const args) {
        (void)args;
        const enum shaders *const s1 = v1;
        const enum shaders *const s2 = v2;
        return (int)*s1 - (int)*s2;
}

unsigned scene_initFromFile(struct scene *const scene,
                            const float width, const float height,
                            const char *const filename) {
        char path[PATH_MAX];
        buildpathObj(PATH_MAX, path, filename);
        if (!accessible(path, true, false, false)) {
                bail("Cannot read scene file.\n");
        }

        FILE *const f = sfopen(path, "rb");
        struct {
                char magic[BOGLE_MAGIC_SIZE] __attribute__ ((nonstring));
                char version;
                unsigned nobjs;
                unsigned nlights;
                unsigned nmaterials;
        } header;

        sfread(&header.magic, 1, BOGLE_MAGIC_SIZE, f);
        if (strncmp(header.magic, "BOGLE", BOGLE_MAGIC_SIZE) != 0) {
                bail("Malformatted scene file: %s\n", path);
        }

        sfread(&header.version, 1, 1, f);
        if (header.version != 0) {
                bail("Unsupported scene file version: %d "
                     "(support only 0)\n", header.version);
        }

        sfread(&header.nobjs, 4, 1, f);
        sfread(&header.nlights, 4, 1, f);
        sfread(&header.nmaterials, 4, 1, f);

        struct object *const objects = parse_objects(header.nobjs, f);
        parse_object_tree(&scene->root, objects, header.nobjs, f);
        scene->nobjs = header.nobjs;
        scene->objs = objects;

        camera_initFromFile(&scene->camera, width, height, f);

        scene->lights = smallocarray(header.nlights, sizeof(*(scene->lights)));
        scene->nlights = header.nlights;
        parse_lights(scene->lights, header.nlights,
                     &scene->globalAmbientLight, f);
        
        struct material *materials = parse_materials(header.nmaterials, f);
        assign_materials(materials, objects, header.nobjs, f);
        scene->nmats = header.nmaterials;
        scene->mats = materials;

        struct growingArray totalShaders;
        growingArray_init(&totalShaders, sizeof(enum shaders), 2);
        for (size_t i=0; i<header.nmaterials; i++) {
                const enum shaders shader = materials[i].shader;
                if (!growingArray_contains(&totalShaders, shdreq, &shader)) {
                        enum shaders *ptr = growingArray_append(&totalShaders);
                        *ptr = shader;
                        
                        shader_use(shader);
                        light_updateGlobalAmbient(shader,
                                                  scene->globalAmbientLight);
                        light_updateShaderAll(scene->lights, scene->nlights,
                                              shader);
                }
        }
        growingArray_pack(&totalShaders);
        scene->nshaders = (unsigned)totalShaders.length;
        scene->shaders = totalShaders.data;
        
        scene->root.geometry = NULL;
        scene->root.model = glms_mat4_identity();
        strcpy(scene->root.name, "root");
        scene->root.parent = NULL;

        const int c = fgetc(f);
        if (c != EOF) {
                //ungetc(c, f);
                bail("Malformated file, trash at the end, I'm being "
                     "very strict so I won't just ignore it.\n");
        }

        fclose(f);
        return header.nobjs;
}

void scene_updateAllLighting(const struct scene *scene) {
        for (unsigned i=0; i<scene->nshaders; i++) {
                light_updateShaderAll(scene->lights, scene->nlights,
                                      scene->shaders[i]);
                light_updateGlobalAmbient(scene->shaders[i],
                                          scene->globalAmbientLight);
        }
}

void scene_draw(const struct scene *const scene) {
        object_draw(&scene->root, &scene->camera,
                    scene->nlights, scene->lights);
}

void scene_free(const struct scene *const scene) {
        free(scene->root.children);
        for (unsigned i=0; i<scene->nobjs; i++) {
                free(scene->objs[i].children);
                object_free(scene->objs + i);
        }
        free(scene->objs);
        free(scene->mats);
        free(scene->shaders);
}
