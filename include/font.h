#ifndef FONT_H
#define FONT_H

#include <texture.h>

#define NCHARS_IN_FONT 256

struct font_char {
        unsigned positionX;
        unsigned positionY;
        unsigned width;
        unsigned height;
        int bearingX;
        int bearingY;
        unsigned advanceX;
        unsigned advanceY;
};

struct font {
        char *name;
        unsigned height;
        char *encoding;

        unsigned linespacing;
        struct font_char characters[NCHARS_IN_FONT];

        struct texture texture;
};

void font_init(struct font *font,
               const char *name, unsigned height, const char *encoding,
               const char *ftdNameWithExt, const char *pngNameNoExt)
        __attribute__((access (write_only, 1)))
        __attribute__((access (read_only, 2)))
        __attribute__((access (read_only, 4)))
        __attribute__((access (read_only, 5)))
        __attribute__((access (read_only, 6)))
        __attribute__((nonnull));

void font_load(struct font *font)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

void font_free(struct font *font)
        __attribute__((access (read_write, 1)))
        __attribute__((nonnull));

#endif
