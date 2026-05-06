CC32 ?= i686-w64-mingw32-gcc
CC64 ?= x86_64-w64-mingw32-gcc
HOST_CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Os
HOST_CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O0 -g
LDFLAGS ?=
LIBS = -luser32 -lkernel32 -lgdi32 -lcomdlg32

SRC = src/main.c src/file_io.c src/settings.c src/text_encoding.c src/file_meta.c src/line_endings.c
TEST_SRC = tests/test_main.c tests/test_text_encoding.c tests/test_file_meta.c tests/test_line_endings.c src/text_encoding.c src/file_meta.c src/line_endings.c

.PHONY: all win32 win64 test clean

all: win32 win64

win32: build/dumbpad32.exe

win64: build/dumbpad64.exe

build:
	mkdir -p build

build/dumbpad32.exe: $(SRC) | build
	$(CC32) $(CFLAGS) -DWINVER=0x0601 -D_WIN32_WINNT=0x0601 -mwindows -municode -o $@ $(SRC) $(LDFLAGS) $(LIBS)

build/dumbpad64.exe: $(SRC) | build
	$(CC64) $(CFLAGS) -DWINVER=0x0601 -D_WIN32_WINNT=0x0601 -mwindows -municode -o $@ $(SRC) $(LDFLAGS) $(LIBS)

test: build/tests
	build/tests

build/tests: $(TEST_SRC) | build
	$(HOST_CC) $(HOST_CFLAGS) -o $@ $(TEST_SRC)

clean:
	rm -rf build
