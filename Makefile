MAKEFLAGS := --warn-undefined-variables

SRC_DIR := src
BIN_DIR := bin
OBJ_DIR := obj
INCLUDE_DIR := include
ASSETS_DIR := assets

SOURCES := $(wildcard $(SRC_DIR)/*.c)

# Filter out generated glad files which need special rules
SOURCES := $(filter-out $(SRC_DIR)/glad_rel.c,$(SOURCES))
SOURCES := $(filter-out $(SRC_DIR)/glad_dbg.c,$(SOURCES))

# Sources to analyze for tidying up (only file names)
TIDY_SOURCES := $(filter-out $(SRC_DIR)/stb_image.c $(SRC_DIR)/main.c,$(SOURCES))
TIDY_SOURCES := $(patsubst $(SRC_DIR)/%,%,$(TIDY_SOURCES))

# Header files to analyze for tidying up (only file names)
TIDY_INCLUDES := $(wildcard $(INCLUDE_DIR)/thirty/*.h)
TIDY_INCLUDES := $(filter-out $(INCLUDE_DIR)/stb_image.h,$(TIDY_INCLUDES))
TIDY_INCLUDES := $(patsubst $(INCLUDE_DIR)/thirty/%,%,$(TIDY_INCLUDES))

TIDY_CHECKS := "-checks=-*,bugprone-*,linuxkernel-*,misc-*,modernize-*,performance-*,portability-*,readability-*"

OBJECTS_DEBUG := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%_dbg.o,$(SOURCES))
OBJECTS_RELEASE := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%_rel.o,$(SOURCES))
OBJECTS_DEVELOP := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%_dev.o,$(SOURCES))

# Add object files for the generated glad files
OBJECTS_DEBUG += $(OBJ_DIR)/glad_dbg.o $(OBJ_DIR)/stb_image_dbg.o
OBJECTS_RELEASE += $(OBJ_DIR)/glad_rel.o $(OBJ_DIR)/stb_image_rel.o
OBJECTS_DEVELOP += $(OBJ_DIR)/glad_dev.o $(OBJ_DIR)/stb_image_dev.o

DEPENDS_DEBUG := $(OBJECTS_DEBUG:.o=.d)
DEPENDS_RELEASE := $(OBJECTS_RELEASE:.o=.d)
DEPENDS_DEVELOP := $(OBJECTS_DEVELOP:.o=.d)

TARGETS := $(BIN_DIR)/main_dbg $(BIN_DIR)/main_rel $(BIN_DIR)/main_dev

FONTS_FTD := $(ASSETS_DIR)/fonts/Cabin-Regular_36_latin-1.ftd
FONTS_FTD += $(ASSETS_DIR)/fonts/CutiveMono-Regular_24_latin-1.ftd
FONTS_PNG := $(patsubst $(ASSETS_DIR)/fonts/%.ftd,$(ASSETS_DIR)/textures/%.png,$(FONTS_FTD))


CC := gcc

# Common flags
CFLAGS := -DASSETSPATH=\"$(realpath $(ASSETS_DIR))\" -I$(realpath $(INCLUDE_DIR)) `pkg-config --cflags glfw3` `pkg-config --cflags cglm` -Wall -Wextra -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wmissing-prototypes -Wwrite-strings -Wcast-qual -Wswitch-default -Wswitch-enum -Wconversion -Wunreachable-code -Wimplicit-fallthrough -Wstringop-overflow=4 -std=c11
LDFLAGS := `pkg-config --libs glfw3` `pkg-config --libs cglm` -lm -ldl -std=c11

# Flags for generating glad files (also common)
GLAD_FLAGS := --profile=core --api=gl=3.3 --spec=gl --extensions= --out-path=$$tmpdir


CFLAGS_DEBUG := -MMD -Og -g
LDFLAGS_DEBUG := $(CFLAGS_DEBUG)
CFLAGS_RELEASE := -MMD -DNDEBUG -flto -O2 -g
LDFLAGS_RELEASE := $(CFLAGS_RELEASE)
LDFLAGS_DEVELOP := -fsanitize=address -fsanitize=undefined -Og
CFLAGS_DEVELOP := -MMD $(LDFLAGS_DEVELOP) -Werror

GLAD_FLAGS_DEBUG := --generator=c-debug
GLAD_FLAGS_RELEASE := --generator=c


define FIND_HEADERS_CMD
( \
    find . '(' -type d -name .git ')' -prune -o \
           '(' -type d -name venv ')' -prune -o \
           -type f '(' -name '*.c' -o -name '*.h' ')' -print; \
)
endef

define FIND_SYSHEADERS_CMD
( \
    find /usr/include -type f -name '*.h' -print; \
)
endef

.PHONY: dbg dev rel clearfonts clean veryclean purify impolute etags glad_rel glad_dbg fonts valgrind static-analysis tidy_src tidy_include line-count

rel: fonts glad_rel $(BIN_DIR)/main
dev: fonts glad_dbg etags $(BIN_DIR)/main_dev
dbg: fonts glad_dbg $(BIN_DIR)/main_dbg

clearfonts:
	-rm -f $(FONTS_PNG)
	-rm -rf $(ASSETS_DIR)/fonts
clean: clearfonts
	-rm -f $(OBJ_DIR)/*.o
	-rm -rf $(BIN_DIR)
veryclean: clean
	-rm -rf $(OBJ_DIR)
	-rm -f TAGS
purify: clearfonts veryclean
	-rm -f $(SRC_DIR)/glad_dbg.c $(SRC_DIR)/glad_rel.c
	-rm -f $(INCLUDE_DIR)/glad/glad_dbg.h $(INCLUDE_DIR)/glad/glad_rel.h
	-rm -rf $(INCLUDE_DIR)/KHR/
	-rm -f $(INCLUDE_DIR)/stb_image.h
	-rm -f sysh_TAGS
impolute: purify
	-rm -rf venv
	-rm -rf BitmapFontGenerator/venv

glad_rel: $(INCLUDE_DIR)/KHR/khrplatform.h $(INCLUDE_DIR)/glad/glad_rel.h $(SRC_DIR)/glad_rel.c

glad_dbg: $(INCLUDE_DIR)/glad/glad_dbg.h $(SRC_DIR)/glad_dbg.c

etags: sysh_TAGS
	-rm -f TAGS
	$(FIND_HEADERS_CMD) | etags --include=$< -

fonts: $(FONTS_FTD) $(FONTS_PNG)

valgrind: $(BIN_DIR)/main_dbg
	valgrind --leak-check=full           			\
                 --show-leak-kinds=definite,indirect,possible	\
	         --track-origins=yes         			\
	         --verbose                   			\
	         --log-file=valgrind-out.txt 			\
	         $(BIN_DIR)/main_dbg

static-analysis: clean
	scan-build -disable-checker cplusplus.InnerPointer \
		   -disable-checker cplusplus.Move \
		   -disable-checker cplusplus.NewDelete \
		   -disable-checker cplusplus.NewDeleteLeaks \
		   -disable-checker cplusplus.PureVirtualCall \
		   -enable-checker nullability.NullableDereferenced \
		   -enable-checker nullability.NullablePassedToNonnull \
		   -enable-checker nullability.NullableReturnedFromNonnull \
		   -enable-checker optin.performance.Padding \
		   -enable-checker optin.portability.UnixAPI \
		   -enable-checker security.FloatLoopCounter \
		   -enable-checker valist.CopyToSelf \
		   -enable-checker valist.Uninitialized \
		   -enable-checker valist.Unterminated \
		   make dbg

tidy_src: $(SRC_DIR)/.clang_complete
	cd $(SRC_DIR) && clang-tidy $(TIDY_SOURCES) $(TIDY_CHECKS) -- $$(<.clang_complete)
tidy_include: $(INCLUDE_DIR)/.clang_complete
	cd $(INCLUDE_DIR) && clang-tidy $(TIDY_INCLUDES) $(TIDY_CHECKS) -- $$(<.clang_complete)

line-count:
	wc -l $(addprefix $(SRC_DIR)/,$(TIDY_SOURCES)) $(addprefix $(INCLUDE_DIR)/thirty/,$(TIDY_INCLUDES)) BOGLE/exporter_BOGLE.py BitmapFontGenerator/BitmapFontGenerator.py

sysh_TAGS:
	$(FIND_SYSHEADERS_CMD) | etags -o $@ -


$(BIN_DIR)/main_dbg: LDFLAGS += $(LDFLAGS_DEBUG)
$(BIN_DIR)/main_rel: LDFLAGS += $(LDFLAGS_RELEASE)
$(BIN_DIR)/main_dev: LDFLAGS += $(LDFLAGS_DEVELOP)

$(BIN_DIR)/main_dbg: $(OBJECTS_DEBUG)
$(BIN_DIR)/main_rel: $(OBJECTS_RELEASE)
$(BIN_DIR)/main_dev: $(OBJECTS_DEVELOP)

$(OBJ_DIR)/%_dbg.o: CFLAGS += $(CFLAGS_DEBUG)
$(OBJ_DIR)/%_rel.o: CFLAGS += $(CFLAGS_RELEASE)
$(OBJ_DIR)/%_dev.o: CFLAGS += $(CFLAGS_DEVELOP)

# The debug version of glad.c has a lot of functions missing
# prototypes for some reason
$(OBJ_DIR)/glad_dbg.o: CFLAGS += -Wno-missing-prototypes
$(OBJ_DIR)/glad_dev.o: CFLAGS += -Wno-missing-prototypes

# Remove some spurious warnings from stb_image
$(OBJ_DIR)/stb_image_%.o: CFLAGS += -Wno-cast-qual
$(OBJ_DIR)/stb_image_%.o: CFLAGS += -Wno-sign-conversion
$(OBJ_DIR)/stb_image_%.o: CFLAGS += -Wno-conversion
$(OBJ_DIR)/stb_image_%.o: CFLAGS += -Wno-switch-default
$(OBJ_DIR)/stb_image_%.o: CFLAGS += -Wno-missing-prototypes

# Specific dependencies for glad generated files
$(OBJ_DIR)/glad_rel.o: $(SRC_DIR)/glad_rel.c
$(OBJ_DIR)/glad_dbg.o: $(SRC_DIR)/glad_dbg.c
$(OBJ_DIR)/glad_dev.o: $(SRC_DIR)/glad_dbg.c
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<

# Can't avoid recipie repetition in pattern rules, see
# https://stackoverflow.com/questions/11441084/makefile-with-multiples-rules-sharing-same-recipe#comment38627982_11441134
$(OBJ_DIR)/%_dbg.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/stb_image.h
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<
$(OBJ_DIR)/%_rel.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/stb_image.h
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<
$(OBJ_DIR)/%_dev.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/stb_image.h
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/stb_image.h
	@mkdir -p $(OBJ_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<


$(TARGETS):
	mkdir -p $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ -o $@

$(BIN_DIR)/main: $(BIN_DIR)/main_rel
	cp $< $@


$(INCLUDE_DIR)/KHR/khrplatform.h $(INCLUDE_DIR)/glad/glad_rel.h $(SRC_DIR)/glad_rel.c &: venv
	mkdir -p $(INCLUDE_DIR)/glad
	mkdir -p $(INCLUDE_DIR)/KHR
	set -e; \
	source venv/bin/activate; \
	tmpdir=`mktemp -d`; \
	glad $(GLAD_FLAGS) $(GLAD_FLAGS_RELEASE); \
	cp $$tmpdir/include/glad/glad.h $(INCLUDE_DIR)/glad/glad_rel.h; \
	cp $$tmpdir/src/glad.c $(SRC_DIR)/glad_rel.c; \
	cp $$tmpdir/include/KHR/khrplatform.h $(INCLUDE_DIR)/KHR; \
	rm -r $$tmpdir

$(INCLUDE_DIR)/glad/glad_dbg.h $(SRC_DIR)/glad_dbg.c &: venv
	mkdir -p $(INCLUDE_DIR)/glad
	set -e; \
	source venv/bin/activate; \
	tmpdir=`mktemp -d`; \
	glad $(GLAD_FLAGS) $(GLAD_FLAGS_DEBUG); \
	cp $$tmpdir/include/glad/glad.h $(INCLUDE_DIR)/glad/glad_dbg.h; \
	sed -e '1i#ifndef __clang_analyzer__' -e '$$a#endif' $$tmpdir/src/glad.c > $(SRC_DIR)/glad_dbg.c; \
	rm -r $$tmpdir

venv: requirements.txt
	virtualenv venv
	source venv/bin/activate; pip install -r requirements.txt


$(INCLUDE_DIR)/stb_image.h:
	wget -q -O $@ "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"


$(SRC_DIR)/.clang_complete $(INCLUDE_DIR)/.clang_complete: Makefile
	echo $(CFLAGS) | tr " " "\n" > $@


$(ASSETS_DIR)/fonts/Cabin-Regular.ttf:
	mkdir -p $(ASSETS_DIR)/fonts
	wget -q -O $@ "https://raw.githubusercontent.com/impallari/Cabin/master/fonts/TTF/Cabin-Bold.ttf"
$(ASSETS_DIR)/fonts/CutiveMono-Regular.ttf:
	mkdir -p $(ASSETS_DIR)/fonts
	wget -q -O $@ "https://raw.githubusercontent.com/vernnobile/CutiveFont/master/CutiveMono/GWF-1.001/CutiveMono-Regular.ttf"

$(ASSETS_DIR)/textures/Cabin-Regular_36_latin-1.png $(ASSETS_DIR)/fonts/Cabin-Regular_36_latin-1.ftd &: $(ASSETS_DIR)/fonts/Cabin-Regular.ttf BitmapFontGenerator/venv
	set -e; \
	source BitmapFontGenerator/venv/bin/activate; \
	python BitmapFontGenerator/BitmapFontGenerator.py --basedir $(ASSETS_DIR) --height 36 --encoding latin-1 $<
$(ASSETS_DIR)/textures/CutiveMono-Regular_24_latin-1.png $(ASSETS_DIR)/fonts/CutiveMono-Regular_24_latin-1.ftd &: $(ASSETS_DIR)/fonts/CutiveMono-Regular.ttf BitmapFontGenerator/venv
	set -e; \
	source BitmapFontGenerator/venv/bin/activate; \
	python BitmapFontGenerator/BitmapFontGenerator.py --basedir $(ASSETS_DIR) --height 24 --encoding latin-1 $<

BitmapFontGenerator/venv: BitmapFontGenerator/requirements.txt
	virtualenv BitmapFontGenerator/venv
	source BitmapFontGenerator/venv/bin/activate; \
	pip install -r BitmapFontGenerator/requirements.txt


-include $(DEPENDS_DEBUG)
-include $(DEPENDS_RELEASE)
-include $(DEPENDS_DEVELOP)
