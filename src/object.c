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

// TODO: Instead of all these arrays, create a vertex struct in the header file
// and make them pass an array of these structs.
void object_initFromArray(struct object *object,
                          const struct vertex *vertices, size_t nvertices,
                          const unsigned *indices, size_t nindices) {
        if (nindices > INT_MAX) {
                bail("Cannot draw object with more than %d "
                     "indices (attempted object has %lu indices)",
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

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                              sizeof(struct vertex),
                              (const void*)offsetof(struct vertex, norm));

        
        glGenBuffers(1, &object->ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->ibo);
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)(nindices * sizeof(*indices)),
                     indices, GL_STATIC_DRAW);
        
        glBindVertexArray(0);
        object->nindices = (int)nindices;
}

static void buildpath(const size_t destsize, char *const dest,
                      const char *const file) {
        size_t len = pathnjoin(destsize, dest, 3,
                               ASSETSPATH, "textures", file);
        if (len + 3 - 1 >= destsize) {
                die("Path to texture file too long.");
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
                buildpath(PATH_MAX, path, textures[i]);
                if (!accessible(path, true, false, false)) {
                        bail("Can't read texture file.");
                }
                
                int width, height, nrChannels;
                unsigned char *data = stbi_load(
                        path, &width, &height, &nrChannels, 0);
                if (data == NULL) {
                        bail("Can't read texture image data.");
                }

                GLenum internalFormat;
                if (nrChannels == 3) { // PNG without transparency data
                        internalFormat = GL_RGB;
                } else if (nrChannels == 4) { // PNG with transparency data
                        internalFormat =  GL_RGBA;
                } else { // Abomination
                        die("Failing to load png texture. I expected 3 or 4 "
                            "channels but this thing has %d?", nrChannels);
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
