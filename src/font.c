#include <thirty/font.h>
#include <thirty/util.h>

#define UIFONT_TEXTURE_SLOT GL_TEXTURE1

void font_init(struct font *const font, const char *const name,
               const unsigned height, const char *const encoding,
               const char *const ftdNameWithExt,
               const char *const pngNameNoExt) {
        font->name = sstrdup(name);
        font->height = height;
        font->encoding = sstrdup(encoding);

        char path[PATH_MAX];
        pathjoin(PATH_MAX, path, 3, ASSETSPATH, "fonts", ftdNameWithExt);
        path[strlen(path)-1] = '\0';
        FILE *f = sfopen(path, "r");
        sfread(&font->linespacing, sizeof(font->linespacing), 1, f);
        sfread(font->characters, sizeof(*font->characters), NCHARS_IN_FONT, f);
        fclose(f);

        texture_init(&font->texture, pngNameNoExt,
                     UIFONT_TEXTURE_SLOT, GL_TEXTURE_2D);
}

void font_load(struct font *font) {
        texture_load(&font->texture);
}

void font_free(struct font *font) {
        free(font->name);
        free(font->encoding);
        texture_free(&font->texture);
}
