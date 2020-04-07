#include <object.h>
#include <camera.h>
#include <shader.h>
#include <util.h>
#include <stb_image.h>
#include <glad/glad.h>
#include <string.h>
#include <limits.h>

void object_initModule(void) {
        stbi_set_flip_vertically_on_load(true);
}

static void object_init(struct object *const object) {
        object->ntextures = 0;
        object->textures = NULL;
        
        object->vao = 0;
        object->vbo = 0;
        object->ibo = 0;
        
        object->model = GLMS_MAT4_IDENTITY;
}

static void buildpathObj(const size_t destsize, char *const dest,
                         const char *const file) {
        size_t len = pathnjoin(destsize, dest, 3, ASSETSPATH, "objects", file);
        if (len + 3 - 1 >= destsize) {
                die("Path to object file too long.\n");
        }
        strcpy(dest+len-2, ".bgl");
}

unsigned object_initFromFile(struct object **const objects,
                             const char *const filename, const bool expected) {
        char path[PATH_MAX];
        buildpathObj(PATH_MAX, path, filename);
        if (!accessible(path, true, false, false)) {
                bail("Cannot read object file.\n");
        }

        FILE *f = sfopen(path, "rb");
        struct {
                char magic[5];
                char version;
                unsigned nobjs;
        } header;

        sfread(&header.magic, 1, 5, f);
        if (strncmp(header.magic, "BOGLE", 5) != 0) {
                bail("Malformatted object file: %s\n", path);
        }

        sfread(&header.version, 1, 1, f);
        if (header.version != 0) {
                bail("Unsupported object file version: %d "
                     "(support only 0)\n", header.version);
        }

        sfread(&header.nobjs, 4, 1, f);
        if (!expected)
                *objects = scalloc(header.nobjs, sizeof(**objects));
        struct object *object = *objects;

        for (unsigned nobj=0; nobj<header.nobjs; nobj++) {
                struct {
                        unsigned char namelen;
                        unsigned vertlen, indlen;
                } obj_header;

                sfread(&obj_header.namelen, 1, 1, f);
                sfread(&obj_header.vertlen, 4, 1, f);
                sfread(&obj_header.indlen, 4, 1, f);

                char *name = scalloc(obj_header.namelen, sizeof(*name));
                struct vertex *vertices = scalloc(obj_header.vertlen,
                                                  sizeof(*vertices));
                unsigned *indices = scalloc(obj_header.indlen,
                                            sizeof(*indices));

                sfread(name, sizeof(*name), obj_header.namelen, f);
                sfread(vertices, sizeof(*vertices), obj_header.vertlen, f);
                sfread(indices, sizeof(*indices), obj_header.indlen, f);

                int c = fgetc(f);
                if (c != EOF) {
                        //ungetc(c, f);
                        bail("Malformated file, trash at the end, I'm being "
                             "very strict so I won't just ignore it.\n");
                }

                object_initFromArray(*objects,
                                     vertices, obj_header.vertlen,
                                     indices, obj_header.indlen);
                object++;
                free(vertices);
                free(indices);
        }

        return header.nobjs;
}

// TODO: Instead of all these arrays, create a vertex struct in the header file
// and make them pass an array of these structs.
void object_initFromArray(struct object *object,
                          const struct vertex *vertices, size_t nvertices,
                          const unsigned *indices, size_t nindices) {
        if (nindices > INT_MAX) {
                bail("Cannot draw object with more than %d "
                     "indices (attempted object has %lu indices)\n",
                     INT_MAX, nindices);
        }
        
        object_init(object);
        
        glGenVertexArrays(1, &object->vao);
        glBindVertexArray(object->vao);

        glGenBuffers(1, &object->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, object->vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     (GLsizeiptr)(nvertices * sizeof(*vertices)),
                     vertices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              sizeof(struct vertex),
                              (const void*)offsetof(struct vertex, vert));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                              sizeof(struct vertex),
                              (const void*)offsetof(struct vertex, tex));

        
        glGenBuffers(1, &object->ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->ibo);
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)(nindices * sizeof(*indices)),
                     indices, GL_STATIC_DRAW);
        
        glBindVertexArray(0);
        object->nindices = (int)nindices;
}

static void buildpathTex(const size_t destsize, char *const dest,
                      const char *const file) {
        size_t len = pathnjoin(destsize, dest, 3,
                               ASSETSPATH, "textures", file);
        if (len + 3 - 1 >= destsize) {
                die("Path to texture file too long.\n");
        }
        strcpy(dest+len-2, ".png");
}

void object_setTextures(struct object *const object,
                        const char *const textures[],
                        const unsigned ntextures) {
        object->ntextures = ntextures;
        object->textures = scalloc((size_t)object->ntextures,
                                   sizeof(*object->textures));
        glGenTextures((int)ntextures, object->textures);

        for (unsigned i=0; i<ntextures; i++) {
                glActiveTexture(GL_TEXTURE0+i);
                glBindTexture(GL_TEXTURE_2D, object->textures[i]);
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);

                char path[PATH_MAX];
                buildpathTex(PATH_MAX, path, textures[i]);
                if (!accessible(path, true, false, false)) {
                        bail("Can't read texture file.\n");
                }
                
                int width, height, nrChannels;
                unsigned char *data = stbi_load(
                        path, &width, &height, &nrChannels, 0);
                if (data == NULL) {
                        bail("Can't read texture image data.\n");
                }

                GLenum internalFormat;
                if (nrChannels == 3) { // PNG without transparency data
                        internalFormat = GL_RGB;
                } else if (nrChannels == 4) { // PNG with transparency data
                        internalFormat =  GL_RGBA;
                } else { // Abomination
                        die("Failing to load png texture. I expected 3 or 4 "
                            "channels but this thing has %d?\n", nrChannels);
                }
                
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
                             internalFormat, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
                
                stbi_image_free(data);
        }
}

void object_translate(struct object *const object, const vec3s position) {
        object->model = glms_translate(object->model, position);
}

void object_rotate(struct object *const object, const float angle,
                   const vec3s axis) {
        object->model = glms_rotate(object->model, angle, axis);
}

void object_scale(struct object *const object, const vec3s scale) {
        object->model = glms_scale(object->model, scale);
}

void object_draw(const struct object *object,
                 const struct camera *camera,
                 unsigned int shader) {
        shader_use(shader);
        for (unsigned i=0; i<object->ntextures; i++) {
                glActiveTexture(GL_TEXTURE0+i);
                glBindTexture(GL_TEXTURE_2D, object->textures[i]);
        }

        mat4s projection = camera_projectionMatrix(camera);
        mat4s view = camera_viewMatrix(camera);
        shader_setMat4(shader, "view", view);
        shader_setMat4(shader, "projection", projection);
        shader_setMat4(shader, "model", object->model);

        glBindVertexArray(object->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->ibo);
        glDrawElements(GL_TRIANGLES, object->nindices, GL_UNSIGNED_INT, 0);
}

void object_tearDown(struct object *object) {
        glDeleteTextures((int)object->ntextures, object->textures);
        glDeleteBuffers(1, &object->vbo);
        glDeleteBuffers(1, &object->ibo);
        glDeleteVertexArrays(1, &object->vao);
        free(object->textures);
}
