#include <geometry.h>
#include <camera.h>
#include <shader.h>
#include <util.h>
#include <glad/glad.h>
#include <string.h>
#include <limits.h>

__attribute__((access (write_only, 1)))
__attribute__((nonnull))
static void geometry_init(struct geometry *const restrict geometry) {
        geometry->ntextures = 0;
        geometry->textures = NULL;
        
        geometry->vao = 0;
        geometry->vbo = 0;
        geometry->ibo = 0;
}

void geometry_initFromArray(struct geometry *const geometry,
                            const struct vertex *const vertices,
                            const size_t nvertices,
                            const unsigned *const indices,
                            const size_t nindices) {
        if (nindices > INT_MAX) {
                bail("Cannot draw geometry with more than %d "
                     "indices (attempted geometry has %lu indices)\n",
                     INT_MAX, nindices);
        }
        
        geometry_init(geometry);
        
        glGenVertexArrays(1, &geometry->vao);
        glBindVertexArray(geometry->vao);

        glGenBuffers(1, &geometry->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, geometry->vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     (GLsizeiptr)(nvertices * sizeof(*vertices)),
                     vertices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              sizeof(struct vertex),
                              (const void *const)
                              offsetof(struct vertex, vert));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                              sizeof(struct vertex),
                              (const void *const)
                              offsetof(struct vertex, tex));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                              sizeof(struct vertex),
                              (const void *const)
                              offsetof(struct vertex, norm));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE,
                              sizeof(struct vertex),
                              (const void *const)
                              offsetof(struct vertex, tang));

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE,
                              sizeof(struct vertex),
                              (const void *const)
                              offsetof(struct vertex, binorm));
        
        glGenBuffers(1, &geometry->ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->ibo);
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)(nindices * sizeof(*indices)),
                     indices, GL_STATIC_DRAW);
        
        glBindVertexArray(0);
        geometry->nindices = (const int)nindices;
}

void geometry_draw(const struct geometry *const geometry, const mat4s model) {
        glBindVertexArray(geometry->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->ibo);
        glDrawElements(GL_TRIANGLES, geometry->nindices, GL_UNSIGNED_INT, 0);
}

void geometry_free(const struct geometry *const geometry) {
        glDeleteBuffers(1, &geometry->vbo);
        glDeleteBuffers(1, &geometry->ibo);
        glDeleteVertexArrays(1, &geometry->vao);
        free(geometry->textures);
}
