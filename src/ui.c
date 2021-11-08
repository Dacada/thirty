#include <thirty/ui.h>
#include <thirty/util.h>

#define UIQUAD_TEXTURE_SLOT GL_TEXTURE0

static struct font *fonts;
static size_t nfonts;

struct uiQuad {
        mat4s model;
        struct texture texture;
};

struct uiText {
        int posx;
        int posy;
        const struct font *font;
        vec4s color;
        size_t ncharacters;
        struct uiChar *characters;
};

struct uiChar {
        mat4s model;
        unsigned char endpoint;
        vec2s uvMul;
        vec2s uvAdd;
};

__attribute__((access (read_write, 1)))
__attribute__((nonnull))
static void recalculateOrthoMatrix(struct ui *const ui) {
        ui->ortho = glms_ortho(0.0F, (float)ui->width,
                               (float)ui->height, 0.0F,
                               -1.0F, 1.0F);
}

void ui_startup(void) {
        struct growingArray fontsSeen;
        growingArray_init(&fontsSeen, sizeof(struct font), 1);
        
        char fontspath[PATH_MAX];
        pathjoin(PATH_MAX, fontspath, 2, ASSETSPATH, "fonts");
        
        DIR *dir = sopendir(fontspath);
        struct dirent *ent;
        while ((ent = sreaddir(dir)) != NULL) {
                const char *const name = ent->d_name;
                const char *const extension = strrchr(name, '.')+1;
                if (strcmp(extension, "ftd") == 0) {
                        char *str = sstrdup(name);
                        char *strOriginal = str;
                        const char *const fontName = str;
                        str = strchr(str, '_');
                        *str = '\0';
                        str++;
                        const unsigned height = (unsigned)atoi(str);
                        str = strchr(str, '_');
                        *str = '\0';
                        str++;
                        const char *const encoding = str;
                        str = strchr(str, '.');
                        *str = '\0';

                        char *const ftdName = sstrdup(name);
                        
                        char *const pngName = sstrdup(name);
                        *strrchr(pngName, '.') = '\0';

                        struct font *font = growingArray_append(&fontsSeen);
                        font_init(font,
                                  fontName, height, encoding,
                                  ftdName, pngName);

                        free(ftdName);
                        free(pngName);
                        free(strOriginal);
                }
        }
        closedir(dir);

        fonts = fontsSeen.data;
        nfonts = fontsSeen.length;
}

struct font *ui_getFont(const char *const face, const unsigned height,
                        const char *const encoding) {
        for (size_t i=0; i<nfonts; i++) {
                struct font *font = fonts + i;
                if (font->height == height &&
                    strcmp(font->name, face) == 0 &&
                    strcmp(font->encoding, encoding) == 0) {
                        return font;
                }
        }
        return NULL;
}

void ui_init(struct ui *const ui, const int width, const int height) {
        ui_resize(ui, width, height);
        ui->quadGeo.base.type = COMPONENT_GEOMETRY;
        geometry_initPlane(&ui->quadGeo, "uiQuad");
        growingArray_init(&ui->quads, sizeof(struct uiQuad), 2);
        growingArray_init(&ui->texts, sizeof(struct uiText), 2);
}

void ui_resize(struct ui *const ui, const int width, const int height) {
        ui->width = width;
        ui->height = height;
        recalculateOrthoMatrix(ui);
}

void ui_addQuad(struct ui *const ui,
                const int tlx, const int tly,
                const int brx, const int bry,
                const float z,
                const char *const texture) {
        struct uiQuad *quad = growingArray_append(&ui->quads);
        
        quad->model = GLMS_MAT4_IDENTITY;

        vec3s scale;
        scale.x = (float)(brx - tlx)/2;
        scale.y = (float)(bry - tly)/2;
        scale.z = 1.0F;
        quad->model = glms_scale(quad->model, scale);
        
        quad->model.col[3].x = (float)tlx + (float)brx/2;
        quad->model.col[3].y = (float)tly + (float)bry/2;
        quad->model.col[3].z = z;
        
        texture_init(&quad->texture, texture,
                     UIQUAD_TEXTURE_SLOT, GL_TEXTURE_2D);
        texture_load(&quad->texture);
}

void ui_addText(struct ui *const ui,
                const int posx, const int posy, const float z,
                const unsigned char *const text, const struct font *const font,
                const vec4s color) {
        struct uiText *uiText = growingArray_append(&ui->texts);
        uiText->posx = posx;
        uiText->posy = posy;
        uiText->font = font;
        uiText->color = color;

        uiText->ncharacters = strlenu(text);
        uiText->characters = smallocarray(
                uiText->ncharacters, sizeof(struct uiChar));

        int currPosX = posx;
        int currPosY = posy;
        for (size_t i=0;; i++) {
                unsigned char c = text[i];
                if (c == '\0') {
                        break;
                }
                
                struct uiChar *character = uiText->characters + i;
                character->model = GLMS_MAT4_IDENTITY;
                character->endpoint = c;

                if (c == '\n') {
                        currPosX = posx;
                        currPosY += (int)font->linespacing;
                        continue;
                }

                float invTextureDimension = 1.0F / (float)font->texture.width;
                const unsigned width = font->characters[c].width;
                const unsigned height = font->characters[c].height;

                vec2s uvMul = {
                        .x = (float)width,
                        .y = (float)height,
                };
                character->uvMul = glms_vec2_scale(uvMul, invTextureDimension);

                vec2s uvAdd = {
                        .x = (float)font->characters[c].positionX,
                        .y = (float)(font->characters[c].positionY + height),
                };
                character->uvAdd = glms_vec2_scale(uvAdd, invTextureDimension);
                character->uvAdd.y = 1 - character->uvAdd.y;

                const int drawPosX = currPosX + font->characters[c].bearingX;
                const int drawPosY = currPosY - font->characters[c].bearingY;

                // Divided by two because the original quad is 2x2
                vec3s scale = {
                        .x = (float)width/2,
                        .y = (float)height/2,
                        .z = 1
                };
                character->model = glms_scale(character->model, scale);

                // Add half width/height because the original quad's "anchor"
                // is at the middle not at the top left like we'd expect.
                character->model.col[3].x = (float)drawPosX + (float)width/2;
                character->model.col[3].y = (float)drawPosY + (float)height/2;
                character->model.col[3].z = z;

                currPosX += (int)font->characters[c].advanceX;
                currPosY += (int)font->characters[c].advanceY;
        }
}

void ui_draw(const struct ui *ui) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        shader_use(SHADER_UI);

        shader_setBool(SHADER_UI, "hasColor", false);
        shader_setBool(SHADER_UI, "hasMask", false);
        shader_setVec2(SHADER_UI, "uvMul", (vec2s){.x=1, .y=1});
        shader_setVec2(SHADER_UI, "uvAdd", (vec2s){.x=0, .y=0});
        growingArray_foreach_START(&ui->quads, struct uiQuad*, quad)
                mat4s modelOrtho = glms_mat4_mul(ui->ortho, quad->model);
                shader_setMat4(SHADER_UI, "modelOrtho", modelOrtho);
                texture_bind(&quad->texture);
                geometry_draw(&ui->quadGeo);
        growingArray_foreach_END;

        shader_setBool(SHADER_UI, "hasColor", true);
        shader_setBool(SHADER_UI, "hasMask", true);
        growingArray_foreach_START(&ui->texts, struct uiText*, text)
                texture_bind(&text->font->texture);
                shader_setVec4(SHADER_UI, "color", text->color);
                for (size_t i=0; i<text->ncharacters; i++) {
                        struct uiChar *character = text->characters + i;
                        if (character->endpoint == '\n') {
                                continue;
                        }
                        
                        mat4s modelOrtho = glms_mat4_mul(
                                ui->ortho, character->model);
                        shader_setMat4(SHADER_UI, "modelOrtho", modelOrtho);

                        shader_setVec2(SHADER_UI, "uvMul", character->uvMul);
                        shader_setVec2(SHADER_UI, "uvAdd", character->uvAdd);
                        
                        geometry_draw(&ui->quadGeo);
                }
        growingArray_foreach_END;
}

void ui_update(struct ui *ui, float timeDelta) {
        (void)ui;
        (void)timeDelta;
        // TODO: Make each UI element its own thing and have an update method
        // and call update in the same way that scene does so UI objects can do
        // stuff.
}

void ui_free(struct ui *ui) {
        growingArray_foreach_START(&ui->quads, struct uiQuad*, quad)
                texture_free(&quad->texture);
        growingArray_foreach_END;
        
        growingArray_foreach_START(&ui->texts, struct uiText*, text)
                free(text->characters);
        growingArray_foreach_END;
        
        growingArray_destroy(&ui->quads);
        growingArray_destroy(&ui->texts);
        
        geometry_free(&ui->quadGeo);
}

void ui_shutdown(void) {
        for (size_t i=0; i<nfonts; i++) {
                font_free(fonts+i);
        }
        free(fonts);
}
