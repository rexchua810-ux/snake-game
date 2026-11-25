#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"

#define CELL 30

#define EMPTY    0
#define FOOD     1
#define OBSTACLE 2

#define MOVE_TIME 0.15f   // ★ 每 0.15 秒走一格（数字越小越快）

// ★ 蛇的资料
typedef struct {
    int *x;          // 每一节的 x，x[0] 是头
    int *y;          // 每一节的 y
    int length;      // 现在有几节
    int dirX, dirY;  // 移动方向
} Snake;

void getMapSize(int *width, int *height)
{
    do {
        printf("Map width (>= 5): ");
        scanf("%d", width);
        printf("Map height (>= 5): ");
        scanf("%d", height);
    } while (*width < 5 || *height < 5);
}

int **createMap(int width, int height)
{
    int **map = malloc(height * sizeof(int *));
    for (int y = 0; y < height; y++)
    {
        map[y] = malloc(width * sizeof(int));
        for (int x = 0; x < width; x++)
            map[y][x] = EMPTY;
    }
    return map;
}

void freeMap(int **map, int height)
{
    for (int y = 0; y < height; y++)
        free(map[y]);
    free(map);
}

// ★ 建立蛇：长度 3，头在地图中间，身体往左排，一开始往右走
Snake createSnake(int width, int height)
{
    Snake s;
    s.x = malloc(width * height * sizeof(int));
    s.y = malloc(width * height * sizeof(int));
    s.length = 3;
    s.dirX = 1;
    s.dirY = 0;
    for (int i = 0; i < s.length; i++)
    {
        s.x[i] = width / 2 - i;
        s.y[i] = height / 2;
    }
    return s;
}

// ★ 释放蛇的内存
void freeSnake(Snake *s)
{
    free(s->x);
    free(s->y);
}

// ★ 检查 (x, y) 这一格有没有蛇，有就回传 1，没有就回传 0
int isOnSnake(Snake *s, int x, int y)
{
    for (int i = 0; i < s->length; i++)
        if (s->x[i] == x && s->y[i] == y)
            return 1;
    return 0;
}

// ★ 改过：多传入蛇，避免把东西放在蛇身上
void placeItem(int **map, int width, int height, Snake *s, int type)
{
    int x, y;
    do {
        x = rand() % width;
        y = rand() % height;
    } while (map[y][x] != EMPTY || isOnSnake(s, x, y));
    map[y][x] = type;
}

// ★ 读键盘改方向（不能直接掉头，比如往右走时不能马上往左）
void handleInput(Snake *s)
{
    if (IsKeyPressed(KEY_W) && s->dirY != 1)  { s->dirX = 0;  s->dirY = -1; }
    if (IsKeyPressed(KEY_S) && s->dirY != -1) { s->dirX = 0;  s->dirY = 1;  }
    if (IsKeyPressed(KEY_A) && s->dirX != 1)  { s->dirX = -1; s->dirY = 0;  }
    if (IsKeyPressed(KEY_D) && s->dirX != -1) { s->dirX = 1;  s->dirY = 0;  }
}

// ★ 蛇往前走一格
void moveSnake(Snake *s)
{
    // 从尾巴开始，每一节移到前一节原来的位置
    for (int i = s->length - 1; i > 0; i--)
    {
        s->x[i] = s->x[i - 1];
        s->y[i] = s->y[i - 1];
    }
    // 头往目前的方向走一格
    s->x[0] += s->dirX;
    s->y[0] += s->dirY;
}

void drawMap(int **map, int width, int height)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (map[y][x] == FOOD)
                DrawRectangle(x * CELL, y * CELL, CELL, CELL, RED);
            else if (map[y][x] == OBSTACLE)
                DrawRectangle(x * CELL, y * CELL, CELL, CELL, GRAY);

            DrawRectangleLines(x * CELL, y * CELL, CELL, CELL, DARKGRAY);
        }
    }
}

// ★ 画蛇：头用亮绿色，身体用绿色
void drawSnake(Snake *s)
{
    for (int i = 0; i < s->length; i++)
    {
        if (i == 0)
            DrawRectangle(s->x[i] * CELL, s->y[i] * CELL, CELL, CELL, LIME);
        else
            DrawRectangle(s->x[i] * CELL, s->y[i] * CELL, CELL, CELL, GREEN);
    }
}

int main(void)
{
    int width, height;
    getMapSize(&width, &height);

    int **map = createMap(width, height);
    Snake snake = createSnake(width, height);       // ★ 先放蛇

    srand(time(NULL));

    int obstacleCount = width * height / 20;
    for (int i = 0; i < obstacleCount; i++)
        placeItem(map, width, height, &snake, OBSTACLE);   // ★ 多传 &snake

    placeItem(map, width, height, &snake, FOOD);           // ★ 多传 &snake

    InitWindow(width * CELL, height * CELL, "Snake");
    SetTargetFPS(60);

    float timer = 0;                                 // ★ 计时器

    while (!WindowShouldClose())
    {
        handleInput(&snake);                         // ★ 每一帧都读键盘

        timer += GetFrameTime();                     // ★ 加上这一帧经过的秒数
        if (timer >= MOVE_TIME)                      // ★ 满 0.15 秒就走一格
        {
            moveSnake(&snake);
            timer = 0;
        }

        BeginDrawing();
        ClearBackground(BLACK);
        drawMap(map, width, height);
        drawSnake(&snake);                           // ★ 画蛇
        EndDrawing();
    }

    CloseWindow();
    freeSnake(&snake);                               // ★ 释放蛇
    freeMap(map, height);
    return 0;
}