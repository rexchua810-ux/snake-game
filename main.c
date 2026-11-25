#include <stdio.h>
#include <time.h>     // time() 在这里面
#include <stdlib.h>   // malloc、free 在这里面
#include "raylib.h"

#define CELL 30       // 每格画成 30x30 像素

#define EMPTY    0
#define FOOD     1
#define OBSTACLE 2

// 让玩家输入地图大小（宽和高都至少 5）
void getMapSize(int *width, int *height)
{
    do {
        printf("Map width (>= 5): ");
        scanf("%d", width);
        printf("Map height (>= 5): ");
        scanf("%d", height);
    } while (*width < 5 || *height < 5);   // 不合格就重新输入
}

// 用 malloc 建立 height 行、width 格的地图，全部设成空格
int **createMap(int width, int height)
{
    int **map = malloc(height * sizeof(int *));   // 第 1 步：存 height 个“行”
    for (int y = 0; y < height; y++)
    {
        map[y] = malloc(width * sizeof(int));     // 第 2 步：每行有 width 格
        for (int x = 0; x < width; x++)
            map[y][x] = EMPTY;
    }
    return map;
}

// 在随机的空格上放一个东西（type 是 FOOD 或 OBSTACLE）
void placeItem(int **map, int width, int height, int type)
{
    int x, y;
    do {
        x = rand() % width;
        y = rand() % height;
    } while (map[y][x] != EMPTY);   // 不是空格就重抽
    map[y][x] = type;
}

// 用完要把内存还回去：先还每一行，再还整个 map
void freeMap(int **map, int height)
{
    for (int y = 0; y < height; y++)
        free(map[y]);
    free(map);
}

// 画地图：食物红色、障碍物灰色，每格都有框线
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

int main(void)
{
    int width, height;
    getMapSize(&width, &height);

    int **map = createMap(width, height);

        srand(time(NULL));                    // 让每次随机结果都不同

    int obstacleCount = width * height / 20;   // 障碍物数量：大约占地图 5%
    for (int i = 0; i < obstacleCount; i++)
        placeItem(map, width, height, OBSTACLE);

    placeItem(map, width, height, FOOD);  // 先放 1 个食物

    InitWindow(width * CELL, height * CELL, "Snake");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);
        drawMap(map, width, height);
        EndDrawing();
    }

    CloseWindow();
    freeMap(map, height);
    return 0;
}