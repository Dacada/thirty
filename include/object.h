#ifndef OBJECT_H
#define OBJECT_H

#include <camera.h>
#include <glad/glad.h>

struct vertex {
        vec3s vert;
        vec2s tex;
        vec3s norm;
};

struct object {
        GLuint vao, vbo, ibo;
        int nindices;
        GLuint *textures;
        unsigned ntextures;
        mat4s model;
};

void object_initModule(void);

void object_initFromArray(struct object *object,
                          const struct vertex *vertices, size_t nvertices,
                          const unsigned *indices, size_t nindices);

void object_setTextures(struct object *object,
                        const char *const textures[], unsigned ntextures);

void object_translate(struct object *object, vec3s position);
void object_rotate(struct object *object, float angle, vec3s axis);
void object_scale(struct object *object, vec3s scale);

void object_draw(const struct object *object,
                 const struct camera *camera,
                 unsigned int shader);

void object_tearDown(struct object *object);

#endif /* OBJECT_H */
