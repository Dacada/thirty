MAKEFLAGS := --warn-undefined-variables

SRC_DIR := src
BIN_DIR := bin
OBJ_DIR := obj
INCLUDE_DIR := include

SOURCES := $(wildcard $(SRC_DIR)/*.c)

# Filter out generated glad files which need special rules
SOURCES := $(filter-out $(SRC_DIR)/glad_rel.c,$(SOURCES))
SOURCES := $(filter-out $(SRC_DIR)/glad_dbg.c,$(SOURCES))

OBJECTS_DEBUG := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%_dbg.o,$(SOURCES))
OBJECTS_RELEASE := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%_rel.o,$(SOURCES))
OBJECTS_DEVELOP := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%_dev.o,$(SOURCES))

# Add object files for the generated glad files
OBJECTS_DEBUG += $(OBJ_DIR)/glad_dbg.o
OBJECTS_RELEASE += $(OBJ_DIR)/glad_rel.o
OBJECTS_DEVELOP += $(OBJ_DIR)/glad_dev.o

DEPENDS_DEBUG := $(OBJECTS_DEBUG:.o=.d)
DEPENDS_RELEASE := $(OBJECTS_RELEASE:.o=.d)
DEPENDS_DEVELOP := $(OBJECTS_DEVELOP:.o=.d)

TARGETS := $(BIN_DIR)/main_dbg $(BIN_DIR)/main_rel $(BIN_DIR)/main_dev


CC := gcc

# Common flags
CFLAGS := -DASSETSPATH=\"$(realpath assets)/\" -I$(INCLUDE_DIR) `pkg-config --cflags glfw3` `pkg-config --cflags cglm` -Wall -Wextra -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wmissing-prototypes -Wwrite-strings -Wcast-qual -Wswitch-default -Wswitch-enum -Wconversion -Wunreachable-code -Wimplicit-fallthrough -std=c11 -MMD
LDFLAGS := `pkg-config --libs glfw3` `pkg-config --libs cglm` -ldl -std=c11

# Flags for generating glad files (also common)
GLAD_FLAGS := --profile=core --api=gl=3.3 --spec=gl --extensions= --out-path=$$tmpdir


CFLAGS_DEBUG := -Og -g
LDFLAGS_DEBUG := $(CFLAGS_DEBUG)
CFLAGS_RELEASE := -DNDEBUG -flto -O2
LDFLAGS_RELEASE := $(CFLAGS_RELEASE)
LDFLAGS_DEVELOP := -fsanitize=address -fsanitize=undefined -Og
CFLAGS_DEVELOP := $(LDFLAGS_DEVELOP) -Werror

GLAD_FLAGS_DEBUG := --generator=c-debug
GLAD_FLAGS_RELEASE := --generator=c


.PHONY: dbg dev rel clean veryclean purify impolute

rel: $(BIN_DIR)/main
dev: $(BIN_DIR)/main_dev
dbg: $(BIN_DIR)/main_dbg

clean:
	-rm -f $(BIN_DIR)/* $(OBJ_DIR)/*.o
veryclean: clean
	-rm -f $(OBJ_DIR)/*
purify: veryclean
	-rm -f $(SRC_DIR)/glad_dbg.c $(SRC_DIR)/glad_rel.c
	-rm -f $(INCLUDE_DIR)/glad/glad_dbg.h $(INCLUDE_DIR)/glad/glad_rel.h
	-rm -rf $(INCLUDE_DIR)/KHR/
impolute:
	-rm -rf venv


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

# Specific dependencies for glad generated files
$(OBJ_DIR)/glad_rel.o: $(SRC_DIR)/glad_rel.c
$(OBJ_DIR)/glad_dbg.o: $(SRC_DIR)/glad_dbg.c
$(OBJ_DIR)/glad_dev.o: $(SRC_DIR)/glad_dbg.c
	$(CC) -c $(CFLAGS) -o $@ $<

# Can't avoid recipie repetition in pattern rules, see
# https://stackoverflow.com/questions/11441084/makefile-with-multiples-rules-sharing-same-recipe#comment38627982_11441134
$(OBJ_DIR)/%_dbg.o: $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) -o $@ $<
$(OBJ_DIR)/%_rel.o: $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) -o $@ $<
$(OBJ_DIR)/%_dev.o: $(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
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
	trap 'rm -r $$tmpdir'; \
	glad $(GLAD_FLAGS) $(GLAD_FLAGS_RELEASE); \
	cp $$tmpdir/include/glad/glad.h $(INCLUDE_DIR)/glad/glad_rel.h; \
	cp $$tmpdir/src/glad.c $(SRC_DIR)/glad_rel.c; \
	cp $$tmpdir/include/KHR/khrplatform.h $(INCLUDE_DIR)/KHR
$(INCLUDE_DIR)/glad/glad_dbg.h $(SRC_DIR)/glad_dbg.c &: venv
	mkdir -p $(INCLUDE_DIR)/glad
	set -e; \
	source venv/bin/activate; \
	tmpdir=`mktemp -d`; \
	trap 'rm -r $$tmpdir'; \
	glad $(GLAD_FLAGS) $(GLAD_FLAGS_DEBUG); \
	cp $$tmpdir/include/glad/glad.h $(INCLUDE_DIR)/glad/glad_dbg.h; \
	cp $$tmpdir/src/glad.c $(SRC_DIR)/glad_dbg.c
venv: requirements.txt
	virtualenv venv
	source venv/bin/activate; pip install -r requirements.txt


-include $(DEPENDS_DEBUG)
-include $(DEPENDS_RELEASE)
-include $(DEPENDS_DEVELOP)
