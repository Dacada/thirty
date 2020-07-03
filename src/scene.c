#include <scene.h>
#include <object.h>
#include <light.h>
#include <camera.h>
#include <util.h>
#include <dsutils.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>

#define BOGLE_MAGIC_SIZE 5
#define OBJECT_TREE_MAXIMUM_DEPTH 256
#define OBJECT_TREE_NUMBER_BASE 10

__attribute__((access (write_only, 2, 1)))
__attribute__((access (read_only, 3)))
__attribute__((nonnull))
static void buildpathObj(const size_t destsize, char *const restrict dest,
                         const char *const restrict file) {
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
                                    FILE *const restrict f) {
        struct object *const objects = smallocarray(nobjs, sizeof(*objects));
        
        for (unsigned n=0; n<nobjs; n++) {
                object_init_fromFile(&objects[n], f);
        }

        return objects;
}

__attribute__((access (write_only, 1)))
__attribute__((access (read_only, 2)))
__attribute__((nonnull))
static bool assign_parent(void *const restrict element,
                          void *const restrict args) {
        struct object *restrict *restrict child = element;
        struct object *restrict parent = args;
        (*child)->parent = parent;
        return true;
}

__attribute__((access (write_only, 1)))
__attribute__((access (write_only, 2)))
__attribute__((access (read_write, 4)))
__attribute__((nonnull))
static void parse_object_tree(struct object *const root,
                              struct object *const objects,
                              const unsigned nobjs, FILE *const restrict f) {
        struct growingArray *const restrict children =
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
                        
                        const struct object **const restrict obj =
                                growingArray_append(&children[currentObject]);
                        *obj = &objects[newObject-1];
                } else if (isspace(c)) {
                } else if (c == '{') {
                        unsigned *const restrict num = stack_push(&stack);
                        *num = currentObject;
                        currentObject = newObject;
                } else if (c == '}') {
                        const unsigned *const restrict num = stack_pop(&stack);
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

                struct object *restrict obj;
                if (i == 0) {
                        obj = root;
                } else {
                        obj = &objects[i-1];
                }

                if (children[i].length > UINT_MAX) {
                        bail("Too many children.\n");
                }
                growingArray_foreach(&children[i], &assign_parent, obj);

                // Assigning a restrict pointer to another restrict pointer is
                // undefined behavior. But we're not accessing them anymore
                // so...
                obj->children = children[i].data;
                obj->nchildren = (const unsigned int)children[i].length;
        }

        free(children);
}

__attribute__((access (write_only, 1, 2)))
__attribute__((access (read_write, 5)))
__attribute__((nonnull))
static void parse_cameras(struct camera *const restrict camera,
                          const unsigned ncams,
                          const float width, const float height,
                          FILE *const restrict f) {
        if (ncams != 1) {
                bail("Right now only exactly 1 camera is supported.");
        }

        vec3s position;
        float yaw;
        float pitch;

        sfread(&position, 4, 3, f);
        sfread(&yaw, 4, 1, f);
        sfread(&pitch, 4, 1, f);

        camera_init(camera, width, height, &position,
                    NULL, &yaw, &pitch, NULL, NULL);
}

__attribute__((access (write_only, 1, 2)))
__attribute__((access (read_write, 3)))
__attribute__((nonnull))
static void parse_lights(struct light *const restrict lights,
                         const unsigned nlights,
                         FILE *const restrict f) {
        // TODO: set global ambient light
        for (unsigned n=0; n<nlights; n++) {
                sfread(&lights[n].position, 4, 3, f);
                sfread(&lights[n].ambientColor, 4, 4, f);
                sfread(&lights[n].difuseColor, 4, 4, f);
                sfread(&lights[n].specularColor, 4, 4, f);
                sfread(&lights[n].ambientPower, 4, 4, f);
                sfread(&lights[n].difusePower, 4, 4, f);
                sfread(&lights[n].specularPower, 4, 4, f);
        }
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
                unsigned ncams;
                unsigned nlights;
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
        sfread(&header.ncams, 4, 1, f);
        sfread(&header.nlights, 4, 1, f);

        struct object *const restrict objects = parse_objects(header.nobjs, f);
        parse_object_tree(&scene->root, objects, header.nobjs, f);
        parse_cameras(&scene->camera, header.ncams, width, height, f);
        parse_lights(scene->lights, header.nlights, f);
        // TODO: when parsing materials, after its done, call the function to
        // update the global ambient light to the shader of each material found

        scene->nobjs = header.nobjs;
        scene->objs = objects;
        
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

void scene_draw(const struct scene *const scene) {
        object_draw(&scene->root, &scene->camera, scene->lights);
}

void scene_free(const struct scene *const scene) {
        free(scene->root.children);
        for (unsigned i=0; i<scene->nobjs; i++) {
                free(scene->objs[i].children);
                object_free(scene->objs + i);
        }
        free(scene->objs);
}
