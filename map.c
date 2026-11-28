// map.c — create/free the map, place items on random empty cells, directions
// AI disclosure: parts of this file were written with the help of AI tools (Claude / Claude Code).
//   The author reviewed the code, tested it in the game, and can explain how it works.
//   See "Use of Generative AI" in README.md for details.

#include <stdlib.h>
#include "game.h"

// ================= Map =================

// create a map with height rows and width cells using malloc
int **createMap(int width, int height)
{
    int **map = malloc(height * sizeof(int *));
    for (int y = 0; y < height; y++)
        map[y] = malloc(width * sizeof(int));
    return map;
}

// free the map: first every row, then the row array itself
void freeMap(int **map, int height)
{
    for (int y = 0; y < height; y++)
        free(map[y]);
    free(map);
}

// return 1 if the snake is on cell (x, y)
static int isOnSnake(Snake *s, int x, int y)
{
    for (int i = 0; i < s->length; i++)
        if (s->x[i] == x && s->y[i] == y)
            return 1;
    return 0;
}

// direction from cell (ax, ay) to the neighboring cell (bx, by), stored in *dx and *dy
void neighborDir(int ax, int ay, int bx, int by, int *dx, int *dy)
{
    *dx = bx - ax;
    *dy = by - ay;
    // No Walls mode: if the two cells are on opposite edges (e.g. 0 and width-1), convert back to 1 or -1
    if (*dx > 1)  *dx = -1;
    if (*dx < -1) *dx = 1;
    if (*dy > 1)  *dy = -1;
    if (*dy < -1) *dy = 1;
}

// direction from segment i to segment j
void segmentDir(Game *g, int i, int j, int *dx, int *dy)
{
    Snake *s = &g->snake;
    neighborDir(s->x[i], s->y[i], s->x[j], s->y[j], dx, dy);
}

// check whether this type of item may be placed on (x, y)
static int isValidSpot(Game *g, int x, int y, int type)
{
    if (g->map[y][x] != EMPTY || isOnSnake(&g->snake, x, y))
        return 0;
    if (type == OBSTACLE)
    {
        if (y == g->height / 2)        // no obstacles on the row where the snake starts
            return 0;
        int distance = abs(x - g->snake.x[0]) + abs(y - g->snake.y[0]);
        if (distance < SAFE_DISTANCE)  // no obstacles near the snake's head
            return 0;
    }
    return 1;
}

// count the valid cells first, then pick one at random (never gets stuck when the map is full)
void placeItem(Game *g, int type)
{
    // pass 1: count how many cells are valid
    int count = 0;
    for (int y = 0; y < g->height; y++)
        for (int x = 0; x < g->width; x++)
            if (isValidSpot(g, x, y, type))
                count++;

    if (count == 0)    // none available: place nothing
        return;

    // pass 2: randomly choose which valid cell to use
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
