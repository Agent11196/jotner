CC = clang
TARGET = jotner

CFLAGS = -std=c23 -g -Wall -Wextra
LDFLAGS = -lGL -lX11
BUILD_DIR = build/dbg/

CFLAGS_REL = -std=c23 -Wall -Wextra -Werror -O3 -DNDEBUG -fomit-frame-pointer -pipe -march=native
LDFLAGS_REL = -flto -s -Wl,--gc-sections -Wl,-O1
BUILD_DIR_REL = build/rel/


all: ${BUILD_DIR}${TARGET}
${BUILD_DIR}${TARGET}: ${TARGET}.c image.vert.glsl image.frag.glsl makefile | ${BUILD_DIR}
	${CC} ${CFLAGS} ${LDFLAGS} ${TARGET}.c -o ${BUILD_DIR}${TARGET}
release: | ${BUILD_DIR_REL}
	${CC} ${CFLAGS_REL} ${LDFLAGS} ${LDFLAGS_REL} ${TARGET}.c -o ${BUILD_DIR_REL}${TARGET}

${BUILD_DIR}:
	mkdir -p ${BUILD_DIR}
${BUILD_DIR_REL}:
	mkdir -p ${BUILD_DIR_REL}
