# Makefile — for compiling
#   make       compile (only recompiles what changed)
#   make run   compile and run
#   make clean delete the compiled files

CC      = clang
CFLAGS  = -Wall -Wextra $(shell pkg-config --cflags raylib)
LDLIBS  = $(shell pkg-config --libs raylib)

SRC = main.c map.c game.c input.c assets.c draw.c effects.c
OBJ = $(SRC:.c=.o)

snake: $(OBJ)
	$(CC) $(OBJ) -o snake $(LDLIBS)

%.o: %.c game.h
	$(CC) $(CFLAGS) -c $< -o $@

run: snake
	./snake

clean:
	rm -f snake $(OBJ)

.PHONY: run clean
