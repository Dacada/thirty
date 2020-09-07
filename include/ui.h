#ifndef UI_H
#define UI_H

#include <geometry.h>
#include <dsutils.h>
#include <cglm/struct.h>

struct ui {
        int width;
        int height;
        mat4s ortho;
        struct geometry quadGeo;
        struct growingArray quads;
        struct growingArray texts;
};

void ui_startup(void);

struct font *ui_getFont(const char *face, unsigned height,
                        const char *encoding)
        __attribute__((access (read_only, 1)))
        __attribute__((access (read_only, 3)))
        __attribute__((nonnull));

void ui_init(struct ui *ui, int width, int height)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

void ui_resize(struct ui *ui, int width, int height)
        __attribute__((access (write_only, 1)))
        __attribute__((nonnull));

void ui_addQuad(struct ui *ui, int tlx, int tly, int brx, int bry, float z,
                const char *texture)
        __attribute__((access (read_write, 1)))
        __attribute__((access (read_only, 7)))
        __attribute__((nonnull));

void ui_addText(struct ui *ui, int posx, int posy, float z,
                const unsigned char *text, const struct font *font, vec4s color)
        __attribute__((access (read_write, 1)))
        __attribute__((access (read_only, 5)))
        __attribute__((access (read_only, 6)))
        __attribute__((nonnull));

void ui_draw(const struct ui *ui)
        __attribute__((access (read_only, 1)))
        __attribute__((nonnull));

void ui_free(struct ui *ui)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void ui_shutdown(void);

#endif
