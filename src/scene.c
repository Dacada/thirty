#include <scene.h>
#include <object.h>
#include <material.h>
#include <light.h>
#include <camera.h>
#include <util.h>
#include <dsutils.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

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
                die("Path to geometry file too long.\n");
        }
        strcpy(dest+len-2, ".bgl");
}

__attribute__((access (write_only, 1, 2)))
__attribute__((access (read_only, 3)))
__attribute__((access (read_only, 4)))
__attribute__((access (read_only, 5)))
__attribute__((access (read_only, 6)))
__attribute__((access (read_write, 7)))
__attribute__((nonnull))
static void parse_objects(struct object *const objects, unsigned nobjs,
                          struct geometry *const geometries,
                          struct material *const materials,
                          struct light *const lights,
                          struct camera *const camera,
                          FILE *const f) {
        for (unsigned i=0; i<nobjs; i++) {
                object_initFromFile(objects+i,
                                    geometries, materials, lights, camera,
                                    f);
        }
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

void scene_initFromFile(struct scene *const scene,
                        const char *const filename,
                        const struct sceneInitParams *const params) {
        char path[PATH_MAX];
        buildpathObj(PATH_MAX, path, filename);
        if (!accessible(path, true, false, false)) {
                bail("Cannot read scene file.\n");
        }

        FILE *const f = sfopen(path, "rb");
        struct {
                uint8_t magic[BOGLE_MAGIC_SIZE];
                uint8_t version;
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

        sfread(&header.ngeos, sizeof(header.ngeos), 1, f);
        sfread(&header.nmats, sizeof(header.nmats), 1, f);
        sfread(&header.nlights, sizeof(header.nlights), 1, f);
        sfread(&header.nobjs, sizeof(header.nobjs), 1, f);

        struct camera *camera_data = camera_new(params->cameraType);
        camera_initFromFile(camera_data, f, params->cameraType);
        
        sfread(scene->globalAmbientLight.raw,
               sizeof(*(scene->globalAmbientLight.raw)), 4, f);
        
#define PARSE(what, amount, f)                                          \
        struct what *what##_data = smallocarray(amount,                 \
                                                sizeof(*what##_data));  \
        for (unsigned i=0; i<(amount); i++) {                           \
                what##_initFromFile(what##_data + i, f);                \
        }

        PARSE(geometry, header.ngeos, f);
        PARSE(material, header.nmats, f);
        PARSE(light, header.nlights, f);
#undef PARSE
        
        struct object *object_data = smallocarray(header.nobjs,
                                                  sizeof(*object_data));
        
        parse_objects(object_data, header.nobjs,
                      geometry_data, material_data, light_data, camera_data,
                      f);
        scene->root.camera = NULL;
        scene->root.geometry = NULL;
        scene->root.material = NULL;
        scene->root.light = NULL;
        scene->root.model = glms_mat4_identity();
        
        parse_object_tree(&scene->root, object_data, header.nobjs, f);
        scene->root.parent = NULL;

        scene->nlights = header.nlights;
        scene->lights = light_data;
        
        scene->nmats = header.nmats;
        scene->mats = material_data;
        
        scene->nobjs = header.nobjs;
        scene->objs = object_data;

        scene->ngeos = header.ngeos;
        scene->geos = geometry_data;

        scene->cam = camera_data;

        const int c = fgetc(f);
        if (c != EOF) {
                bail("Malformated file, trash at the end, I'm being "
                     "very strict so I won't just ignore it.\n");
        }

        fclose(f);
}

void scene_draw(const struct scene *const scene) {
        object_draw(&scene->root, scene->globalAmbientLight);
}

void scene_free(const struct scene *const scene) {
        free(scene->root.children);
        for (unsigned i=0; i<scene->nobjs; i++) {
                free(scene->objs[i].children);
                object_free(scene->objs + i);
        }
        for (unsigned i=0; i<scene->ngeos; i++) {
                geometry_free(scene->geos + i);
        }
        free(scene->geos);
        free(scene->objs);
        free(scene->mats);
        free(scene->lights);
        free(scene->cam);
}
