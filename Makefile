# Makefile — 编译用
#   make       编译（只会重新编译有改过的部分）
#   make run   编译并执行
#   make clean 删除编译出来的文件

CC      = clang
CFLAGS  = -Wall -Wextra $(shell pkg-config --cflags raylib)
LDLIBS  = $(shell pkg-config --libs raylib)

SRC = main.c map.c game.c input.c assets.c draw.c
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
