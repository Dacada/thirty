#include <geometry.h>
#include <camera.h>
#include <shader.h>
#include <util.h>
#include <stb_image.h>
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
        
        glGenBuffers(1, &geometry->ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->ibo);
        
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)(nindices * sizeof(*indices)),
                     indices, GL_STATIC_DRAW);
        
        glBindVertexArray(0);
        geometry->nindices = (const int)nindices;
}

__attribute__((access (write_only, 2, 1)))
__attribute__((access (read_only, 3)))
__attribute__((nonnull))
static void buildpathTex(const size_t destsize, char *const dest,
                         const char *const file) {
        const size_t len = pathjoin(destsize, dest, 3, ASSETSPATH,
                                     "textures", file);
        if (len + 3 - 1 >= destsize) {
                die("Path to texture file too long.\n");
        }
        strcpy(dest+len-2, ".png");
}

void geometry_setTextures(struct geometry *const geometry,
                          const char *const textures[],
                          const unsigned ntextures) {
        stbi_set_flip_vertically_on_load(true);
        
        geometry->ntextures = ntextures;
        geometry->textures = smallocarray((const size_t)geometry->ntextures,
                                          sizeof(*geometry->textures));
        glGenTextures((const int)ntextures, geometry->textures);

        for (unsigned i=0; i<ntextures; i++) {
                glActiveTexture(GL_TEXTURE0+i);
                glBindTexture(GL_TEXTURE_2D, geometry->textures[i]);
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                GL_LINEAR);

                static char path[PATH_MAX];
                buildpathTex(PATH_MAX, path, textures[i]);
                if (!accessible(path, true, false, false)) {
                        bail("Can't read texture file.\n");
                }
                
                int width;
                int height;
                int nrChannels;
                unsigned char *const data __attribute__ ((nonstring)) =
                        stbi_load(path, &width, &height, &nrChannels, 0);
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

void geometry_draw(const struct geometry *const geometry,
                   const mat4s model,
                   const struct camera *const camera,
                   const unsigned int shader) {
        shader_use(shader);
        for (unsigned i=0; i<geometry->ntextures; i++) {
                glActiveTexture(GL_TEXTURE0+i);
                glBindTexture(GL_TEXTURE_2D, geometry->textures[i]);
        }

        const mat4s projection = camera_projectionMatrix(camera);
        const mat4s view = camera_viewMatrix(camera);
        shader_setMat4(shader, "view", view);
        shader_setMat4(shader, "projection", projection);
        shader_setMat4(shader, "model", model);

        glBindVertexArray(geometry->vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->ibo);
        glDrawElements(GL_TRIANGLES, geometry->nindices, GL_UNSIGNED_INT, 0);
}

void geometry_free(const struct geometry *const geometry) {
        if (geometry->textures != NULL) {
                glDeleteTextures((const int)geometry->ntextures,
                                 geometry->textures);
        }
        
        glDeleteBuffers(1, &geometry->vbo);
        glDeleteBuffers(1, &geometry->ibo);
        glDeleteVertexArrays(1, &geometry->vao);
        free(geometry->textures);
}
