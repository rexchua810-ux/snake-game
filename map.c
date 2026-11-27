// map.c — 地图：建立/释放地图、在随机空格放东西、方向计算

#include <stdlib.h>
#include "game.h"

// ================= 地图 =================

// 用 malloc 建立 height 行、width 格的地图
int **createMap(int width, int height)
{
    int **map = malloc(height * sizeof(int *));
    for (int y = 0; y < height; y++)
        map[y] = malloc(width * sizeof(int));
    return map;
}

// 释放地图：先还每一行，再还整个 map
void freeMap(int **map, int height)
{
    for (int y = 0; y < height; y++)
        free(map[y]);
    free(map);
}

// 检查 (x, y) 这一格有没有蛇，有就回传 1
int isOnSnake(Snake *s, int x, int y)
{
    for (int i = 0; i < s->length; i++)
        if (s->x[i] == x && s->y[i] == y)
            return 1;
    return 0;
}

// ★ 从格子 (ax, ay) 往相邻的格子 (bx, by) 是哪个方向，结果放进 *dx、*dy
void neighborDir(int ax, int ay, int bx, int by, int *dx, int *dy)
{
    *dx = bx - ax;
    *dy = by - ay;
    // 无边界模式：两格隔着地图边缘（例如 0 和 width-1）时，换算回 1 或 -1
    if (*dx > 1)  *dx = -1;
    if (*dx < -1) *dx = 1;
    if (*dy > 1)  *dy = -1;
    if (*dy < -1) *dy = 1;
}

// ★ 第 i 节往第 j 节是哪个方向
void segmentDir(Game *g, int i, int j, int *dx, int *dy)
{
    Snake *s = &g->snake;
    neighborDir(s->x[i], s->y[i], s->x[j], s->y[j], dx, dy);
}

// ★ 检查 (x, y) 能不能放这种东西
int isValidSpot(Game *g, int x, int y, int type)
{
    if (g->map[y][x] != EMPTY || isOnSnake(&g->snake, x, y))
        return 0;
    if (type == OBSTACLE)
    {
        if (y == g->height / 2)        // 障碍物不放在蛇出发的那一行
            return 0;
        int distance = abs(x - g->snake.x[0]) + abs(y - g->snake.y[0]);
        if (distance < SAFE_DISTANCE)  // 障碍物不放在蛇头附近
            return 0;
    }
    return 1;
}

// ★ 改过：先数有几个能放的格子，再从里面随机挑一个（不会因为找不到空格而卡住）
void placeItem(Game *g, int type)
{
    // 第 1 轮：数一数有几个格子可以放
    int count = 0;
    for (int y = 0; y < g->height; y++)
        for (int x = 0; x < g->width; x++)
            if (isValidSpot(g, x, y, type))
                count++;

    if (count == 0)    // 一个都没有就不放
        return;

    // 第 2 轮：随机决定放在第几个可以放的格子
    int target = rand() % count;
    for (int y = 0; y < g->height; y++)
    {
        for (int x = 0; x < g->width; x++)
        {
            if (isValidSpot(g, x, y, type))
            {
                if (target == 0)
                {
                    g->map[y][x] = type;
                    return;
                }
                target--;
            }
        }
    }
}
